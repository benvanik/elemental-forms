/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/graphics/bitmap_fragment.h"
#include "el/graphics/renderer.h"
#include "el/util/debug.h"

namespace el {
namespace graphics {

#define VER_COL(r, g, b, a) (((a) << 24) + ((b) << 16) + ((g) << 8) + r)
#define VER_COL_OPACITY(a) (0x00ffffff + (((uint32_t)a) << 24))

Renderer* Renderer::renderer_singleton_ = nullptr;

Renderer::Renderer() = default;

Renderer::~Renderer() {
  if (renderer_singleton_ == this) {
    renderer_singleton_ = nullptr;
  }
}

void Renderer::InvokeContextLost() {
  auto iter = listeners_.IterateForward();
  while (RendererListener* listener = iter.GetAndStep()) {
    listener->OnContextLost();
  }
}

void Renderer::InvokeContextRestored() {
  auto iter = listeners_.IterateForward();
  while (RendererListener* listener = iter.GetAndStep()) {
    listener->OnContextRestored();
  }
}

void Renderer::BeginPaint(int render_target_w, int render_target_h) {
  begin_paint_batch_id_ = batch_.batch_id;
  frame_triangle_count_ = 0;

  screen_rect_.reset(0, 0, render_target_w, render_target_h);
  clip_rect_ = screen_rect_;
}

void Renderer::EndPaint() {
  FlushAllInternal();

#ifdef EL_RUNTIME_DEBUG_INFO
  if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kDrawRenderBatches)) {
    TBDebugOut("Frame rendered using %d batches and a total of %d triangles.\n",
               batch_.batch_id - begin_paint_batch_id_, frame_triangle_count_);
  }
#endif  // EL_RUNTIME_DEBUG_INFO
}

void Renderer::Translate(int dx, int dy) {
  translation_x_ += dx;
  translation_y_ += dy;
}

void Renderer::set_opacity(float opacity) {
  int8_t opacity8 = (uint8_t)(opacity * 255);
  if (opacity8 == opacity_) return;
  opacity_ = opacity8;
}

float Renderer::opacity() { return opacity_ / 255.0f; }

Rect Renderer::set_clip_rect(const Rect& rect, bool add_to_current) {
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

Rect Renderer::clip_rect() {
  Rect curr_clip_rect = clip_rect_;
  curr_clip_rect.x -= translation_x_;
  curr_clip_rect.y -= translation_y_;
  return curr_clip_rect;
}

void Renderer::DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                          BitmapFragment* bitmap_fragment) {
  if (auto bitmap = bitmap_fragment->GetBitmap(Validate::kFirstTime)) {
    AddQuadInternal(
        dst_rect.Offset(translation_x_, translation_y_),
        src_rect.Offset(bitmap_fragment->m_rect.x, bitmap_fragment->m_rect.y),
        VER_COL_OPACITY(opacity_), bitmap, bitmap_fragment);
  }
}

void Renderer::DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                          Bitmap* bitmap) {
  AddQuadInternal(dst_rect.Offset(translation_x_, translation_y_), src_rect,
                  VER_COL_OPACITY(opacity_), bitmap, nullptr);
}

void Renderer::DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
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

void Renderer::DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                                 const Color& color, Bitmap* bitmap) {
  uint32_t a = (color.a * opacity_) / 255;
  AddQuadInternal(dst_rect.Offset(translation_x_, translation_y_), src_rect,
                  VER_COL(color.r, color.g, color.b, a), bitmap, nullptr);
}

void Renderer::DrawBitmapTile(const Rect& dst_rect, Bitmap* bitmap) {
  AddQuadInternal(dst_rect.Offset(translation_x_, translation_y_),
                  Rect(0, 0, dst_rect.w, dst_rect.h), VER_COL_OPACITY(opacity_),
                  bitmap, nullptr);
}

