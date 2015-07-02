/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_RENDERER_GL_H
#define TB_RENDERER_GL_H

#include <cstdint>

#include "tb_renderer_batcher.h"

#ifdef TB_RENDERER_GLES_1
#include <EGL/egl.h>
#include <GLES/gl.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>  // make gl.h compile
#include <GL/gl.h>
#elif defined(MACOSX)
#include <OpenGL/gl.h>
#elif defined(ANDROID)
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif

namespace tb {

class RendererGL;

class BitmapGL : public Bitmap {
 public:
  BitmapGL(RendererGL* renderer);
  ~BitmapGL() override;

  bool Init(int width, int height, uint32_t* data);
  int Width() override { return m_w; }
  int Height() override { return m_h; }
  void SetData(uint32_t* data) override;

 public:
  RendererGL* m_renderer = nullptr;
  int m_w = 0, m_h = 0;
  GLuint m_texture = 0;
};

class RendererGL : public RendererBatcher {
 public:
  RendererGL();

  void BeginPaint(int render_target_w, int render_target_h) override;
  void EndPaint() override;

  Bitmap* CreateBitmap(int width, int height, uint32_t* data) override;

  void RenderBatch(Batch* batch) override;
  void SetClipRect(const Rect& rect) override;
};

}  // namespace tb

#endif  // TB_RENDERER_GL_H
