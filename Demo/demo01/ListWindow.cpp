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
  SetSkinBg(TBIDC("ListItem"));
  SetLayoutDistribution(LayoutDistribution::kGravity);
  SetLayoutDistributionPosition(LayoutDistributionPosition::kLeftTop);
  SetPaintOverflowFadeout(false);

  GetContentRoot()->LoadFile("Demo/demo01/ui_resources/test_list_item.tb.txt");
  CheckBox* checkbox = GetElementByIDAndType<CheckBox>(TBIDC("check"));
  Label* name = GetElementByIDAndType<Label>(TBIDC("name"));
  Label* info = GetElementByIDAndType<Label>(TBIDC("info"));
  checkbox->SetValue(item->GetChecked() ? true : false);
  name->SetText(item->str);
  info->SetText(item->GetMale() ? "Male" : "Female");
}

bool AdvancedItemElement::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick && ev.target->id() == TBIDC("check")) {
    AdvancedItem* item = m_source->GetItem(m_index);
    item->SetChecked(ev.target->GetValue() ? true : false);

    m_source->InvokeItemChanged(m_index, m_source_viewer);
    return true;
  } else if (ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("delete")) {
    m_source->DeleteItem(m_index);
    return true;
  }
  return Layout::OnEvent(ev);
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
  if (ListBox* select = GetElementByIDAndType<ListBox>("list")) {
    select->SetSource(source);
    select->GetScrollContainer()->SetScrollMode(ScrollMode::kAutoY);
  }
}

bool ListWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kChanged && ev.target->id() == TBIDC("filter")) {
    ListBox* select = GetElementByIDAndType<ListBox>("list");
    select->SetFilter(ev.target->GetText());
    return true;
  }
  return DemoWindow::OnEvent(ev);
}

// == AdvancedListWindow
// ==============================================================

AdvancedListWindow::AdvancedListWindow(AdvancedItemSource* source)
    : m_source(source) {
  LoadResourceFile("Demo/demo01/ui_resources/test_select_advanced.tb.txt");
  if (ListBox* select = GetElementByIDAndType<ListBox>("list")) {
    select->SetSource(source);
    select->GetScrollContainer()->SetScrollMode(ScrollMode::kAutoXAutoY);
  }
}

bool AdvancedListWindow::OnEvent(const ElementEvent& ev) {
  ListBox* select = GetElementByIDAndType<ListBox>("list");
  if (select && ev.type == EventType::kChanged &&
      ev.target->id() == TBIDC("filter")) {
    select->SetFilter(ev.target->GetText());
    return true;
  } else if (select && ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("add")) {
    std::string name = GetTextByID(TBIDC("add_name"));
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
