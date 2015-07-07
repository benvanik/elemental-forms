/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/elements/check_box.h"
#include "el/elements/list_box.h"
#include "el/elements/text_box.h"
#include "el/util/string.h"
#include "testbed/list_window.h"

namespace testbed {

using namespace el;

// == AdvancedItemElement ======================================================

AdvancedItemElement::AdvancedItemElement(AdvancedItem* item,
                                         AdvancedItemSource* source,
                                         ListItemObserver* source_viewer,
                                         size_t index)
    : m_source(source), m_source_viewer(source_viewer), m_index(index) {
  set_background_skin(TBIDC("ListItem"));
  set_layout_distribution(LayoutDistribution::kGravity);
  set_layout_distribution_position(LayoutDistributionPosition::kLeftTop);
  set_paint_overflow_fadeout(false);

  content_root()->LoadFile("test_list_item.tb.txt");
  CheckBox* checkbox = GetElementById<CheckBox>(TBIDC("check"));
  Label* name = GetElementById<Label>(TBIDC("name"));
  Label* info = GetElementById<Label>(TBIDC("info"));
  checkbox->set_value(item->GetChecked() ? true : false);
  name->set_text(item->str);
  info->set_text(item->GetMale() ? "Male" : "Female");
}

bool AdvancedItemElement::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick && ev.target->id() == TBIDC("check")) {
    AdvancedItem* item = m_source->at(m_index);
    item->SetChecked(ev.target->value() ? true : false);

    m_source->InvokeItemChanged(m_index, m_source_viewer);
    return true;
  } else if (ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("delete")) {
    m_source->erase(m_index);
    return true;
  }
  return LayoutBox::OnEvent(ev);
}

// == AdvancedItemSource ======================================================

bool AdvancedItemSource::Filter(size_t index, const std::string& filter) {
  // Override this method so we can return hits for our extra data too.

  if (ListItemSource::Filter(index, filter)) return true;

  AdvancedItem* item = at(index);
  return el::util::stristr(item->GetMale() ? "Male" : "Female", filter.c_str())
             ? true
             : false;
}

Element* AdvancedItemSource::CreateItemElement(size_t index,
                                               ListItemObserver* viewer) {
  auto layout = new AdvancedItemElement(at(index), this, viewer, index);
  return layout;
}

// == ListWindow ==============================================================

ListWindow::ListWindow(ListItemSource* source) {
  LoadResourceFile("test_select.tb.txt");
  if (ListBox* select = GetElementById<ListBox>("list")) {
    select->set_source(source);
    select->scroll_container()->set_scroll_mode(ScrollMode::kAutoY);
  }
  auto filter = GetElementById<TextBox>("filter");
  auto select = GetElementById<ListBox>("list");
  eh_.Listen(EventType::kChanged, filter, [this, select](const Event& ev) {
    select->set_filter(ev.target->text());
    return true;
  });
  /*eh_.Listen(EventType::kChanged, TBIDC("filter"), [this](const Event& ev) {
    auto select = GetElementById<ListBox>("list");
    select->set_filter(ev.target->text());
    return true;
  });*/
}

bool ListWindow::OnEvent(const Event& ev) {
  /*if (ev.type == EventType::kChanged && ev.target->id() == TBIDC("filter")) {
    auto select = GetElementById<ListBox>("list");
    select->set_filter(ev.target->text());
    return true;
  }*/
  return DemoWindow::OnEvent(ev);
}

// == AdvancedListWindow
// ==============================================================

AdvancedListWindow::AdvancedListWindow(AdvancedItemSource* source)
    : m_source(source) {
  LoadResourceFile("test_select_advanced.tb.txt");
  if (ListBox* select = GetElementById<ListBox>("list")) {
    select->set_source(source);
    select->scroll_container()->set_scroll_mode(ScrollMode::kAutoXAutoY);
  }
}

bool AdvancedListWindow::OnEvent(const Event& ev) {
  ListBox* select = GetElementById<ListBox>("list");
  if (select && ev.type == EventType::kChanged &&
      ev.target->id() == TBIDC("filter")) {
    select->set_filter(ev.target->text());
    return true;
  } else if (select && ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("add")) {
    std::string name = GetTextById(TBIDC("add_name"));
    if (!name.empty()) {
      m_source->push_back(std::make_unique<AdvancedItem>(
          name.c_str(), TBIDC("boy_item"), true));
    }
    return true;
  } else if (select && ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("delete all")) {
    m_source->clear();
    return true;
  }
  return DemoWindow::OnEvent(ev);
}

}  // namespace testbed
