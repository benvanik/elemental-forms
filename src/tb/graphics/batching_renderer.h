/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#ifndef TB_GRAPHICS_BATCHING_RENDERER_H_
#define TB_GRAPHICS_BATCHING_RENDERER_H_

#include "tb/graphics/renderer.h"

namespace tb {
namespace graphics {

class BitmapFragment;

// A helper class that implements batching of draw operations for a Renderer.
// If you do not want to do your own batching you can subclass this class
// instead of Renderer.
// If overriding any function in this class, make sure to call the base class
// too.
class BatchingRenderer : public Renderer {
 public:
  static const size_t kVertexBatchSize = 6 * 2048;

  // Vertex stored in a Batch.
  struct Vertex {
    float x, y;
    float u, v;
    union {
      struct {
        unsigned char r, g, b, a;
      };
      uint32_t col;
    };
  };

  // A batch which should be rendered.
  class Batch {
   public:
    Batch() = default;
    void Flush(BatchingRenderer* batch_renderer);
    Vertex* Reserve(BatchingRenderer* batch_renderer, int count);

    Vertex vertex[kVertexBatchSize];
    int vertex_count = 0;

    Bitmap* bitmap = nullptr;
    BitmapFragment* fragment = nullptr;

    uint32_t batch_id = 0;
    bool is_flushing = false;
  };

  BatchingRenderer();
  ~BatchingRenderer() override;

  void BeginPaint(int render_target_w, int render_target_h) override;
  void EndPaint() override;

  void Translate(int dx, int dy) override;

  float opacity() override;
  void set_opacity(float opacity) override;

  Rect clip_rect() override;
  Rect set_clip_rect(const Rect& rect, bool add_to_current) override;

  void DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                  BitmapFragment* bitmap_fragment) override;
  void DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                  Bitmap* bitmap) override;
  void DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                         const Color& color,
                         BitmapFragment* bitmap_fragment) override;
  void DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                         const Color& color, Bitmap* bitmap) override;
  void DrawBitmapTile(const Rect& dst_rect, Bitmap* bitmap) override;
  void DrawRect(const Rect& dst_rect, const Color& color) override;
  void DrawRectFill(const Rect& dst_rect, const Color& color) override;
  void FlushBitmap(Bitmap* bitmap);
  void FlushBitmapFragment(BitmapFragment* bitmap_fragment) override;

  void BeginBatchHint(Renderer::BatchHint hint) override {}
  void EndBatchHint() override {}

  virtual std::unique_ptr<Bitmap> CreateBitmap(int width, int height,
                                               uint32_t* data) = 0;
  virtual void RenderBatch(Batch* batch) = 0;
  virtual void set_clip_rect(const Rect& rect) = 0;

 protected:
  void AddQuadInternal(const Rect& dst_rect, const Rect& src_rect,
                       uint32_t color, Bitmap* bitmap,
                       BitmapFragment* fragment);
  void FlushAllInternal();

  uint8_t opacity_ = 255;
  Rect screen_rect_;
  Rect clip_rect_;
  int translation_x_ = 0;
  int translation_y_ = 0;

  Batch batch_;
  float m_u = 0, m_v = 0, m_uu = 0, m_vv = 0;
};

}  // namespace graphics
}  // namespace tb

#endif  // TB_GRAPHICS_BATCHING_RENDERER_H_
