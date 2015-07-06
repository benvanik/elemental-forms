/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/graphics/batching_renderer.h"
#include "el/graphics/bitmap_fragment.h"
#include "el/util/debug.h"

namespace el {
namespace graphics {

#ifdef EL_RUNTIME_DEBUG_INFO
uint32_t dbg_begin_paint_batch_id = 0;
uint32_t dbg_frame_triangle_count = 0;
#endif  // EL_RUNTIME_DEBUG_INFO

#define VER_COL(r, g, b, a) (((a) << 24) + ((b) << 16) + ((g) << 8) + r)
#define VER_COL_OPACITY(a) (0x00ffffff + (((uint32_t)a) << 24))

void BatchingRenderer::Batch::Flush(BatchingRenderer* batch_renderer) {
  if (!vertex_count || is_flushing) {
    return;
  }

  // Prevent re-entrancy. Calling fragment->GetBitmap may end up calling
  // Bitmap::SetData which will end up flushing any existing batch with that
  // bitmap.
  is_flushing = true;

  if (fragment) {
    // Now it's time to ensure the bitmap data is up to date. A call to
    // GetBitmap with Validate::kAlways should guarantee that its data is
    // validated.
    auto frag_bitmap = fragment->GetBitmap(Validate::kAlways);
    ((void)frag_bitmap);  // silence warning about unused variable
    assert(frag_bitmap == bitmap);
  }

  batch_renderer->RenderBatch(this);

#ifdef EL_RUNTIME_DEBUG_INFO
  if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kDrawRenderBatches)) {
    // This assumes we're drawing triangles. Need to modify this
    // if we start using strips, fans or whatever.
    dbg_frame_triangle_count += vertex_count / 3;

    // Draw the triangles again using a random color based on the batch
    // id. This indicates which triangles belong to the same batch_.
    uint32_t id = batch_id - dbg_begin_paint_batch_id;
    uint32_t hash = id * (2166136261U ^ id);
    uint32_t color = 0xAA000000 + (hash & 0x00FFFFFF);
    for (int i = 0; i < vertex_count; ++i) {
      vertex[i].col = color;
    }
    bitmap = nullptr;
    batch_renderer->RenderBatch(this);
  }
#endif  // EL_RUNTIME_DEBUG_INFO

  vertex_count = 0;

  ++batch_id;  // Will overflow eventually, but that doesn't really matter.

  is_flushing = false;
}

BatchingRenderer::Vertex* BatchingRenderer::Batch::Reserve(
    BatchingRenderer* batch_renderer, int count) {
  assert(count < kVertexBatchSize);
  if (vertex_count + count > kVertexBatchSize) {
    Flush(batch_renderer);
  }
  Vertex* ret = &vertex[vertex_count];
  vertex_count += count;
  return ret;
}

BatchingRenderer::BatchingRenderer() = default;

BatchingRenderer::~BatchingRenderer() = default;

void BatchingRenderer::BeginPaint(int render_target_w, int render_target_h) {
#ifdef EL_RUNTIME_DEBUG_INFO
  dbg_begin_paint_batch_id = batch_.batch_id;
  dbg_frame_triangle_count = 0;
#endif  // EL_RUNTIME_DEBUG_INFO

  screen_rect_.reset(0, 0, render_target_w, render_target_h);
  clip_rect_ = screen_rect_;
}

void BatchingRenderer::EndPaint() {
  FlushAllInternal();

#ifdef EL_RUNTIME_DEBUG_INFO
  if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kDrawRenderBatches)) {
    TBDebugOut("Frame rendered using %d batches and a total of %d triangles.\n",
               batch_.batch_id - dbg_begin_paint_batch_id,
               dbg_frame_triangle_count);
  }
#endif  // EL_RUNTIME_DEBUG_INFO
}

void BatchingRenderer::Translate(int dx, int dy) {
  translation_x_ += dx;
  translation_y_ += dy;
}

void BatchingRenderer::set_opacity(float opacity) {
  int8_t opacity8 = (uint8_t)(opacity * 255);
  if (opacity8 == opacity_) return;
  opacity_ = opacity8;
}

float BatchingRenderer::opacity() { return opacity_ / 255.0f; }

Rect BatchingRenderer::set_clip_rect(const Rect& rect, bool add_to_current) {
  Rect old_clip_rect = clip_rect_;
  clip_rect_ = rect;
  clip_rect_.x += translation_x_;
  clip_rect_.y += translation_y_;

  if (add_to_current) {
    clip_rect_ = clip_rect_.Clip(old_clip_rect);
  }

  FlushAllInternal();
  set_clip_rect(clip_rect_);

  old_clip_rect.x -= translation_x_;
  old_clip_rect.y -= translation_y_;
  return old_clip_rect;
}

Rect BatchingRenderer::clip_rect() {
  Rect curr_clip_rect = clip_rect_;
  curr_clip_rect.x -= translation_x_;
  curr_clip_rect.y -= translation_y_;
  return curr_clip_rect;
}

void BatchingRenderer::DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                                  BitmapFragment* bitmap_fragment) {
  if (auto bitmap = bitmap_fragment->GetBitmap(Validate::kFirstTime)) {
    AddQuadInternal(
        dst_rect.Offset(translation_x_, translation_y_),
        src_rect.Offset(bitmap_fragment->m_rect.x, bitmap_fragment->m_rect.y),
        VER_COL_OPACITY(opacity_), bitmap, bitmap_fragment);
  }
}

void BatchingRenderer::DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                                  Bitmap* bitmap) {
  AddQuadInternal(dst_rect.Offset(translation_x_, translation_y_), src_rect,
                  VER_COL_OPACITY(opacity_), bitmap, nullptr);
}

