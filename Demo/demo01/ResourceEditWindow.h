#ifndef ResourceEditWindow_H
#define ResourceEditWindow_H

#include "tb_widgets.h"
#include "tb_select.h"
#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"
#include "tb_text_box.h"
#include "tb_msg.h"

using namespace tb;

class ResourceItem : public GenericStringItem {
 public:
  ResourceItem(Widget* widget, const std::string& str);
  Widget* GetWidget() { return m_widget; }

 private:
  Widget* m_widget;
};

class ResourceEditWindow : public Window,
                           public MessageHandler,
                           public WidgetListener {
 public:
  TBOBJECT_SUBCLASS(ResourceEditWindow, Window);

  ResourceEditWindow();
  ~ResourceEditWindow();

  void UpdateWidgetList(bool immediately);

  struct ITEM_INFO {
    ResourceItem* item;
    int index;
  };
  ITEM_INFO GetItemFromWidget(Widget* widget);
  Widget* GetSelectedWidget() { return m_selected_widget.Get(); }
  void SetSelectedWidget(Widget* widget);

  void Load(const char* resource_file);
  void RefreshFromSource();

  // == Window
  // ======================================================================
  virtual bool OnEvent(const WidgetEvent& ev);
  virtual void OnPaintChildren(const PaintProps& paint_props);

  // == MessageHandler
  // ==============================================================
  virtual void OnMessageReceived(Message* msg);

  // == WidgetListener
  // ========================================================
  virtual bool OnWidgetInvokeEvent(Widget* widget, const WidgetEvent& ev);
  virtual void OnWidgetAdded(Widget* parent, Widget* child);
  virtual void OnWidgetRemove(Widget* parent, Widget* child);

 private:
  SelectList* m_widget_list;
  SelectItemSourceList<ResourceItem> m_widget_list_source;
  ScrollContainer* m_scroll_container;
  Widget* m_build_container;
  TextBox* m_source_text_box;
  std::string m_resource_filename;
  WeakWidgetPointer m_selected_widget;
  void AddWidgetListItemsRecursive(Widget* widget, int depth);
  bool OnDropFileEvent(const WidgetEvent& ev);
};

#endif  // ResourceEditWindow_H
