/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>
#include <cstdio>

#include "tb_bitmap_fragment.h"
#include "tb_renderer_gl.h"

#include "tb/util/debug.h"

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO
uint32_t dbg_bitmap_validations = 0;
#endif  // TB_RUNTIME_DEBUG_INFO

static void Ortho2D(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top) {
#ifdef TB_RENDERER_GLES_1
  glOrthof(left, right, bottom, top, -1.0, 1.0);
#else
  glOrtho(left, right, bottom, top, -1.0, 1.0);
#endif
}

GLuint g_current_texture = (GLuint)-1;
RendererBatcher::Batch* g_current_batch = 0;

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
  assert(width == GetNearestPowerOfTwo(width));
  assert(height == GetNearestPowerOfTwo(height));

  m_w = width;
  m_h = height;

  glGenTextures(1, &m_texture);
  BindBitmap(this);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  SetData(data);

  return true;
}

void BitmapGL::SetData(uint32_t* data) {
  m_renderer->FlushBitmap(this);
  BindBitmap(this);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_w, m_h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, data);
  TB_IF_DEBUG_SETTING(util::DebugInfo::Setting::kDrawRenderBatches,
                      dbg_bitmap_validations++);
}

RendererGL::RendererGL() = default;

void RendererGL::BeginPaint(int render_target_w, int render_target_h) {
#ifdef TB_RUNTIME_DEBUG_INFO
  dbg_bitmap_validations = 0;
#endif

  RendererBatcher::BeginPaint(render_target_w, render_target_h);

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
  RendererBatcher::EndPaint();

#ifdef TB_RUNTIME_DEBUG_INFO
  if (TB_DEBUG_SETTING(util::DebugInfo::Setting::kDrawRenderBatches))
    TBDebugOut("Frame caused %d bitmap validations.\n", dbg_bitmap_validations);
#endif  // TB_RUNTIME_DEBUG_INFO
}

Bitmap* RendererGL::CreateBitmap(int width, int height, uint32_t* data) {
  BitmapGL* bitmap = new BitmapGL(this);
  if (!bitmap->Init(width, height, data)) {
    delete bitmap;
    return nullptr;
  }
  return bitmap;
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

void RendererGL::SetClipRect(const Rect& rect) {
  glScissor(m_clip_rect.x, m_screen_rect.h - (m_clip_rect.y + m_clip_rect.h),
            m_clip_rect.w, m_clip_rect.h);
}

}  // namespace tb