void BatchingRenderer::DrawBitmapColored(const Rect& dst_rect,
                                         const Rect& src_rect,
                                         const Color& color,
                                         BitmapFragment* bitmap_fragment) {
  if (auto bitmap = bitmap_fragment->GetBitmap(Validate::kFirstTime)) {
    uint32_t a = (color.a * opacity_) / 255;
    AddQuadInternal(
        dst_rect.Offset(translation_x_, translation_y_),
        src_rect.Offset(bitmap_fragment->m_rect.x, bitmap_fragment->m_rect.y),
        VER_COL(color.r, color.g, color.b, a), bitmap, bitmap_fragment);
  }
}

void BatchingRenderer::DrawBitmapColored(const Rect& dst_rect,
                                         const Rect& src_rect,
                                         const Color& color, Bitmap* bitmap) {
  uint32_t a = (color.a * opacity_) / 255;
  AddQuadInternal(dst_rect.Offset(translation_x_, translation_y_), src_rect,
                  VER_COL(color.r, color.g, color.b, a), bitmap, nullptr);
}

void BatchingRenderer::DrawBitmapTile(const Rect& dst_rect, Bitmap* bitmap) {
  AddQuadInternal(dst_rect.Offset(translation_x_, translation_y_),
                  Rect(0, 0, dst_rect.w, dst_rect.h), VER_COL_OPACITY(opacity_),
                  bitmap, nullptr);
}

void BatchingRenderer::DrawRect(const Rect& dst_rect, const Color& color) {
  if (dst_rect.empty()) return;
  // Top.
  DrawRectFill(Rect(dst_rect.x, dst_rect.y, dst_rect.w, 1), color);
  // Bottom.
  DrawRectFill(Rect(dst_rect.x, dst_rect.y + dst_rect.h - 1, dst_rect.w, 1),
               color);
  // Left.
  DrawRectFill(Rect(dst_rect.x, dst_rect.y + 1, 1, dst_rect.h - 2), color);
  // Right.
  DrawRectFill(
      Rect(dst_rect.x + dst_rect.w - 1, dst_rect.y + 1, 1, dst_rect.h - 2),
      color);
}

void BatchingRenderer::DrawRectFill(const Rect& dst_rect, const Color& color) {
  if (dst_rect.empty()) return;
  uint32_t a = (color.a * opacity_) / 255;
  AddQuadInternal(dst_rect.Offset(translation_x_, translation_y_), Rect(),
                  VER_COL(color.r, color.g, color.b, a), nullptr, nullptr);
}

void BatchingRenderer::AddQuadInternal(const Rect& dst_rect,
                                       const Rect& src_rect, uint32_t color,
                                       Bitmap* bitmap,
                                       BitmapFragment* fragment) {
  if (batch_.bitmap != bitmap) {
    batch_.Flush(this);
    batch_.bitmap = bitmap;
  }
  batch_.fragment = fragment;
  if (bitmap) {
    int bitmap_w = bitmap->width();
    int bitmap_h = bitmap->height();
    m_u = (float)src_rect.x / bitmap_w;
    m_v = (float)src_rect.y / bitmap_h;
    m_uu = (float)(src_rect.x + src_rect.w) / bitmap_w;
    m_vv = (float)(src_rect.y + src_rect.h) / bitmap_h;
  }
  Vertex* ver = batch_.Reserve(this, 6);
  ver[0].x = (float)dst_rect.x;
  ver[0].y = (float)(dst_rect.y + dst_rect.h);
  ver[0].u = m_u;
  ver[0].v = m_vv;
  ver[0].col = color;
  ver[1].x = (float)(dst_rect.x + dst_rect.w);
  ver[1].y = (float)(dst_rect.y + dst_rect.h);
  ver[1].u = m_uu;
  ver[1].v = m_vv;
  ver[1].col = color;
  ver[2].x = (float)dst_rect.x;
  ver[2].y = (float)dst_rect.y;
  ver[2].u = m_u;
  ver[2].v = m_v;
  ver[2].col = color;

  ver[3].x = (float)dst_rect.x;
  ver[3].y = (float)dst_rect.y;
  ver[3].u = m_u;
  ver[3].v = m_v;
  ver[3].col = color;
  ver[4].x = (float)(dst_rect.x + dst_rect.w);
  ver[4].y = (float)(dst_rect.y + dst_rect.h);
  ver[4].u = m_uu;
  ver[4].v = m_vv;
  ver[4].col = color;
  ver[5].x = (float)(dst_rect.x + dst_rect.w);
  ver[5].y = (float)dst_rect.y;
  ver[5].u = m_uu;
  ver[5].v = m_v;
  ver[5].col = color;

  // Update fragments batch id (See FlushBitmapFragment).
  if (fragment) {
    fragment->m_batch_id = batch_.batch_id;
  }
}

void BatchingRenderer::FlushAllInternal() { batch_.Flush(this); }

void BatchingRenderer::FlushBitmap(Bitmap* bitmap) {
  // Flush the batch if it's using this bitmap (that is about to change or be
  // deleted).
  if (batch_.vertex_count && bitmap == batch_.bitmap) {
    batch_.Flush(this);
  }
}

void BatchingRenderer::FlushBitmapFragment(BitmapFragment* bitmap_fragment) {
  // Flush the batch if it is using this fragment (that is about to change or be
  // deleted).
  // We know if it is in use in the current batch if its batch_id matches the
  // current batch_id in our (one and only) batch_.
  // If we switch to a more advance batching system with multiple batches, we
  // need to solve this a bit differently.
  if (batch_.vertex_count && bitmap_fragment->m_batch_id == batch_.batch_id) {
    batch_.Flush(this);
  }
}

}  // namespace graphics
}  // namespace el
