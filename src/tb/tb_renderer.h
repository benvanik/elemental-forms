/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See tb_core.h and LICENSE in the root for more information.                *
 ******************************************************************************
 */

#ifndef TB_RENDERER_H
#define TB_RENDERER_H

#include <cstdint>

#include "tb_color.h"
#include "tb_core.h"
#include "tb_geometry.h"
#include "tb_linklist.h"

namespace tb {

class BitmapFragment;

// RendererListener is a listener for Renderer.
class RendererListener : public TBLinkOf<RendererListener> {
 public:
  virtual ~RendererListener() = default;

  // Called when the context has been lost and all Bitmaps need to be deleted.
  // NOTE: Only do cleanup here. It's not safe to do work on any bitmap since
  // the context is already lost.
  virtual void OnContextLost() = 0;

  // Called when the context has been restored again, and new Bitmaps can be
  // created again.
  virtual void OnContextRestored() = 0;
};

// A minimal interface for bitmap to be painted by Renderer.
//
// NOTE: Implementations for batched renderers should call
// Renderer::FlushBitmap to make sure any active batch is being flushed before
// the bitmap is deleted.
class Bitmap {
 public:
  virtual ~Bitmap() = default;

  virtual int Width() = 0;
  virtual int Height() = 0;

  // Updates the bitmap with the given data (in BGRA32 format).
  // NOTE: Implementations for batched renderers should call
  // Renderer::FlushBitmap to make sure any active batch is being flushed
  // before the bitmap is changed.
  virtual void SetData(uint32_t* data) = 0;
};

// A minimal interface for painting strings and bitmaps.
class Renderer {
 public:
  virtual ~Renderer() = default;

  // Should be called before invoking paint on any widget.
  // render_target_w and render_target_h should be the size of the render target
  // that the renderer renders to. I.e window size, screen size or frame buffer
  // object.
  virtual void BeginPaint(int render_target_w, int render_target_h) = 0;
  virtual void EndPaint() = 0;

  // Translates all drawing with the given offset.
  virtual void Translate(int dx, int dy) = 0;

  // Sets the current opacity that should apply to all drawing (0.f-1.f).
  virtual void SetOpacity(float opacity) = 0;
  virtual float GetOpacity() = 0;

  // Sets a clip rect to the renderer. add_to_current should be true when
  // pushing a new cliprect that should clip inside the last clip rect, and
  // false when restoring. It will return the clip rect that was in use before
  // this call.
  virtual Rect SetClipRect(const Rect& rect, bool add_to_current) = 0;

  // Gets the current clip rect. Note: This may be different from the rect sent
  // to SetClipRect, due to intersecting with the previous cliprect!
  virtual Rect GetClipRect() = 0;

  // Draws the src_rect part of the fragment stretched to dst_rect.
  // dst_rect or src_rect can have negative width and height to achieve
  // horizontal and vertical flip.
  virtual void DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                          BitmapFragment* bitmap_fragment) = 0;

  // Draws the src_rect part of the bitmap stretched to dst_rect.
  // dst_rect or src_rect can have negative width and height to achieve
  // horizontal and vertical flip.
  virtual void DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                          Bitmap* bitmap) = 0;

  // Draws the src_rect part of the fragment stretched to dst_rect.
  // The bitmap will be used as a mask for the color.
  // dst_rect or src_rect can have negative width and height to achieve
  // horizontal and vertical flip.
  virtual void DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                                 const Color& color,
                                 BitmapFragment* bitmap_fragment) = 0;

  // Draws the src_rect part of the bitmap stretched to dst_rect.
  // The bitmap will be used as a mask for the color.
  // dst_rect or src_rect can have negative width and height to achieve
  // horizontal and vertical flip.
  virtual void DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                                 const Color& color, Bitmap* bitmap) = 0;

  // Draws the bitmap tiled into dst_rect.
  virtual void DrawBitmapTile(const Rect& dst_rect, Bitmap* bitmap) = 0;

  // Draws a 1px thick rectangle outline.
  virtual void DrawRect(const Rect& dst_rect, const Color& color) = 0;

  // Draws a filled rectangle.
  virtual void DrawRectFill(const Rect& dst_rect, const Color& color) = 0;

  // Makes sure the given bitmap fragment is flushed from any batching, because
  // it may be changed or deleted after this call.
  virtual void FlushBitmapFragment(BitmapFragment* bitmap_fragment) = 0;

  // Creates a new Bitmap from the given data (in BGRA32 format).
  // Width and height must be a power of two.
  // Returns nullptr if fail.
  virtual Bitmap* CreateBitmap(int width, int height, uint32_t* data) = 0;

  // Adds a listener to this renderer.
  // Does not take ownership.
  void AddListener(RendererListener* listener) {
    m_listeners.AddLast(listener);
  }

  // Removes a listener from this renderer.
  void RemoveListener(RendererListener* listener) {
    m_listeners.Remove(listener);
  }

  // Invokes OnContextLost on all listeners.
  // Call when bitmaps should be forgotten.
  void InvokeContextLost();

  // Invokes OnContextRestored on all listeners.
  // Call when bitmaps can safely be restored.
  void InvokeContextRestored();

  // Defines the hint given to BeginBatchHint.
  enum class BatchHint {
    // All calls are either DrawBitmap or DrawBitmapColored with the same bitmap
    // fragment.
    kDrawBitmapFragment,
  };

  // A hint to batching renderers that the following set of draw calls are of
  // the same type so batching might be optimized.
  // The hint defines what operations are allowed between BeginBatchHint until
  // EndBatchHint is called. All other draw operations are invalid.
  // It's not valid to nest calls to BeginBatchHint.
  virtual void BeginBatchHint(BatchHint hint) {}

  // Ends the hint scope started with BeginBatchHint.
  virtual void EndBatchHint() {}

 private:
  TBLinkListOf<RendererListener> m_listeners;
};

}  // namespace tb

#endif  // TB_RENDERER_H