void Renderer::DrawRect(const Rect& dst_rect, const Color& color) {
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

void Renderer::DrawRectFill(const Rect& dst_rect, const Color& color) {
  if (dst_rect.empty()) return;
  uint32_t a = (color.a * opacity_) / 255;
  AddQuadInternal(dst_rect.Offset(translation_x_, translation_y_), Rect(),
                  VER_COL(color.r, color.g, color.b, a), nullptr, nullptr);
}

void Renderer::AddQuadInternal(const Rect& dst_rect, const Rect& src_rect,
                               uint32_t color, Bitmap* bitmap,
                               BitmapFragment* fragment) {
  // On state change force flush.
  if (batch_.bitmap != bitmap) {
    FlushBatch();
  }

  // Reserve vertices for our quad.
  // This will flush if the buffer is full.
  Vertex* v = ReserveVertices(6);

  // Setup batch textures (if any).
  batch_.bitmap = bitmap;
  if (bitmap) {
    int bitmap_w = bitmap->width();
    int bitmap_h = bitmap->height();
    m_u = static_cast<float>(src_rect.x) / bitmap_w;
    m_v = static_cast<float>(src_rect.y) / bitmap_h;
    m_uu = static_cast<float>(src_rect.x + src_rect.w) / bitmap_w;
    m_vv = static_cast<float>(src_rect.y + src_rect.h) / bitmap_h;
  }
  batch_.fragment = fragment;
  if (fragment) {
    // Update fragments batch id (See FlushBitmapFragment).
    fragment->m_batch_id = batch_.batch_id;
  }

  v[0].x = static_cast<float>(dst_rect.x);
  v[0].y = static_cast<float>(dst_rect.y + dst_rect.h);
  v[0].u = m_u;
  v[0].v = m_vv;
  v[0].color = color;
  v[1].x = static_cast<float>(dst_rect.x + dst_rect.w);
  v[1].y = static_cast<float>(dst_rect.y + dst_rect.h);
  v[1].u = m_uu;
  v[1].v = m_vv;
  v[1].color = color;
  v[2].x = static_cast<float>(dst_rect.x);
  v[2].y = static_cast<float>(dst_rect.y);
  v[2].u = m_u;
  v[2].v = m_v;
  v[2].color = color;

  v[3].x = static_cast<float>(dst_rect.x);
  v[3].y = static_cast<float>(dst_rect.y);
  v[3].u = m_u;
  v[3].v = m_v;
  v[3].color = color;
  v[4].x = static_cast<float>(dst_rect.x + dst_rect.w);
  v[4].y = static_cast<float>(dst_rect.y + dst_rect.h);
  v[4].u = m_uu;
  v[4].v = m_vv;
  v[4].color = color;
  v[5].x = static_cast<float>(dst_rect.x + dst_rect.w);
  v[5].y = static_cast<float>(dst_rect.y);
  v[5].u = m_uu;
  v[5].v = m_v;
  v[5].color = color;
}

Renderer::Vertex* Renderer::ReserveVertices(size_t vertex_count) {
  assert(vertex_count < max_vertex_batch_size());
  if (batch_.vertex_count + vertex_count > max_vertex_batch_size()) {
    FlushBatch();
  }
  Vertex* ret = &batch_.vertices[batch_.vertex_count];
  batch_.vertex_count += vertex_count;
  return ret;
}

void Renderer::FlushBatch() {
  if (!batch_.vertex_count || batch_.is_flushing) {
    return;
  }

  // Prevent re-entrancy. Calling fragment->GetBitmap may end up calling
  // Bitmap::SetData which will end up flushing any existing batch with that
  // bitmap.
  batch_.is_flushing = true;

  if (batch_.fragment) {
    // Now it's time to ensure the bitmap data is up to date. A call to
    // GetBitmap with Validate::kAlways should guarantee that its data is
    // validated.
    auto frag_bitmap = batch_.fragment->GetBitmap(Validate::kAlways);
    assert(frag_bitmap == batch_.bitmap);
  }

  RenderBatch(&batch_);

#ifdef EL_RUNTIME_DEBUG_INFO
  if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kDrawRenderBatches)) {
    // This assumes we're drawing triangles. Need to modify this
    // if we start using strips, fans or whatever.
    frame_triangle_count_ += batch_.vertex_count / 3;

    // Draw the triangles again using a random color based on the batch
    // id. This indicates which triangles belong to the same batch_.
    size_t id = batch_.batch_id - begin_paint_batch_id_;
    uint32_t hash = uint32_t(id * (2166136261U ^ id));
    uint32_t color = 0xAA000000 + (hash & 0x00FFFFFF);
    for (int i = 0; i < batch_.vertex_count; ++i) {
      batch_.vertices[i].color = color;
    }
    batch_.bitmap = nullptr;
    RenderBatch(&batch_);
  }
#endif  // EL_RUNTIME_DEBUG_INFO

  batch_.vertex_count = 0;

  // Will overflow eventually, but that doesn't really matter.
  ++batch_.batch_id;

  batch_.is_flushing = false;
}

void Renderer::FlushAllInternal() { FlushBatch(); }

void Renderer::FlushBitmap(Bitmap* bitmap) {
  // Flush the batch if it's using this bitmap (that is about to change or be
  // deleted).
  if (batch_.vertex_count && bitmap == batch_.bitmap) {
    FlushBatch();
  }
}

void Renderer::FlushBitmapFragment(BitmapFragment* bitmap_fragment) {
  // Flush the batch if it is using this fragment (that is about to change or be
  // deleted).
  // We know if it is in use in the current batch if its batch_id matches the
  // current batch_id in our (one and only) batch_.
  // If we switch to a more advance batching system with multiple batches, we
  // need to solve this a bit differently.
  if (batch_.vertex_count && bitmap_fragment->m_batch_id == batch_.batch_id) {
    FlushBatch();
  }
}

}  // namespace graphics
}  // namespace el
