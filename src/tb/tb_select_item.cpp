/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include <cassert>
#include <cstdlib>

#include "tb_select.h"
#include "tb_select_item.h"

#include "tb/menu_window.h"
#include "tb/util/debug.h"
#include "tb/util/string.h"

namespace tb {

// SimpleLayoutItemElement is a item containing a layout with the following:
// - SkinImage showing the item image.
// - Label showing the item string.
// - SkinImage showing the arrow for items with a submenu.
// It also handles submenu events.
class SimpleLayoutItemElement : public Layout, private ElementListener {
 public:
  SimpleLayoutItemElement(TBID image, SelectItemSource* source,
                          const char* str);
  ~SimpleLayoutItemElement() override;

  bool OnEvent(const ElementEvent& ev) override;

 private:
  void OnElementDelete(Element* element) override;
  void OpenSubMenu();
  void CloseSubMenu();

  SelectItemSource* m_source = nullptr;
  Label m_textfield;
  SkinImage m_image;
  SkinImage m_image_arrow;
  MenuWindow* m_menu = nullptr;  // Points to the submenu window if opened.
};

SimpleLayoutItemElement::SimpleLayoutItemElement(TBID image,
                                                 SelectItemSource* source,
                                                 const char* str)
    : m_source(source) {
  SetSkinBg(TBIDC("SelectItem"));
  SetLayoutDistribution(LayoutDistribution::kAvailable);
  SetPaintOverflowFadeout(false);

  if (image) {
    m_image.SetSkinBg(image);
    m_image.SetIgnoreInput(true);
    AddChild(&m_image);
  }

  m_textfield.SetText(str);
  m_textfield.SetTextAlign(TextAlign::kLeft);
  m_textfield.SetIgnoreInput(true);
  AddChild(&m_textfield);

  if (source) {
    m_image_arrow.SetSkinBg(TBIDC("arrow.right"));
    m_image_arrow.SetIgnoreInput(true);
    AddChild(&m_image_arrow);
  }
}

SimpleLayoutItemElement::~SimpleLayoutItemElement() {
  if (m_image_arrow.GetParent()) {
    RemoveChild(&m_image_arrow);
  }
  RemoveChild(&m_textfield);
  if (m_image.GetParent()) {
    RemoveChild(&m_image);
  }
  CloseSubMenu();
}

bool SimpleLayoutItemElement::OnEvent(const ElementEvent& ev) {
  if (m_source && ev.type == EventType::kClick && ev.target == this) {
    OpenSubMenu();
    return true;
  }
  return false;
}

void SimpleLayoutItemElement::OnElementDelete(Element* element) {
  assert(element == m_menu);
  CloseSubMenu();
}

void SimpleLayoutItemElement::OpenSubMenu() {
  if (m_menu) return;

  // Open a new menu window for the submenu with this element as target.
  m_menu = new MenuWindow(this, TBIDC("submenu"));
  SetState(resources::SkinState::kSelected, true);
  m_menu->AddListener(this);
  m_menu->Show(m_source, PopupAlignment(Align::kRight), -1);
}

void SimpleLayoutItemElement::CloseSubMenu() {
  if (!m_menu) return;

  SetState(resources::SkinState::kSelected, false);
  m_menu->RemoveListener(this);
  if (!m_menu->GetIsDying()) {
    m_menu->Close();
  }
  m_menu = nullptr;
}

void SelectItemObserver::SetSource(SelectItemSource* source) {
  if (m_source == source) return;

  if (m_source) {
    m_source->m_observers.Remove(this);
  }
  m_source = source;
  if (m_source) {
    m_source->m_observers.AddLast(this);
  }

  OnSourceChanged();
}

SelectItemSource::~SelectItemSource() {
  // If this assert trig, you are deleting a model that's still set on some
  // Select element. That might be dangerous.
  assert(!m_observers.HasLinks());
}

bool SelectItemSource::Filter(size_t index, const std::string& filter) {
  const char* str = GetItemString(index);
  if (str && util::stristr(str, filter.c_str())) {
    return true;
  }
  return false;
}

Element* SelectItemSource::CreateItemElement(size_t index,
                                             SelectItemObserver* observer) {
  const char* string = GetItemString(index);
  SelectItemSource* sub_source = GetItemSubSource(index);
  TBID image = GetItemImage(index);
  if (sub_source || image) {
    SimpleLayoutItemElement* itemelement =
        new SimpleLayoutItemElement(image, sub_source, string);
    return itemelement;
  } else if (string && *string == '-') {
    Separator* separator = new Separator();
    separator->SetGravity(Gravity::kAll);
    separator->SetSkinBg(TBIDC("SelectItem.separator"));
    return separator;
  } else {
    Label* textfield = new Label();
    textfield->SetSkinBg("SelectItem");
    textfield->SetText(string);
    textfield->SetTextAlign(TextAlign::kLeft);
    return textfield;
  }
  return nullptr;
}

void SelectItemSource::InvokeItemChanged(size_t index,
                                         SelectItemObserver* exclude_observer) {
  auto iter = m_observers.IterateForward();
  while (SelectItemObserver* observer = iter.GetAndStep()) {
    if (observer != exclude_observer) {
      observer->OnItemChanged(index);
    }
  }
}

void SelectItemSource::InvokeItemAdded(size_t index) {
  auto iter = m_observers.IterateForward();
  while (SelectItemObserver* observer = iter.GetAndStep()) {
    observer->OnItemAdded(index);
  }
}

void SelectItemSource::InvokeItemRemoved(size_t index) {
  auto iter = m_observers.IterateForward();
  while (SelectItemObserver* observer = iter.GetAndStep()) {
    observer->OnItemRemoved(index);
  }
}

void SelectItemSource::InvokeAllItemsRemoved() {
  auto iter = m_observers.IterateForward();
  while (SelectItemObserver* observer = iter.GetAndStep()) {
    observer->OnAllItemsRemoved();
  }
}

}  // namespace tb
