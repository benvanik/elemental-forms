#include <cstdio>

#include "ResourceEditWindow.h"
#include "tb_widgets_reader.h"
#include "tb_message_window.h"
#include "tb_system.h"
#include "tb_select.h"
#include "tb_text_box.h"
#include "tb_scroll_container.h"

#include "tb/util/string.h"
#include "tb/util/string_builder.h"

// == ResourceItem
// ====================================================================================

ResourceItem::ResourceItem(Element* element, const std::string& str)
    : GenericStringItem(str), m_element(element) {}

// == ResourceEditWindow
// ==============================================================================

ResourceEditWindow::ResourceEditWindow()
    : m_element_list(nullptr),
      m_scroll_container(nullptr),
      m_build_container(nullptr),
      m_source_text_box(nullptr) {
  // Register as global listener to intercept events in the build container
  ElementListener::AddGlobalListener(this);

  g_elements_reader->LoadFile(
      this, "Demo/demo01/ui_resources/resource_edit_window.tb.txt");

  m_scroll_container =
      GetElementByIDAndType<ScrollContainer>(TBIDC("scroll_container"));
  m_build_container = m_scroll_container->GetContentRoot();
  m_source_text_box = GetElementByIDAndType<TextBox>(TBIDC("source_edit"));

  m_element_list = GetElementByIDAndType<SelectList>(TBIDC("element_list"));
  m_element_list->SetSource(&m_element_list_source);

  set_rect({100, 50, 900, 600});
}

ResourceEditWindow::~ResourceEditWindow() {
  ElementListener::RemoveGlobalListener(this);

  // avoid assert
  m_element_list->SetSource(nullptr);
}

void ResourceEditWindow::Load(const char* resource_file) {
  m_resource_filename = resource_file;
  SetText(resource_file);

  // Set the text of the source view.
  m_source_text_box->SetText("");

  if (TBFile* file = TBFile::Open(m_resource_filename, TBFile::Mode::kRead)) {
    StringBuilder buffer(file->Size());
    size_t size_read = file->Read(buffer.GetData(), 1, buffer.GetCapacity());
    m_source_text_box->SetText(buffer.GetData(), size_read);
    delete file;
  } else {
    // Error, show message.
    MessageWindow* msg_win = new MessageWindow(GetParentRoot(), TBIDC(""));
    msg_win->Show(
        "Error loading resource",
        tb::util::format_string("Could not load file %s", resource_file));
  }

  RefreshFromSource();
}

void ResourceEditWindow::RefreshFromSource() {
  // Clear old elements
  while (Element* child = m_build_container->GetFirstChild()) {
    m_build_container->RemoveChild(child);
    delete child;
  }

  // Create new elements from source
  g_elements_reader->LoadData(m_build_container,
                              m_source_text_box->GetText().c_str());

  // Force focus back in case the edited resource has autofocus.
  // FIX: It would be better to prevent the focus change instead!
  m_source_text_box->SetFocus(FocusReason::kUnknown);
}

void ResourceEditWindow::UpdateElementList(bool immediately) {
  if (!immediately) {
    TBID id = TBIDC("update_element_list");
    if (!GetMessageByID(id)) PostMessage(id, nullptr);
  } else {
    m_element_list_source.DeleteAllItems();
    AddElementListItemsRecursive(m_build_container, 0);

    m_element_list->InvalidateList();
  }
}

void ResourceEditWindow::AddElementListItemsRecursive(Element* element,
                                                      int depth) {
  if (depth > 0)  // Ignore the root
  {
    // Add a new ResourceItem for this element
    const char* classname = element->GetClassName();
    if (!*classname) {
      classname = "<Unknown element type>";
    }
    auto str = tb::util::format_string("% *s%s", depth - 1, "", classname);
    auto item = std::make_unique<ResourceItem>(element, str.c_str());
    m_element_list_source.AddItem(std::move(item));
  }

  for (Element* child = element->GetFirstChild(); child;
       child = child->GetNext()) {
    AddElementListItemsRecursive(child, depth + 1);
  }
}

