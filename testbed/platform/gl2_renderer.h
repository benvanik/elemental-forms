/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef TESTBED_PLATFORM_GL2_RENDERER_H_
#define TESTBED_PLATFORM_GL2_RENDERER_H_

#include <cstdint>

#include "el/graphics/renderer.h"
#include "glad/glad_gl2.h"

namespace testbed {
namespace platform {

class GL2Renderer : public el::graphics::Renderer {
 public:
  GL2Renderer();
  ~GL2Renderer() override;

  void BeginPaint(int render_target_w, int render_target_h) override;
  void EndPaint() override;

  std::unique_ptr<el::graphics::Bitmap> CreateBitmap(int width, int height,
                                                     uint32_t* data) override;

 protected:
  class GL2Bitmap : public el::graphics::Bitmap {
   public:
    GL2Bitmap(GL2Renderer* renderer);
    ~GL2Bitmap() override;

    bool Init(int width, int height, uint32_t* data);
    int width() override { return width_; }
    int height() override { return height_; }
    void set_data(uint32_t* data) override;

   public:
    GL2Renderer* renderer_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    GLuint handle_ = 0;
  };

  static const uint32_t kMaxVertexBatchSize = 6 * 2048;

  size_t max_vertex_batch_size() const override { return kMaxVertexBatchSize; }
  void RenderBatch(Batch* batch) override;
  void set_clip_rect(const el::Rect& rect) override;

  void BindBitmap(el::graphics::Bitmap* bitmap);

  GLuint program_ = 0;
  GLuint projection_matrix_loc_ = 0;
  GLuint texture_sampler_loc_ = 0;
  GLuint texture_mix_loc_ = 0;

  GLuint current_texture_ = 0;
  Vertex vertices_[kMaxVertexBatchSize];

  size_t bitmap_validations_ = 0;
};

}  // namespace platform
}  // namespace testbed

#endif  // TESTBED_PLATFORM_GL2_RENDERER_H_
