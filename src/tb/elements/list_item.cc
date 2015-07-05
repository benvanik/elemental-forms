/**
 ******************************************************************************
 * xenia-project/turbobadger : a fork of Turbo Badger for Xenia               *
 ******************************************************************************
 * Copyright 2011-2015 Emil Segerås and Ben Vanik. All rights reserved.       *
 * See turbo_badger.h and LICENSE in the root for more information.           *
 ******************************************************************************
 */

#include "tb/elements/icon_box.h"
#include "tb/elements/layout_box.h"
#include "tb/elements/list_item.h"
#include "tb/elements/menu_window.h"
#include "tb/elements/separator.h"
#include "tb/parsing/parse_node.h"
#include "tb/util/debug.h"
#include "tb/util/string.h"

namespace tb {
namespace elements {

// SimpleBoxItemElement is a item containing a layout with the following:
// - IconBox showing the item image.
// - Label showing the item string.
// - IconBox showing the arrow for items with a submenu.
// It also handles submenu events.
class SimpleBoxItemElement : public LayoutBox, private ElementListener {
 public:
  SimpleBoxItemElement(TBID image, ListItemSource* source, const char* str);
  ~SimpleBoxItemElement() override;

  bool OnEvent(const ElementEvent& ev) override;

 private:
  void OnElementDelete(Element* element) override;
  void OpenSubMenu();
  void CloseSubMenu();

  ListItemSource* m_source = nullptr;
  Label m_textfield;
  IconBox m_image;
  IconBox m_image_arrow;
  MenuWindow* m_menu = nullptr;  // Points to the submenu window if opened.
};

SimpleBoxItemElement::SimpleBoxItemElement(TBID image, ListItemSource* source,
                                           const char* str)
    : m_source(source) {
  SetSkinBg(TBIDC("ListItem"));
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

SimpleBoxItemElement::~SimpleBoxItemElement() {
  if (m_image_arrow.GetParent()) {
    RemoveChild(&m_image_arrow);
  }
  RemoveChild(&m_textfield);
  if (m_image.GetParent()) {
    RemoveChild(&m_image);
  }
  CloseSubMenu();
}

bool SimpleBoxItemElement::OnEvent(const ElementEvent& ev) {
  if (m_source && ev.type == EventType::kClick && ev.target == this) {
    OpenSubMenu();
    return true;
  }
  return false;
}

void SimpleBoxItemElement::OnElementDelete(Element* element) {
  assert(element == m_menu);
  CloseSubMenu();
}

void SimpleBoxItemElement::OpenSubMenu() {
  if (m_menu) return;

  // Open a new menu window for the submenu with this element as target.
  m_menu = new MenuWindow(this, TBIDC("submenu"));
  SetState(resources::SkinState::kSelected, true);
  m_menu->AddListener(this);
  m_menu->Show(m_source, PopupAlignment(Align::kRight), -1);
}

void SimpleBoxItemElement::CloseSubMenu() {
  if (!m_menu) return;

  SetState(resources::SkinState::kSelected, false);
  m_menu->RemoveListener(this);
  if (!m_menu->GetIsDying()) {
    m_menu->Close();
  }
  m_menu = nullptr;
}

void ListItemObserver::SetSource(ListItemSource* source) {
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

ListItemSource::~ListItemSource() {
  // If this assert trig, you are deleting a model that's still set on some
  // Select element. That might be dangerous.
  assert(!m_observers.HasLinks());
}

bool ListItemSource::Filter(size_t index, const std::string& filter) {
  const char* str = GetItemString(index);
  if (str && util::stristr(str, filter.c_str())) {
    return true;
  }
  return false;
}

Element* ListItemSource::CreateItemElement(size_t index,
                                           ListItemObserver* observer) {
  const char* string = GetItemString(index);
  ListItemSource* sub_source = GetItemSubSource(index);
  TBID image = GetItemImage(index);
  if (sub_source || image) {
    auto itemelement = new SimpleBoxItemElement(image, sub_source, string);
    return itemelement;
  } else if (string && *string == '-') {
    Separator* separator = new Separator();
    separator->SetGravity(Gravity::kAll);
    separator->SetSkinBg(TBIDC("ListItem.separator"));
    return separator;
  } else {
    Label* textfield = new Label();
    textfield->SetSkinBg("ListItem");
    textfield->SetText(string);
    textfield->SetTextAlign(TextAlign::kLeft);
    return textfield;
  }
  return nullptr;
}

void ListItemSource::InvokeItemChanged(size_t index,
                                       ListItemObserver* exclude_observer) {
  auto iter = m_observers.IterateForward();
  while (ListItemObserver* observer = iter.GetAndStep()) {
    if (observer != exclude_observer) {
      observer->OnItemChanged(index);
    }
  }
}

void ListItemSource::InvokeItemAdded(size_t index) {
  auto iter = m_observers.IterateForward();
  while (ListItemObserver* observer = iter.GetAndStep()) {
    observer->OnItemAdded(index);
  }
}

void ListItemSource::InvokeItemRemoved(size_t index) {
  auto iter = m_observers.IterateForward();
  while (ListItemObserver* observer = iter.GetAndStep()) {
    observer->OnItemRemoved(index);
  }
}

void ListItemSource::InvokeAllItemsRemoved() {
  auto iter = m_observers.IterateForward();
  while (ListItemObserver* observer = iter.GetAndStep()) {
    observer->OnAllItemsRemoved();
  }
}

void GenericStringItemSource::ReadItemNodes(
    tb::parsing::ParseNode* parent_node,
    GenericStringItemSource* target_source) {
  // If there is a items node, loop through all its children and add
  // items to the target item source.
  auto items_node = parent_node->GetNode("items");
  if (!items_node) {
    return;
  }
  for (auto node = items_node->GetFirstChild(); node; node = node->GetNext()) {
    if (std::strcmp(node->GetName(), "item") != 0) {
      continue;
    }
    const char* text = node->GetValueString("text", "");
    TBID item_id;
    if (auto id_node = node->GetNode("id")) {
      Element::SetIdFromNode(item_id, id_node);
    }
    auto item = std::make_unique<GenericStringItem>(text, item_id);
    target_source->AddItem(std::move(item));
  }
}

}  // namespace elements
}  // namespace tb
