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

/** TBRendererBatcher is a helper class that implements batching of draw
   operations for a TBRenderer.
        If you do not want to do your own batching you can subclass this class
   instead of TBRenderer.
        If overriding any function in this class, make sure to call the base
   class too. */
class TBRendererBatcher : public TBRenderer {
 public:
  /** Vertex stored in a Batch */
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
  /** A batch which should be rendered. */
  class Batch {
   public:
    Batch()
        : vertex_count(0),
          bitmap(nullptr),
          fragment(nullptr),
          batch_id(0),
          is_flushing(false) {}
    void Flush(TBRendererBatcher* batch_renderer);
    Vertex* Reserve(TBRendererBatcher* batch_renderer, int count);

    Vertex vertex[VERTEX_BATCH_SIZE];
    int vertex_count;

    TBBitmap* bitmap;
    TBBitmapFragment* fragment;

    uint32_t batch_id;
    bool is_flushing;
  };

  TBRendererBatcher();
  virtual ~TBRendererBatcher();

  virtual void BeginPaint(int render_target_w, int render_target_h);
  virtual void EndPaint();

  virtual void Translate(int dx, int dy);

  virtual void SetOpacity(float opacity);
  virtual float GetOpacity();

  virtual Rect SetClipRect(const Rect& rect, bool add_to_current);
  virtual Rect GetClipRect();

  virtual void DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                          TBBitmapFragment* bitmap_fragment);
  virtual void DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                          TBBitmap* bitmap);
  virtual void DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                                 const Color& color,
                                 TBBitmapFragment* bitmap_fragment);
  virtual void DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                                 const Color& color, TBBitmap* bitmap);
  virtual void DrawBitmapTile(const Rect& dst_rect, TBBitmap* bitmap);
  virtual void DrawRect(const Rect& dst_rect, const Color& color);
  virtual void DrawRectFill(const Rect& dst_rect, const Color& color);
  virtual void FlushBitmap(TBBitmap* bitmap);
  virtual void FlushBitmapFragment(TBBitmapFragment* bitmap_fragment);

  virtual void BeginBatchHint(TBRenderer::BatchHint hint) {}
  virtual void EndBatchHint() {}

  virtual TBBitmap* CreateBitmap(int width, int height, uint32_t* data) = 0;
  virtual void RenderBatch(Batch* batch) = 0;
  virtual void SetClipRect(const Rect& rect) = 0;

 protected:
  uint8_t m_opacity;
  Rect m_screen_rect;
  Rect m_clip_rect;
  int m_translation_x;
  int m_translation_y;

  float m_u, m_v, m_uu, m_vv;  ///< Some temp variables
  Batch batch;  ///< The one and only batch. this should be improved.

  void AddQuadInternal(const Rect& dst_rect, const Rect& src_rect,
                       uint32_t color, TBBitmap* bitmap,
                       TBBitmapFragment* fragment);
  void FlushAllInternal();
};

}  // namespace tb

#endif  // TB_RENDERER_BATCHER_H
