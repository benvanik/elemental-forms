#ifndef LISTWINDOW_DEMO_H
#define LISTWINDOW_DEMO_H

#include "Demo.h"
#include "tb/event_handler.h"
#include "tb/list_item.h"

class AdvancedItemSource;

/** Shows a list of items from *any* type of source. */
class ListWindow : public DemoWindow {
 public:
  ListWindow(ListItemSource* source);
  bool OnEvent(const Event& ev) override;
  EventHandler eh_ = EventHandler(this);
};

/** Shows a list of items from a source of type AdvancedItemSource. */
class AdvancedListWindow : public DemoWindow {
 public:
  AdvancedListWindow(AdvancedItemSource* source);
  bool OnEvent(const Event& ev) override;

 private:
  AdvancedItemSource* m_source;
};

/** AdvancedItem adds extra info to a string item. */
class AdvancedItem : public GenericStringItem {
 public:
  AdvancedItem(const char* str, const TBID& id, bool male)
      : GenericStringItem(str, id), m_checked(false), m_male(male) {}
  void SetChecked(bool checked) { m_checked = checked; }
  bool GetChecked() const { return m_checked; }
  bool GetMale() const { return m_male; }

 private:
  std::string m_info;
  bool m_checked;
  bool m_male;
};

/** AdvancedItemSource provides items of type AdvancedItem and makes sure
        the viewer is populated with the customized element for each item. */
class AdvancedItemSource : public ListItemSourceList<AdvancedItem> {
 public:
  bool Filter(size_t index, const std::string& filter) override;
  Element* CreateItemElement(size_t index, ListItemObserver* viewer) override;
};

/** AdvancedItemElement is the element representing a AdvancedItem.
        On changes to the item, it calls InvokeItemChanged on the source, so
   that all
        viewers of the source are updated to reflect the change. */
class AdvancedItemElement : public elements::LayoutBox {
 public:
  AdvancedItemElement(AdvancedItem* item, AdvancedItemSource* source,
                      ListItemObserver* source_viewer, size_t index);
  bool OnEvent(const Event& ev) override;

 private:
  AdvancedItemSource* m_source;
  ListItemObserver* m_source_viewer;
  size_t m_index;
};

#endif  // LISTWINDOW_DEMO_H
