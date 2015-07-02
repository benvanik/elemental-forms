#ifndef ResourceEditWindow_H
#define ResourceEditWindow_H

#include "tb_widgets.h"
#include "tb_select.h"
#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"
#include "tb_editfield.h"
#include "tb_msg.h"

using namespace tb;

class ResourceItem : public GenericStringItem {
 public:
  ResourceItem(TBWidget* widget, const std::string& str);
  TBWidget* GetWidget() { return m_widget; }

 private:
  TBWidget* m_widget;
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
  ITEM_INFO GetItemFromWidget(TBWidget* widget);
  TBWidget* GetSelectedWidget() { return m_selected_widget.Get(); }
  void SetSelectedWidget(TBWidget* widget);

  void Load(const char* resource_file);
  void RefreshFromSource();

  // == Window
  // ======================================================================
  virtual bool OnEvent(const TBWidgetEvent& ev);
  virtual void OnPaintChildren(const PaintProps& paint_props);

  // == MessageHandler
  // ==============================================================
  virtual void OnMessageReceived(Message* msg);

  // == TBWidgetListener
  // ========================================================
  virtual bool OnWidgetInvokeEvent(TBWidget* widget, const TBWidgetEvent& ev);
  virtual void OnWidgetAdded(TBWidget* parent, TBWidget* child);
  virtual void OnWidgetRemove(TBWidget* parent, TBWidget* child);

 private:
  SelectList* m_widget_list;
  SelectItemSourceList<ResourceItem> m_widget_list_source;
  ScrollContainer* m_scroll_container;
  TBWidget* m_build_container;
  TBEditField* m_source_edit;
  std::string m_resource_filename;
  WeakWidgetPointer m_selected_widget;
  void AddWidgetListItemsRecursive(TBWidget* widget, int depth);
  bool OnDropFileEvent(const TBWidgetEvent& ev);
};

#endif  // ResourceEditWindow_H
