#include "ListWindow.h"

#include "tb/elements/check_box.h"
#include "tb/elements/list_box.h"
#include "tb/util/string.h"

using namespace tb::elements;

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

  content_root()->LoadFile("Demo/demo01/ui_resources/test_list_item.tb.txt");
  CheckBox* checkbox = GetElementByIdAndType<CheckBox>(TBIDC("check"));
  Label* name = GetElementByIdAndType<Label>(TBIDC("name"));
  Label* info = GetElementByIdAndType<Label>(TBIDC("info"));
  checkbox->set_value(item->GetChecked() ? true : false);
  name->set_text(item->str);
  info->set_text(item->GetMale() ? "Male" : "Female");
}

bool AdvancedItemElement::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick && ev.target->id() == TBIDC("check")) {
    AdvancedItem* item = m_source->GetItem(m_index);
    item->SetChecked(ev.target->value() ? true : false);

    m_source->InvokeItemChanged(m_index, m_source_viewer);
    return true;
  } else if (ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("delete")) {
    m_source->DeleteItem(m_index);
    return true;
  }
  return LayoutBox::OnEvent(ev);
}

// == AdvancedItemSource ======================================================

bool AdvancedItemSource::Filter(size_t index, const std::string& filter) {
  // Override this method so we can return hits for our extra data too.

  if (ListItemSource::Filter(index, filter)) return true;

  AdvancedItem* item = GetItem(index);
  return tb::util::stristr(item->GetMale() ? "Male" : "Female", filter.c_str())
             ? true
             : false;
}

Element* AdvancedItemSource::CreateItemElement(size_t index,
                                               ListItemObserver* viewer) {
  auto layout = new AdvancedItemElement(GetItem(index), this, viewer, index);
  return layout;
}

// == ListWindow ==============================================================

ListWindow::ListWindow(ListItemSource* source) {
  LoadResourceFile("Demo/demo01/ui_resources/test_select.tb.txt");
  if (ListBox* select = GetElementByIdAndType<ListBox>("list")) {
    select->set_source(source);
    select->scroll_container()->set_scroll_mode(ScrollMode::kAutoY);
  }
}

bool ListWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kChanged && ev.target->id() == TBIDC("filter")) {
    ListBox* select = GetElementByIdAndType<ListBox>("list");
    select->set_filter(ev.target->text());
    return true;
  }
  return DemoWindow::OnEvent(ev);
}

// == AdvancedListWindow
// ==============================================================

AdvancedListWindow::AdvancedListWindow(AdvancedItemSource* source)
    : m_source(source) {
  LoadResourceFile("Demo/demo01/ui_resources/test_select_advanced.tb.txt");
  if (ListBox* select = GetElementByIdAndType<ListBox>("list")) {
    select->set_source(source);
    select->scroll_container()->set_scroll_mode(ScrollMode::kAutoXAutoY);
  }
}

bool AdvancedListWindow::OnEvent(const ElementEvent& ev) {
  ListBox* select = GetElementByIdAndType<ListBox>("list");
  if (select && ev.type == EventType::kChanged &&
      ev.target->id() == TBIDC("filter")) {
    select->set_filter(ev.target->text());
    return true;
  } else if (select && ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("add")) {
    std::string name = GetTextById(TBIDC("add_name"));
    if (!name.empty()) {
      m_source->AddItem(std::make_unique<AdvancedItem>(
          name.c_str(), TBIDC("boy_item"), true));
    }
    return true;
  } else if (select && ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("delete all")) {
    m_source->DeleteAllItems();
    return true;
  }
  return DemoWindow::OnEvent(ev);
}
