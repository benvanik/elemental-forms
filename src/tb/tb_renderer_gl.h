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

class TBRendererGL;

class TBBitmapGL : public TBBitmap {
 public:
  TBBitmapGL(TBRendererGL* renderer);
  ~TBBitmapGL();
  bool Init(int width, int height, uint32_t* data);
  virtual int Width() { return m_w; }
  virtual int Height() { return m_h; }
  virtual void SetData(uint32_t* data);

 public:
  TBRendererGL* m_renderer;
  int m_w, m_h;
  GLuint m_texture;
};

class TBRendererGL : public TBRendererBatcher {
 public:
  TBRendererGL();

  // == TBRenderer
  // ====================================================================

  virtual void BeginPaint(int render_target_w, int render_target_h);
  virtual void EndPaint();

  virtual TBBitmap* CreateBitmap(int width, int height, uint32_t* data);

  // == TBRendererBatcher
  // ===============================================================

  virtual void RenderBatch(Batch* batch);
  virtual void SetClipRect(const TBRect& rect);

 public:
};

}  // namespace tb

#endif  // TB_RENDERER_GL_H