ResourceEditWindow::ITEM_INFO ResourceEditWindow::GetItemFromElement(
    Element* element) {
  ITEM_INFO item_info = {nullptr, -1};
  for (int i = 0; i < m_element_list_source.GetNumItems(); i++)
    if (m_element_list_source.GetItem(i)->GetElement() == element) {
      item_info.index = i;
      item_info.item = m_element_list_source.GetItem(i);
      break;
    }
  return item_info;
}

void ResourceEditWindow::SetSelectedElement(Element* element) {
  m_selected_element.Set(element);
  ITEM_INFO item_info = GetItemFromElement(element);
  if (item_info.item) m_element_list->SetValue(item_info.index);
}

bool ResourceEditWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kChanged &&
      ev.target->GetID() == TBIDC("element_list_search")) {
    m_element_list->SetFilter(ev.target->GetText());
    return true;
  } else if (ev.type == EventType::kChanged && ev.target == m_element_list) {
    if (m_element_list->GetValue() >= 0 &&
        m_element_list->GetValue() < m_element_list_source.GetNumItems())
      if (ResourceItem* item =
              m_element_list_source.GetItem(m_element_list->GetValue()))
        SetSelectedElement(item->GetElement());
  } else if (ev.type == EventType::kChanged && ev.target == m_source_text_box) {
    RefreshFromSource();
    return true;
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("test")) {
    // Create a window containing the current layout, resize and center it.
    Window* win = new Window();
    win->SetText("Test window");
    g_elements_reader->LoadData(win->GetContentRoot(),
                                m_source_text_box->GetText().c_str());
    Rect bounds(0, 0, GetParent()->rect().w, GetParent()->rect().h);
    win->set_rect(
        win->GetResizeToFitContentRect().CenterIn(bounds).MoveIn(bounds).Clip(
            bounds));
    GetParent()->AddChild(win);
    return true;
  } else if (ev.target->GetID() == TBIDC("constrained")) {
    m_scroll_container->SetAdaptContentSize(ev.target->GetValue() ? true
                                                                  : false);
    return true;
  } else if (ev.type == EventType::kFileDrop) {
    return OnDropFileEvent(ev);
  }
  return Window::OnEvent(ev);
}

void ResourceEditWindow::OnPaintChildren(const PaintProps& paint_props) {
  Window::OnPaintChildren(paint_props);

  // Paint the selection of the selected element
  if (Element* selected_element = GetSelectedElement()) {
    Rect element_rect(0, 0, selected_element->rect().w,
                      selected_element->rect().h);
    selected_element->ConvertToRoot(element_rect.x, element_rect.y);
    ConvertFromRoot(element_rect.x, element_rect.y);
    g_renderer->DrawRect(element_rect, Color(255, 205, 0));
  }
}

void ResourceEditWindow::OnMessageReceived(Message* msg) {
  if (msg->message == TBIDC("update_element_list")) UpdateElementList(true);
}

bool ResourceEditWindow::OnElementInvokeEvent(Element* element,
                                              const ElementEvent& ev) {
  // Intercept all events to elements in the build container
  if (m_build_container->IsAncestorOf(ev.target)) {
    // Let events through if alt is pressed so we can test some
    // functionality right in the editor (like toggle hidden UI).
    if (any(ev.modifierkeys & ModifierKeys::kAlt)) return false;

    // Select element when clicking
    if (ev.type == EventType::kPointerDown) SetSelectedElement(ev.target);

    if (ev.type == EventType::kFileDrop) OnDropFileEvent(ev);
    return true;
  }
  return false;
}

void ResourceEditWindow::OnElementAdded(Element* parent, Element* child) {
  if (m_build_container && m_build_container->IsAncestorOf(child))
    UpdateElementList(false);
}

void ResourceEditWindow::OnElementRemove(Element* parent, Element* child) {
  if (m_build_container && m_build_container->IsAncestorOf(child))
    UpdateElementList(false);
}

bool ResourceEditWindow::OnDropFileEvent(const ElementEvent& ev) {
  const ElementEventFileDrop* fd_event = TBSafeCast<ElementEventFileDrop>(&ev);
  if (fd_event->files.size() > 0) {
    Load(fd_event->files[0].c_str());
  }
  return true;
}
