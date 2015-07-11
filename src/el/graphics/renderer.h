/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_GRAPHICS_RENDERER_H_
#define EL_GRAPHICS_RENDERER_H_

#include <cstdint>
#include <memory>

#include "el/color.h"
#include "el/rect.h"
#include "el/util/intrusive_list.h"

namespace el {
namespace graphics {

class BitmapFragment;

// RendererListener is a listener for Renderer.
class RendererListener : public util::IntrusiveListEntry<RendererListener> {
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

  virtual int width() = 0;
  virtual int height() = 0;

  // Updates the bitmap with the given data (in BGRA32 format).
  // NOTE: Implementations for batched renderers should call
  // Renderer::FlushBitmap to make sure any active batch is being flushed
  // before the bitmap is changed.
  virtual void set_data(uint32_t* data) = 0;
};

// A batching interface for drawing performed by the library.
//
// If overriding any function in this class, make sure to call the base class
// too.
class Renderer {
 public:
  static Renderer* get() { return renderer_singleton_; }
  static void set(Renderer* value) { renderer_singleton_ = value; }

  virtual ~Renderer();

  // Should be called before invoking paint on any element.
  // render_target_w and render_target_h should be the size of the render target
  // that the renderer renders to. I.e window size, screen size or frame buffer
  // object.
  virtual void BeginPaint(int render_target_w, int render_target_h);
  virtual void EndPaint();

  // Translates all drawing with the given offset.
  void Translate(int dx, int dy);

  float opacity();
  // Sets the current opacity that should apply to all drawing (0.f-1.f).
  void set_opacity(float opacity);

  // Gets the current clip rect. Note: This may be different from the rect sent
  // to set_clip_rect, due to intersecting with the previous cliprect!
  Rect clip_rect();

  // Sets a clip rect to the renderer. add_to_current should be true when
  // pushing a new cliprect that should clip inside the last clip rect, and
  // false when restoring. It will return the clip rect that was in use before
  // this call.
  Rect set_clip_rect(const Rect& rect, bool add_to_current);

  // Draws the src_rect part of the fragment stretched to dst_rect.
  // dst_rect or src_rect can have negative width and height to achieve
  // horizontal and vertical flip.
  void DrawBitmap(const Rect& dst_rect, const Rect& src_rect,
                  BitmapFragment* bitmap_fragment);

  // Draws the src_rect part of the bitmap stretched to dst_rect.
  // dst_rect or src_rect can have negative width and height to achieve
  // horizontal and vertical flip.
  void DrawBitmap(const Rect& dst_rect, const Rect& src_rect, Bitmap* bitmap);

  // Draws the src_rect part of the fragment stretched to dst_rect.
  // The bitmap will be used as a mask for the color.
  // dst_rect or src_rect can have negative width and height to achieve
  // horizontal and vertical flip.
  void DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                         const Color& color, BitmapFragment* bitmap_fragment);

  // Draws the src_rect part of the bitmap stretched to dst_rect.
  // The bitmap will be used as a mask for the color.
  // dst_rect or src_rect can have negative width and height to achieve
  // horizontal and vertical flip.
  void DrawBitmapColored(const Rect& dst_rect, const Rect& src_rect,
                         const Color& color, Bitmap* bitmap);

  // Draws the bitmap tiled into dst_rect.
  void DrawBitmapTile(const Rect& dst_rect, Bitmap* bitmap);

  // Draws a 1px thick rectangle outline.
  void DrawRect(const Rect& dst_rect, const Color& color);

  // Draws a filled rectangle.
  void DrawRectFill(const Rect& dst_rect, const Color& color);

  // Makes sure the given bitmap fragment is flushed from any batching, because
  // it may be changed or deleted after this call.
  void FlushBitmapFragment(BitmapFragment* bitmap_fragment);

  // Creates a new Bitmap from the given data (in BGRA32 format).
  // Width and height must be a power of two.
  // Returns nullptr if fail.
  virtual std::unique_ptr<Bitmap> CreateBitmap(int width, int height,
                                               uint32_t* data) = 0;

  // Adds a listener to this renderer.
  // Does not take ownership.
  void AddListener(RendererListener* listener) { listeners_.AddLast(listener); }

  // Removes a listener from this renderer.
  void RemoveListener(RendererListener* listener) {
    listeners_.Remove(listener);
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

 protected:
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
  struct Batch {
    Vertex* vertices = nullptr;
    size_t vertex_count = 0;

    Bitmap* bitmap = nullptr;
    BitmapFragment* fragment = nullptr;

    uint32_t batch_id = 0;
    bool is_flushing = false;
  };

  Renderer();

  // Returns the maximum number of vertices that may fit within a single batch.
  virtual size_t max_vertex_batch_size() const = 0;

  // Flushes the current batch.
  void FlushBatch();
  // Flushes the current batch if the given bitmap is used.
  void FlushBitmap(Bitmap* bitmap);
  // Reserves a block of vertices for use within the current batch.
  // The batch will be flushed if the request cannot be satisfied.
  Vertex* ReserveVertices(size_t vertex_count);

  // Renders the given batch with the renderer implementation.
  virtual void RenderBatch(Batch* batch) = 0;
  // Sets the clipping rectangle used when rendering.
  virtual void set_clip_rect(const Rect& rect) = 0;

  void AddQuadInternal(const Rect& dst_rect, const Rect& src_rect,
                       uint32_t color, Bitmap* bitmap,
                       BitmapFragment* fragment);
  void FlushAllInternal();

  static Renderer* renderer_singleton_;

  util::IntrusiveList<RendererListener> listeners_;

  uint8_t opacity_ = 255;
  Rect screen_rect_;
  Rect clip_rect_;
  int translation_x_ = 0;
  int translation_y_ = 0;

  Batch batch_;
  float m_u = 0, m_v = 0, m_uu = 0, m_vv = 0;

  size_t begin_paint_batch_id_ = 0;
  size_t frame_triangle_count_ = 0;
};

}  // namespace graphics
}  // namespace el

#endif  // EL_GRAPHICS_RENDERER_H_
