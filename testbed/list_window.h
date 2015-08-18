/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef TESTBED_LIST_WINDOW_H_
#define TESTBED_LIST_WINDOW_H_

#include "el/event_handler.h"
#include "el/list_item.h"
#include "testbed/testbed_application.h"

namespace testbed {

class AdvancedItemSource;

/** Shows a list of items from *any* type of source. */
class ListWindow : public DemoWindow {
 public:
  ListWindow(ListItemSource* source);
  bool OnEvent(const Event& ev) override;
  EventHandler eh_;
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

}  // namespace testbed

#endif  // TESTBED_LIST_WINDOW_H_
