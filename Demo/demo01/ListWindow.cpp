#include "ListWindow.h"

// == AdvancedItemWidget ======================================================

AdvancedItemWidget::AdvancedItemWidget(AdvancedItem* item,
                                       AdvancedItemSource* source,
                                       SelectItemObserver* source_viewer,
                                       int index)
    : m_source(source), m_source_viewer(source_viewer), m_index(index) {
  SetSkinBg(TBIDC("SelectItem"));
  SetLayoutDistribution(LayoutDistribution::kGravity);
  SetLayoutDistributionPosition(LayoutDistributionPosition::kLeftTop);
  SetPaintOverflowFadeout(false);

  g_widgets_reader->LoadFile(GetContentRoot(),
                             "Demo/demo01/ui_resources/test_list_item.tb.txt");
  CheckBox* checkbox = GetWidgetByIDAndType<CheckBox>(TBIDC("check"));
  Label* name = GetWidgetByIDAndType<Label>(TBIDC("name"));
  Label* info = GetWidgetByIDAndType<Label>(TBIDC("info"));
  checkbox->SetValue(item->GetChecked() ? true : false);
  name->SetText(item->str);
  info->SetText(item->GetMale() ? "Male" : "Female");
}

bool AdvancedItemWidget::OnEvent(const TBWidgetEvent& ev) {
  if (ev.type == EventType::kClick && ev.target->GetID() == TBIDC("check")) {
    AdvancedItem* item = m_source->GetItem(m_index);
    item->SetChecked(ev.target->GetValue() ? true : false);

    m_source->InvokeItemChanged(m_index, m_source_viewer);
    return true;
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("delete")) {
    m_source->DeleteItem(m_index);
    return true;
  }
  return Layout::OnEvent(ev);
}

// == AdvancedItemSource ======================================================

bool AdvancedItemSource::Filter(int index, const char* filter) {
  // Override this method so we can return hits for our extra data too.

  if (SelectItemSource::Filter(index, filter)) return true;

  AdvancedItem* item = GetItem(index);
  return stristr(item->GetMale() ? "Male" : "Female", filter) ? true : false;
}

TBWidget* AdvancedItemSource::CreateItemWidget(int index,
                                               SelectItemObserver* viewer) {
  if (Layout* layout =
          new AdvancedItemWidget(GetItem(index), this, viewer, index))
    return layout;
  return nullptr;
}

// == ListWindow ==============================================================

ListWindow::ListWindow(SelectItemSource* source) {
  LoadResourceFile("Demo/demo01/ui_resources/test_select.tb.txt");
  if (SelectList* select = GetWidgetByIDAndType<SelectList>("list")) {
    select->SetSource(source);
    select->GetScrollContainer()->SetScrollMode(ScrollMode::kAutoY);
  }
}

bool ListWindow::OnEvent(const TBWidgetEvent& ev) {
  if (ev.type == EventType::kChanged && ev.target->GetID() == TBIDC("filter")) {
    SelectList* select = GetWidgetByIDAndType<SelectList>("list");
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
  if (SelectList* select = GetWidgetByIDAndType<SelectList>("list")) {
    select->SetSource(source);
    select->GetScrollContainer()->SetScrollMode(ScrollMode::kAutoXAutoY);
  }
}

bool AdvancedListWindow::OnEvent(const TBWidgetEvent& ev) {
  SelectList* select = GetWidgetByIDAndType<SelectList>("list");
  if (select && ev.type == EventType::kChanged &&
      ev.target->GetID() == TBIDC("filter")) {
    select->SetFilter(ev.target->GetText());
    return true;
  } else if (select && ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("add")) {
    std::string name = GetTextByID(TBIDC("add_name"));
    if (!name.empty()) {
      m_source->AddItem(
          new AdvancedItem(name.c_str(), TBIDC("boy_item"), true));
    }
    return true;
  } else if (select && ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("delete all")) {
    m_source->DeleteAllItems();
    return true;
  }
  return DemoWindow::OnEvent(ev);
}
