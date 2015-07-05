#ifndef ResourceEditWindow_H
#define ResourceEditWindow_H

#include "tb_widgets.h"
#include "tb_select.h"
#include "tb_widgets_common.h"
#include "tb_text_box.h"
#include "tb_msg.h"

#include "tb/element_listener.h"

using namespace tb;

class ResourceItem : public GenericStringItem {
 public:
  ResourceItem(Element* element, const std::string& str);
  Element* GetElement() { return m_element; }

 private:
  Element* m_element;
};

class ResourceEditWindow : public Window,
                           public MessageHandler,
                           public ElementListener {
 public:
  TBOBJECT_SUBCLASS(ResourceEditWindow, Window);

  ResourceEditWindow();
  ~ResourceEditWindow();

  void UpdateElementList(bool immediately);

  struct ITEM_INFO {
    ResourceItem* item;
    int index;
  };
  ITEM_INFO GetItemFromElement(Element* element);
  Element* GetSelectedElement() { return m_selected_element.Get(); }
  void SetSelectedElement(Element* element);

  void Load(const char* resource_file);
  void RefreshFromSource();

  // == Window
  // ======================================================================
  virtual bool OnEvent(const ElementEvent& ev);
  virtual void OnPaintChildren(const PaintProps& paint_props);

  // == MessageHandler
  // ==============================================================
  virtual void OnMessageReceived(Message* msg);

  // == ElementListener
  // ========================================================
  virtual bool OnElementInvokeEvent(Element* element, const ElementEvent& ev);
  virtual void OnElementAdded(Element* parent, Element* child);
  virtual void OnElementRemove(Element* parent, Element* child);

 private:
  SelectList* m_element_list;
  SelectItemSourceList<ResourceItem> m_element_list_source;
  ScrollContainer* m_scroll_container;
  Element* m_build_container;
  TextBox* m_source_text_box;
  std::string m_resource_filename;
  WeakElementPointer m_selected_element;
  void AddElementListItemsRecursive(Element* element, int depth);
  bool OnDropFileEvent(const ElementEvent& ev);
};

#endif  // ResourceEditWindow_H
