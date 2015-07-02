/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_RENDERER_BATCHER_H
#define TB_RENDERER_BATCHER_H

#include "tb_renderer.h"

namespace tb {

#define VERTEX_BATCH_SIZE 6 * 2048

// A helper class that implements batching of draw operations for a Renderer.
// If you do not want to do your own batching you can subclass this class
// instead of Renderer.
// If overriding any function in this class, make sure to call the base class
// too.
class RendererBatcher : public Renderer {
 public:
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
    void Flush(RendererBatcher* batch_renderer);
    Vertex* Reserve(RendererBatcher* batch_renderer, int count);

    Vertex vertex[VERTEX_BATCH_SIZE];
    int vertex_count = 0;

    Bitmap* bitmap = nullptr;
    BitmapFragment* fragment = nullptr;

    uint32_t batch_id = 0;
    bool is_flushing = false;
  };

  RendererBatcher();
  ~RendererBatcher() override;

  void BeginPaint(int render_target_w, int render_target_h) override;
  void EndPaint() override;

  void Translate(int dx, int dy) override;

  void SetOpacity(float opacity) override;
  float GetOpacity() override;

  Rect SetClipRect(const Rect& rect, bool add_to_current) override;
  Rect GetClipRect() override;

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

  virtual Bitmap* CreateBitmap(int width, int height, uint32_t* data) = 0;
  virtual void RenderBatch(Batch* batch) = 0;
  virtual void SetClipRect(const Rect& rect) = 0;

 protected:
  void AddQuadInternal(const Rect& dst_rect, const Rect& src_rect,
                       uint32_t color, Bitmap* bitmap,
                       BitmapFragment* fragment);
  void FlushAllInternal();

  uint8_t m_opacity = 255;
  Rect m_screen_rect;
  Rect m_clip_rect;
  int m_translation_x = 0;
  int m_translation_y = 0;

  float m_u = 0, m_v = 0, m_uu = 0, m_vv = 0;
  Batch batch;
};

}  // namespace tb

#endif  // TB_RENDERER_BATCHER_H
