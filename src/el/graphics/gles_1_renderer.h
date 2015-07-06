/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_GRAPHICS_GLES_1_RENDERER_H_
#define EL_GRAPHICS_GLES_1_RENDERER_H_

#include <cstdint>

#include "el/graphics/batching_renderer.h"

#ifdef EL_RENDERER_GLES_1
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

namespace el {
namespace graphics {

class RendererGL;

class BitmapGL : public Bitmap {
 public:
  BitmapGL(RendererGL* renderer);
  ~BitmapGL() override;

  bool Init(int width, int height, uint32_t* data);
  int width() override { return m_w; }
  int height() override { return m_h; }
  void set_data(uint32_t* data) override;

 public:
  RendererGL* m_renderer = nullptr;
  int m_w = 0, m_h = 0;
  GLuint m_texture = 0;
};

class RendererGL : public BatchingRenderer {
 public:
  RendererGL();

  void BeginPaint(int render_target_w, int render_target_h) override;
  void EndPaint() override;

  std::unique_ptr<Bitmap> CreateBitmap(int width, int height,
                                       uint32_t* data) override;

  void RenderBatch(Batch* batch) override;
  void set_clip_rect(const Rect& rect) override;
};

}  // namespace graphics
}  // namespace el

#endif  // EL_GRAPHICS_GLES_1_RENDERER_H_
