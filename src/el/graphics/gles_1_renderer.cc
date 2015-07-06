/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cassert>
#include <cstdio>

#include "el/graphics/bitmap_fragment.h"
#include "el/graphics/gles_1_renderer.h"
#include "el/util/debug.h"
#include "el/util/math.h"

namespace el {
namespace graphics {

#ifdef EL_RUNTIME_DEBUG_INFO
uint32_t dbg_bitmap_validations = 0;
#endif  // EL_RUNTIME_DEBUG_INFO

static void Ortho2D(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top) {
#ifdef EL_RENDERER_GLES_1
  glOrthof(left, right, bottom, top, -1.0, 1.0);
#else
  glOrtho(left, right, bottom, top, -1.0, 1.0);
#endif
}

GLuint g_current_texture = (GLuint)-1;
BatchingRenderer::Batch* g_current_batch = 0;

void BindBitmap(Bitmap* bitmap) {
  GLuint texture = bitmap ? static_cast<BitmapGL*>(bitmap)->m_texture : 0;
  if (texture != g_current_texture) {
    g_current_texture = texture;
    glBindTexture(GL_TEXTURE_2D, g_current_texture);
  }
}

BitmapGL::BitmapGL(RendererGL* renderer) : m_renderer(renderer) {}

BitmapGL::~BitmapGL() {
  // Must flush and unbind before we delete the texture
  m_renderer->FlushBitmap(this);
  if (m_texture == g_current_texture) {
    BindBitmap(nullptr);
  }
  glDeleteTextures(1, &m_texture);
}

bool BitmapGL::Init(int width, int height, uint32_t* data) {
  assert(width == util::GetNearestPowerOfTwo(width));
  assert(height == util::GetNearestPowerOfTwo(height));

  m_w = width;
  m_h = height;

  glGenTextures(1, &m_texture);
  BindBitmap(this);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  set_data(data);

  return true;
}

void BitmapGL::set_data(uint32_t* data) {
  m_renderer->FlushBitmap(this);
  BindBitmap(this);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_w, m_h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, data);
  EL_IF_DEBUG_SETTING(util::DebugInfo::Setting::kDrawRenderBatches,
                      dbg_bitmap_validations++);
}

RendererGL::RendererGL() = default;

void RendererGL::BeginPaint(int render_target_w, int render_target_h) {
#ifdef EL_RUNTIME_DEBUG_INFO
  dbg_bitmap_validations = 0;
#endif

  BatchingRenderer::BeginPaint(render_target_w, render_target_h);

  g_current_texture = (GLuint)-1;
  g_current_batch = nullptr;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  Ortho2D(0, (GLfloat)render_target_w, (GLfloat)render_target_h, 0);
  glMatrixMode(GL_MODELVIEW);
  glViewport(0, 0, render_target_w, render_target_h);
  glScissor(0, 0, render_target_w, render_target_h);

  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
}

void RendererGL::EndPaint() {
  BatchingRenderer::EndPaint();

#ifdef EL_RUNTIME_DEBUG_INFO
  if (EL_DEBUG_SETTING(util::DebugInfo::Setting::kDrawRenderBatches))
    TBDebugOut("Frame caused %d bitmap validations.\n", dbg_bitmap_validations);
#endif  // EL_RUNTIME_DEBUG_INFO
}

std::unique_ptr<Bitmap> RendererGL::CreateBitmap(int width, int height,
                                                 uint32_t* data) {
  auto bitmap = std::make_unique<BitmapGL>(this);
  if (!bitmap->Init(width, height, data)) {
    return nullptr;
  }
  return std::unique_ptr<Bitmap>(std::move(bitmap));
}

void RendererGL::RenderBatch(Batch* batch) {
  // Bind texture and array pointers
  BindBitmap(batch->bitmap);
  if (g_current_batch != batch) {
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex),
                   (void*)&batch->vertex[0].r);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (void*)&batch->vertex[0].u);
    glVertexPointer(2, GL_FLOAT, sizeof(Vertex), (void*)&batch->vertex[0].x);
    g_current_batch = batch;
  }

  // Flush
  glDrawArrays(GL_TRIANGLES, 0, batch->vertex_count);
}

void RendererGL::set_clip_rect(const Rect& rect) {
  glScissor(clip_rect_.x, screen_rect_.h - (clip_rect_.y + clip_rect_.h),
            clip_rect_.w, clip_rect_.h);
}

}  // namespace graphics
}  // namespace el
