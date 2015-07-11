/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef EL_ELEMENTS_SCROLL_CONTAINER_H_
#define EL_ELEMENTS_SCROLL_CONTAINER_H_

#include "el/element.h"
#include "el/elements/scroll_bar.h"

namespace el {
namespace elements {

enum class ScrollMode {
  kXY,          // X and Y always; scroll-mode: xy
  kY,           // Y always (X never); scroll-mode: y
  kAutoY,       // Y auto (X never); scroll-mode: y-auto
  kAutoXAutoY,  // X auto, Y auto; scroll-mode: auto
  kOff,         // X and Y never; scroll-mode: off
};
MAKE_ORDERED_ENUM_STRING_UTILS(ScrollMode, "xy", "y", "y-auto", "auto", "off");

// A container with scrollbars that can scroll its children.
class ScrollContainer : public Element {
 public:
  TBOBJECT_SUBCLASS(ScrollContainer, Element);
  static void RegisterInflater();

  ScrollContainer();
  ~ScrollContainer() override;

  bool is_adapting_to_content_size() { return m_adapt_to_content_size; }
  // Sets to true if the preferred size of this container should adapt to the
  // preferred size of the content. This is disabled by default.
  void set_adapt_to_content_size(bool adapt);

  bool is_adapting_content_size() { return m_adapt_content_size; }
  // Sets to true if the content should adapt to the available size of this
  // container when it's larger than the preferred size.
  void set_adapt_content_size(bool adapt);

  ScrollMode scroll_mode() { return m_mode; }
  void set_scroll_mode(ScrollMode mode);

  void ScrollTo(int x, int y) override;
  Element::ScrollInfo scroll_info() override;
  Element* scroll_root() override { return &m_root; }

  void InvalidateLayout(InvalidationMode il) override;

  Rect padding_rect() override;
  PreferredSize OnCalculatePreferredContentSize(
      const SizeConstraints& constraints) override;

  void OnInflate(const parsing::InflateInfo& info) override;
  bool OnEvent(const Event& ev) override;
  void OnProcess() override;
  void OnResized(int old_w, int old_h) override;

  Element* content_root() override { return &m_root; }

 protected:
  class ScrollContainerRoot : public Element {
   public:
    void OnPaintChildren(const PaintProps& paint_props) override;
    void GetChildTranslation(int& x, int& y) const override;
  };

  void ValidateLayout(const SizeConstraints& constraints);

  ScrollBar m_scrollbar_x;
  ScrollBar m_scrollbar_y;
  ScrollContainerRoot m_root;
  bool m_adapt_to_content_size = false;
  bool m_adapt_content_size = false;
  bool m_layout_is_invalid = false;
  ScrollMode m_mode = ScrollMode::kXY;
};

}  // namespace elements
namespace dsl {

using el::elements::ScrollMode;

struct ScrollContainerNode : public ElementNode<ScrollContainerNode> {
  using R = ScrollContainerNode;
  ScrollContainerNode(std::vector<Node> children = {})
      : ElementNode("ScrollContainer", {}, std::move(children)) {}
  //
  R& adapt_content(bool value) {
    set("adapt-content", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& adapt_to_content(bool value) {
    set("adapt-to-content", value ? 1 : 0);
    return *reinterpret_cast<R*>(this);
  }
  //
  R& scroll_mode(ScrollMode value) {
    set("scroll-mode", el::elements::to_string(value));
    return *reinterpret_cast<R*>(this);
  }
};

}  // namespace dsl
}  // namespace el

#endif  // EL_ELEMENTS_SCROLL_CONTAINER_H_
