#ifndef ResourceEditWindow_H
#define ResourceEditWindow_H

#include "tb/element.h"
#include "tb/element_listener.h"
#include "tb/elements/list_box.h"
#include "tb/elements/text_box.h"
#include "tb/message_handler.h"
#include "tb/window.h"

using namespace tb;

class ResourceItem : public tb::GenericStringItem {
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
  Element* GetSelectedElement() { return m_selected_element.get(); }
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
  elements::ListBox* m_element_list;
  ListItemSourceList<ResourceItem> m_element_list_source;
  elements::ScrollContainer* m_scroll_container;
  Element* m_build_container;
  elements::TextBox* m_source_text_box;
  std::string m_resource_filename;
  WeakElementPointer m_selected_element;
  void AddElementListItemsRecursive(Element* element, int depth);
  bool OnDropFileEvent(const ElementEvent& ev);
};

#endif  // ResourceEditWindow_H
