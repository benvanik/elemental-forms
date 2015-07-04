#include "Demo.h"
#include "ListWindow.h"
#include "ResourceEditWindow.h"
#include <stdio.h>
#include <stdarg.h>
#include "tests/tb_test.h"
#include "tb_inline_select.h"
#include "tb_select.h"
#include "tb_menu_window.h"
#include "tb_text_box.h"
#include "tb_tab_container.h"
#include "tb_bitmap_fragment.h"
#include "tb_widget_animation.h"
#include "tb_node_tree.h"
#include "tb_font_renderer.h"
#include "tb_image_manager.h"
#include "CodeTextBox\CodeTextBox.h"

#include "tb/turbo_badger.h"
#include "tb/util/debug.h"
#include "tb/util/metrics.h"
#include "tb/util/string.h"
#include "tb/util/string_builder.h"
#include "tb/util/utf8.h"

static Application* application;

AdvancedItemSource advanced_source;
GenericStringItemSource name_source;
GenericStringItemSource popup_menu_source;

#ifdef TB_SUPPORT_CONSTEXPR

void const_expr_test() {
  // Some code here just to see if the compiler really did
  // implement constexpr (and not just ignored it)
  // Should obviosly only compile if it really works. If not,
  // disable TB_SUPPORT_CONSTEXPR in tb_hash.h for your compiler.
  TBID id("foo");
  switch (id) {
    case TBIDC("foo"):
      break;
    case TBIDC("baar"):
      break;
    default:
      break;
  };
}

#endif  // TB_SUPPORT_CONSTEXPR

// == DemoWindow ==============================================================

DemoWindow::DemoWindow() { application->GetRoot()->AddChild(this); }

bool DemoWindow::LoadResourceFile(const std::string& filename) {
  // We could do ElementFactory::get()->LoadFile(this, filename) but we want
  // some extra data we store under "WindowInfo", so read into node tree.
  Node node;
  if (!node.ReadFile(filename)) return false;
  LoadResource(node);
  return true;
}

void DemoWindow::LoadResourceData(const char* data) {
  // We could do ElementFactory::get()->LoadData(this, filename) but we want
  // some extra data we store under "WindowInfo", so read into node tree.
  Node node;
  node.ReadData(data);
  LoadResource(node);
}

void DemoWindow::LoadResource(Node& node) {
  this->LoadNodeTree(&node);

  // Get title from the WindowInfo section (or use "" if not specified)
  SetText(node.GetValueString("WindowInfo>title", ""));

  const Rect parent_rect(0, 0, GetParent()->rect().w, GetParent()->rect().h);
  auto dc = Skin::get()->GetDimensionConverter();
  Rect window_rect = GetResizeToFitContentRect();

  // Use specified size or adapt to the preferred content size.
  Node* tmp = node.GetNode("WindowInfo>size");
  if (tmp && tmp->GetValue().GetArrayLength() == 2) {
    window_rect.w = dc->GetPxFromString(
        tmp->GetValue().GetArray()->GetValue(0)->GetString(), window_rect.w);
    window_rect.h = dc->GetPxFromString(
        tmp->GetValue().GetArray()->GetValue(1)->GetString(), window_rect.h);
  }

  // Use the specified position or center in parent.
  tmp = node.GetNode("WindowInfo>position");
  if (tmp && tmp->GetValue().GetArrayLength() == 2) {
    window_rect.x = dc->GetPxFromString(
        tmp->GetValue().GetArray()->GetValue(0)->GetString(), window_rect.x);
    window_rect.y = dc->GetPxFromString(
        tmp->GetValue().GetArray()->GetValue(1)->GetString(), window_rect.y);
  } else
    window_rect = window_rect.CenterIn(parent_rect);

  // Make sure the window is inside the parent, and not larger.
  window_rect = window_rect.MoveIn(parent_rect).Clip(parent_rect);

  set_rect(window_rect);

  // Ensure we have focus - now that we've filled the window with possible
  // focusable
  // elements. EnsureFocus was automatically called when the window was
  // activated
  // (by
  // adding the window to the root), but then we had nothing to focus.
  // Alternatively, we could add the window after setting it up properly.
  EnsureFocus();
}

bool DemoWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kKeyDown && ev.special_key == SpecialKey::kEsc) {
    // We could call Die() to fade away and die, but click the close button
    // instead.
    // That way the window has a chance of intercepting the close and f.ex ask
    // if it really should be closed.
    ElementEvent click_ev(EventType::kClick);
    m_close_button.InvokeEvent(click_ev);
    return true;
  }
  return Window::OnEvent(ev);
}

// == EditWindow ==============================================================

class EditWindow : public DemoWindow {
 public:
  EditWindow() {
    LoadResourceFile("Demo/demo01/ui_resources/test_textwindow.tb.txt");
  }
  virtual void OnProcessStates() {
    // Update the disabled state of undo/redo buttons, and caret info.

    if (TextBox* edit = GetElementByIDAndType<TextBox>(TBIDC("text_box"))) {
      if (Element* undo = GetElementByID("undo"))
        undo->SetState(SkinState::kDisabled, !edit->GetStyleEdit()->CanUndo());
      if (Element* redo = GetElementByID("redo"))
        redo->SetState(SkinState::kDisabled, !edit->GetStyleEdit()->CanRedo());
      if (Label* info = GetElementByIDAndType<Label>(TBIDC("info"))) {
        info->SetText(tb::util::format_string(
            "Caret ofs: %d", edit->GetStyleEdit()->caret.GetGlobalOffset()));
      }
    }
  }
  virtual bool OnEvent(const ElementEvent& ev) {
    if (ev.type == EventType::kClick) {
      TextBox* edit = GetElementByIDAndType<TextBox>(TBIDC("text_box"));
      if (!edit) return false;

      if (ev.target->GetID() == TBIDC("clear")) {
        edit->SetText("");
        return true;
      } else if (ev.target->GetID() == TBIDC("undo")) {
        edit->GetStyleEdit()->Undo();
        return true;
      } else if (ev.target->GetID() == TBIDC("redo")) {
        edit->GetStyleEdit()->Redo();
        return true;
      } else if (ev.target->GetID() == TBIDC("menu")) {
        static GenericStringItemSource source;
        if (!source.GetNumItems()) {
          source.AddItem(std::make_unique<GenericStringItem>(
              "Default font", TBIDC("default font")));
          source.AddItem(std::make_unique<GenericStringItem>(
              "Default font (larger)", TBIDC("large font")));
          source.AddItem(std::make_unique<GenericStringItem>(
              "RGB font (Neon)", TBIDC("rgb font Neon")));
          source.AddItem(std::make_unique<GenericStringItem>(
              "RGB font (Orangutang)", TBIDC("rgb font Orangutang")));
          source.AddItem(std::make_unique<GenericStringItem>(
              "RGB font (Orange)", TBIDC("rgb font Orange")));
          source.AddItem(std::make_unique<GenericStringItem>("-"));
          source.AddItem(std::make_unique<GenericStringItem>(
              "Glyph cache stresstest (CJK)", TBIDC("CJK")));
          source.AddItem(std::make_unique<GenericStringItem>("-"));
          source.AddItem(std::make_unique<GenericStringItem>(
              "Toggle wrapping", TBIDC("toggle wrapping")));
          source.AddItem(std::make_unique<GenericStringItem>("-"));
          source.AddItem(std::make_unique<GenericStringItem>(
              "Align left", TBIDC("align left")));
          source.AddItem(std::make_unique<GenericStringItem>(
              "Align center", TBIDC("align center")));
          source.AddItem(std::make_unique<GenericStringItem>(
              "Align right", TBIDC("align right")));
        }

        if (MenuWindow* menu = new MenuWindow(ev.target, TBIDC("popup_menu")))
          menu->Show(&source, PopupAlignment());
        return true;
      } else if (ev.target->GetID() == TBIDC("popup_menu")) {
        if (ev.ref_id == TBIDC("default font"))
          edit->SetFontDescription(FontDescription());
        else if (ev.ref_id == TBIDC("large font")) {
          FontDescription fd = FontManager::get()->GetDefaultFontDescription();
          fd.SetSize(28);
          edit->SetFontDescription(fd);
        } else if (ev.ref_id == TBIDC("rgb font Neon")) {
          FontDescription fd = edit->GetCalculatedFontDescription();
          fd.SetID(TBIDC("Neon"));
          edit->SetFontDescription(fd);
        } else if (ev.ref_id == TBIDC("rgb font Orangutang")) {
          FontDescription fd = edit->GetCalculatedFontDescription();
          fd.SetID(TBIDC("Orangutang"));
          edit->SetFontDescription(fd);
        } else if (ev.ref_id == TBIDC("rgb font Orange")) {
          FontDescription fd = edit->GetCalculatedFontDescription();
          fd.SetID(TBIDC("Orange"));
          edit->SetFontDescription(fd);
        } else if (ev.ref_id == TBIDC("CJK")) {
          StringBuilder buf;
          for (int i = 0, cp = 0x4E00; cp <= 0x9FCC; cp++, i++) {
            char utf8[8];
            int len = util::utf8::encode(cp, utf8);
            buf.Append(utf8, len);
            if (i % 64 == 63) buf.Append("\n", 1);
          }
          edit->GetStyleEdit()->SetText(buf.GetData(), buf.GetAppendPos());
        } else if (ev.ref_id == TBIDC("toggle wrapping"))
          edit->SetWrapping(!edit->GetWrapping());
        else if (ev.ref_id == TBIDC("align left"))
          edit->SetTextAlign(TextAlign::kLeft);
        else if (ev.ref_id == TBIDC("align center"))
          edit->SetTextAlign(TextAlign::kCenter);
        else if (ev.ref_id == TBIDC("align right"))
          edit->SetTextAlign(TextAlign::kRight);
        return true;
      }
    }
    return DemoWindow::OnEvent(ev);
  }
};

// == LayoutWindow ============================================================

LayoutWindow::LayoutWindow(const std::string& filename) {
  LoadResourceFile(filename);
}

bool LayoutWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kChanged &&
      ev.target->GetID() == TBIDC("select position")) {
    LayoutPosition pos = LayoutPosition::kCenter;
    if (SelectDropdown* select =
            GetElementByIDAndType<SelectDropdown>(TBIDC("select position")))
      pos = static_cast<LayoutPosition>(select->GetValue());
    for (int i = 0; i < 3; i++)
      if (Layout* layout = GetElementByIDAndType<Layout>(i + 1))
        layout->SetLayoutPosition(pos);
    return true;
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("toggle axis")) {
    static Axis axis = Axis::kY;
    for (int i = 0; i < 3; i++)
      if (Layout* layout = GetElementByIDAndType<Layout>(i + 1))
        layout->SetAxis(axis);
    axis = axis == Axis::kX ? Axis::kY : Axis::kX;
    if (Layout* layout = GetElementByIDAndType<Layout>(TBIDC("switch_layout")))
      layout->SetAxis(axis);
    ResizeToFitContent(ResizeFit::kCurrentOrNeeded);
    return true;
  }
  return DemoWindow::OnEvent(ev);
}

// == TabContainerWindow
// ============================================================

TabContainerWindow::TabContainerWindow() {
  LoadResourceFile("Demo/demo01/ui_resources/test_tabcontainer01.tb.txt");
}

bool TabContainerWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick &&
      ev.target->GetID() == TBIDC("set_align")) {
    if (TabContainer* tc =
            GetElementByIDAndType<TabContainer>(TBIDC("tabcontainer")))
      tc->SetAlignment(static_cast<Align>(ev.target->data.GetInt()));
    ResizeToFitContent(ResizeFit::kCurrentOrNeeded);
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("toggle_tab_axis")) {
    static Axis axis = Axis::kX;
    axis = axis == Axis::kX ? Axis::kY : Axis::kX;
    if (TabContainer* tc =
            GetElementByIDAndType<TabContainer>(TBIDC("tabcontainer"))) {
      for (Element* child = tc->GetTabLayout()->GetFirstChild(); child;
           child = child->GetNext()) {
        if (Button* button = util::SafeCast<Button>(child)) {
          button->SetAxis(axis);
        }
      }
    }
    ResizeToFitContent(ResizeFit::kCurrentOrNeeded);
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("start_spinner")) {
    if (ProgressSpinner* spinner =
            GetElementByIDAndType<ProgressSpinner>(TBIDC("spinner")))
      spinner->SetValue(1);
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("stop_spinner")) {
    if (ProgressSpinner* spinner =
            GetElementByIDAndType<ProgressSpinner>(TBIDC("spinner")))
      spinner->SetValue(0);
  }
  return DemoWindow::OnEvent(ev);
}

// == ConnectionWindow =========================================================

ConnectionWindow::ConnectionWindow() {
  LoadResourceFile("Demo/demo01/ui_resources/test_connections.tb.txt");
}

bool ConnectionWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick &&
      ev.target->GetID() == TBIDC("reset-master-volume")) {
    if (ElementValue* val = ValueGroup::get()->GetValue(TBIDC("master-volume")))
      val->SetInt(50);
  } else if (ev.type == EventType::kClick &&
             ev.target->GetID() == TBIDC("reset-user-name")) {
    if (ElementValue* val = ValueGroup::get()->GetValue(TBIDC("user-name")))
      val->SetText("");
  }
  return DemoWindow::OnEvent(ev);
}

// == ScrollContainerWindow ===================================================

ScrollContainerWindow::ScrollContainerWindow() {
  LoadResourceFile("Demo/demo01/ui_resources/test_scrollcontainer.tb.txt");

  if (SelectDropdown* select =
          GetElementByIDAndType<SelectDropdown>(TBIDC("name dropdown")))
    select->SetSource(&name_source);

  if (SelectDropdown* select =
          GetElementByIDAndType<SelectDropdown>(TBIDC("advanced dropdown")))
    select->SetSource(&advanced_source);
}

bool ScrollContainerWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick) {
    if (ev.target->GetID() == TBIDC("add img")) {
      Button* button = util::SafeCast<Button>(ev.target);
      SkinImage* skin_image = new SkinImage;
      skin_image->SetSkinBg(TBIDC("Icon16"));
      button->GetContentRoot()->AddChild(skin_image, ElementZ::kBottom);
      return true;
    } else if (ev.target->GetID() == TBIDC("new buttons")) {
      for (int i = 0; i < ev.target->data.GetInt(); i++) {
        Button* button = new Button;
        button->SetID(TBIDC("remove button"));
        button->SetText(tb::util::format_string("Remove %d", i));
        ev.target->GetParent()->AddChild(button);
      }
      return true;
    } else if (ev.target->GetID() == TBIDC("new buttons delayed")) {
      for (int i = 0; i < ev.target->data.GetInt(); i++) {
        MessageData* data = new MessageData();
        data->id1 = ev.target->GetParent()->GetID();
        data->v1.SetInt(i);
        PostMessageDelayed(TBIDC("new button"), data, 100 + i * 500);
      }
      return true;
    } else if (ev.target->GetID() == TBIDC("remove button")) {
      ev.target->GetParent()->RemoveChild(ev.target);
      delete ev.target;
      return true;
    } else if (ev.target->GetID() == TBIDC("showpopupmenu1")) {
      if (MenuWindow* menu = new MenuWindow(ev.target, TBIDC("popupmenu1")))
        menu->Show(&popup_menu_source, PopupAlignment());
      return true;
    } else if (ev.target->GetID() == TBIDC("popupmenu1")) {
      MessageWindow* msg_win = new MessageWindow(this, TBIDC("popup_dialog"));
      msg_win->Show("Info",
                    tb::util::format_string("Menu event received!\nref_id: %d",
                                            (int)ev.ref_id));
      return true;
    }
  }
  return DemoWindow::OnEvent(ev);
}

void ScrollContainerWindow::OnMessageReceived(Message* msg) {
  if (msg->message == TBIDC("new button") && msg->data) {
    if (Element* target = GetElementByID(msg->data->id1)) {
      Button* button = new Button;
      button->SetID(TBIDC("remove button"));
      button->SetText(
          tb::util::format_string("Remove %d", msg->data->v1.GetInt()));
      target->AddChild(button);
    }
  }
}

// == ImageWindow =============================================================

ImageWindow::ImageWindow() {
  LoadResourceFile("Demo/demo01/ui_resources/test_image_widget.tb.txt");
}

bool ImageWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick && ev.target->GetID() == TBIDC("remove")) {
    Element* image = ev.target->GetParent();
    image->GetParent()->RemoveChild(image);
    delete image;
    return true;
  }
  return DemoWindow::OnEvent(ev);
}

// == PageWindow =============================================================

PageWindow::PageWindow() {
  LoadResourceFile("Demo/demo01/ui_resources/test_scroller_snap.tb.txt");

  // Listen to the pagers scroller
  if (Element* pager = GetElementByID(TBIDC("page-scroller")))
    pager->GetScroller()->SetSnapListener(this);
}

bool PageWindow::OnEvent(const ElementEvent& ev) {
  return DemoWindow::OnEvent(ev);
}

void PageWindow::OnScrollSnap(Element* target_element, int& target_x,
                              int& target_y) {
  int page_w = target_element->GetPaddingRect().w;
  int target_page = (target_x + page_w / 2) / page_w;
  target_x = target_page * page_w;
}

// == AnimationsWindow ========================================================

AnimationsWindow::AnimationsWindow() {
  LoadResourceFile("Demo/demo01/ui_resources/test_animations.tb.txt");
  Animate();
}

void AnimationsWindow::Animate() {
  // Abort any still unfinished animations.
  ElementAnimationManager::AbortAnimations(this);

  AnimationCurve curve = AnimationCurve::kSlowDown;
  uint64_t duration = 500;
  bool fade = true;

  if (SelectList* curve_select = GetElementByIDAndType<SelectList>("curve")) {
    curve = static_cast<AnimationCurve>(curve_select->GetValue());
  }
  if (SelectInline* duration_select =
          GetElementByIDAndType<SelectInline>("duration")) {
    duration = uint64_t(duration_select->GetValueDouble());
  }
  if (CheckBox* fade_check = GetElementByIDAndType<CheckBox>("fade")) {
    fade = fade_check->GetValue() ? true : false;
  }

  // Start move animation
  Animation* anim = new RectElementAnimation(
      this, rect().Offset(-rect().x - rect().w, 0), rect());
  AnimationManager::StartAnimation(anim, curve, duration);
  // Start fade animation
  if (fade) {
    auto anim = new OpacityElementAnimation(this, kAlmostZeroOpacity, 1, false);
    AnimationManager::StartAnimation(anim, AnimationCurve::kSlowDown, duration);
  }
}

bool AnimationsWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick && ev.target->GetID() == TBIDC("Animate!"))
    Animate();
  return DemoWindow::OnEvent(ev);
}

// == MainWindow ==============================================================

MainWindow::MainWindow() {
  LoadResourceFile("Demo/demo01/ui_resources/test_ui.tb.txt");

  SetOpacity(0.97f);
}

void MainWindow::OnMessageReceived(Message* msg) {
  if (msg->message == TBIDC("instantmsg")) {
    MessageWindow* msg_win = new MessageWindow(this, TBIDC("test_dialog"));
    msg_win->Show("Message window", "Instant message received!");
  } else if (msg->message == TBIDC("busy")) {
    // Keep the message queue busy by posting another "busy" message.
    PostMessage(TBIDC("busy"), nullptr);
  } else if (msg->message == TBIDC("delayedmsg")) {
    MessageWindow* msg_win = new MessageWindow(this, TBIDC(""));
    msg_win->Show("Message window",
                  tb::util::format_string(
                      "Delayed message received!\n\n"
                      "It was received %d ms after its intended fire time.",
                      int(util::GetTimeMS() - msg->GetFireTime())));
  }
}

bool MainWindow::OnEvent(const ElementEvent& ev) {
  if (ev.type == EventType::kClick) {
    if (ev.target->GetID() == TBIDC("new")) {
      new MainWindow();
      return true;
    }
    if (ev.target->GetID() == TBIDC("msg")) {
      PostMessage(TBIDC("instantmsg"), nullptr);
      return true;
    } else if (ev.target->GetID() == TBIDC("busymsg")) {
      if (ev.target->GetValue() == 1) {
        // Post the first "busy" message when we check the checkbox.
        assert(!GetMessageByID(TBIDC("busy")));
        if (!GetMessageByID(TBIDC("busy"))) {
          PostMessage(TBIDC("busy"), nullptr);
          MessageWindow* msg_win =
              new MessageWindow(this, TBIDC("test_dialog"));
          msg_win->Show("Message window",
                        "The message loop is now constantly busy with messages "
                        "to process.\n\n"
                        "The main thread should be working hard, but input & "
                        "animations should still be running smoothly.");
        }
      } else {
        // Remove any pending "busy" message when we uncheck the checkbox.
        assert(GetMessageByID(TBIDC("busy")));
        if (Message* busymsg = GetMessageByID(TBIDC("busy")))
          DeleteMessage(busymsg);
      }
      return true;
    } else if (ev.target->GetID() == TBIDC("delayedmsg")) {
      PostMessageDelayed(TBIDC("delayedmsg"), nullptr, 2000);
      return true;
    } else if (ev.target->GetID() == TBIDC("Window.close")) {
      // Intercept the Window.close message and stop it from bubbling
      // to Window (prevent the window from closing)
      MessageWindow* msg_win =
          new MessageWindow(this, TBIDC("confirm_close_dialog"));
      MessageWindowSettings settings(MessageWindowButtons::kYesNo,
                                     TBIDC("Icon48"));
      settings.dimmer = true;
      settings.styling = true;
      msg_win->Show("Are you sure?",
                    "Really <color #0794f8>close</color> the window?",
                    &settings);
      return true;
    } else if (ev.target->GetID() == TBIDC("confirm_close_dialog")) {
      if (ev.ref_id == TBIDC("MessageWindow.yes")) Close();
      return true;
    } else if (ev.target->GetID() == TBIDC("reload skin bitmaps")) {
      int reload_count = 10;
      uint64_t t1 = util::GetTimeMS();
      for (int i = 0; i < reload_count; i++) Skin::get()->ReloadBitmaps();
      uint64_t t2 = util::GetTimeMS();

      MessageWindow* msg_win = new MessageWindow(ev.target, TBID());
      msg_win->Show("GFX load performance",
                    tb::util::format_string(
                        "Reloading the skin graphics %d times took %dms",
                        reload_count, (int)(t2 - t1)));
      return true;
    } else if (ev.target->GetID() == TBIDC("test context lost")) {
      Renderer::get()->InvokeContextLost();
      Renderer::get()->InvokeContextRestored();
      MessageWindow* msg_win = new MessageWindow(ev.target, TBID());
      msg_win->Show("Context lost & restore",
                    "Called InvokeContextLost and InvokeContextRestored.\n\n"
                    "Does everything look fine?");
      return true;
    } else if (ev.target->GetID() == TBIDC("test-layout")) {
      std::string resource_file("Demo/demo01/ui_resources/");
      resource_file.append(ev.target->data.GetString());
      new LayoutWindow(resource_file);
      return true;
    } else if (ev.target->GetID() == TBIDC("test-connections")) {
      new ConnectionWindow();
      return true;
    } else if (ev.target->GetID() == TBIDC("test-list")) {
      new AdvancedListWindow(&advanced_source);
      return true;
    } else if (ev.target->GetID() == TBIDC("test-image")) {
      new ImageWindow();
      return true;
    } else if (ev.target->GetID() == TBIDC("test-page")) {
      new PageWindow();
      return true;
    } else if (ev.target->GetID() == TBIDC("test-animations")) {
      new AnimationsWindow();
      return true;
    } else if (ev.target->GetID() == TBIDC("test-scroll-container")) {
      new ScrollContainerWindow();
      return true;
    } else if (ev.target->GetID() == TBIDC("test-skin-conditions")) {
      (new DemoWindow())
          ->LoadResourceFile(
              "Demo/demo01/ui_resources/test_skin_conditions01.tb.txt");
      (new DemoWindow())
          ->LoadResourceFile(
              "Demo/demo01/ui_resources/test_skin_conditions02.tb.txt");
      return true;
    } else if (ev.target->GetID() == TBIDC("test-resource-edit")) {
      ResourceEditWindow* res_edit_win = new ResourceEditWindow();
      res_edit_win->Load("Demo/demo01/ui_resources/resource_edit_test.tb.txt");
      GetParent()->AddChild(res_edit_win);
      return true;
    } else if (ev.type == EventType::kClick &&
               ev.target->GetID() == TBIDC("debug settings")) {
#ifdef TB_RUNTIME_DEBUG_INFO
      util::ShowDebugInfoSettingsWindow(GetParentRoot());
#else
      MessageWindow* msg_win = new MessageWindow(ev.target, TBID());
      msg_win->Show("Debug settings",
                    "Debug settings is only available in builds "
                    "compiled with TB_RUNTIME_DEBUG_INFO defined.\n\n"
                    "Debug builds enable this by default.");
#endif
      return true;
    }
  }
  return DemoWindow::OnEvent(ev);
}

// ======================================================

int fps = 0;
uint32_t frame_counter_total = 0;
uint32_t frame_counter = 0;
uint64_t frame_counter_reset_time = 0;

const char* girl_names[] = {
    "Maja",     "Alice",    "Julia",   "Linnéa",   "Wilma",  "Ella",
    "Elsa",     "Emma",     "Alva",    "Olivia",   "Molly",  "Ebba",
    "Klara",    "Nellie",   "Agnes",   "Isabelle", "Ida",    "Elin",
    "Ellen",    "Moa",      "Emilia",  "Nova",     "Alma",   "Saga",
    "Amanda",   "Isabella", "Lilly",   "Alicia",   "Astrid", "Matilda",
    "Tuva",     "Tilde",    "Stella",  "Felicia",  "Elvira", "Tyra",
    "Hanna",    "Sara",     "Vera",    "Thea",     "Freja",  "Lova",
    "Selma",    "Meja",     "Signe",   "Ester",    "Lovisa", "Ellie",
    "Lea",      "Tilda",    "Tindra",  "Sofia",    "Nora",   "Nathalie",
    "Leia",     "Filippa",  "Siri",    "Emelie",   "Inez",   "Edith",
    "Stina",    "Liv",      "Lisa",    "Linn",     "Tove",   "Emmy",
    "Livia",    "Jasmine",  "Evelina", "Cornelia", "Märta",  "Svea",
    "Ingrid",   "My",       "Rebecca", "Joline",   "Mira",   "Ronja",
    "Hilda",    "Melissa",  "Anna",    "Frida",    "Maria",  "Iris",
    "Josefine", "Elise",    "Elina",   "Greta",    "Vilda",  "Minna",
    "Lina",     "Hedda",    "Nicole",  "Kajsa",    "Majken", "Sofie",
    "Annie",    "Juni",     "Novalie", "Hedvig",   0};
const char* boy_names[] = {
    "Oscar",  "William",    "Lucas",   "Elias",    "Alexander", "Hugo",
    "Oliver", "Theo",       "Liam",    "Leo",      "Viktor",    "Erik",
    "Emil",   "Isak",       "Axel",    "Filip",    "Anton",     "Gustav",
    "Edvin",  "Vincent",    "Arvid",   "Albin",    "Ludvig",    "Melvin",
    "Noah",   "Charlie",    "Max",     "Elliot",   "Viggo",     "Alvin",
    "Alfred", "Theodor",    "Adam",    "Olle",     "Wilmer",    "Benjamin",
    "Simon",  "Nils",       "Noel",    "Jacob",    "Leon",      "Rasmus",
    "Kevin",  "Linus",      "Casper",  "Gabriel",  "Jonathan",  "Milo",
    "Melker", "Felix",      "Love",    "Ville",    "Sebastian", "Sixten",
    "Carl",   "Malte",      "Neo",     "David",    "Joel",      "Adrian",
    "Valter", "Josef",      "Jack",    "Hampus",   "Samuel",    "Mohammed",
    "Alex",   "Tim",        "Daniel",  "Vilgot",   "Wilhelm",   "Harry",
    "Milton", "Maximilian", "Robin",   "Sigge",    "Måns",      "Eddie",
    "Elton",  "Vidar",      "Hjalmar", "Loke",     "Elis",      "August",
    "John",   "Hannes",     "Sam",     "Frank",    "Svante",    "Marcus",
    "Mio",    "Otto",       "Ali",     "Johannes", "Fabian",    "Ebbe",
    "Aron",   "Julian",     "Elvin",   "Ivar",     0};

bool DemoApplication::Init() {
  if (!Application::Init()) return false;

  // Block new animations during Init.
  AnimationBlocker anim_blocker;

  // Run unit tests
  int num_failed_tests = TBRunTests();

  // SelectList and SelectDropdown elements have a default item source that
  // are fed with any items
  // specified in the resource files. But it is also possible to set any source
  // which can save memory
  // and improve performance. Then you don't have to populate each instance with
  // its own set of items,
  // for elements that occur many times in a UI, always with the same items.
  // Here we prepare the name source, that is used in a few places.
  for (int i = 0; boy_names[i]; i++)
    advanced_source.AddItem(std::make_unique<AdvancedItem>(
        boy_names[i++], TBIDC("boy_item"), true));
  for (int i = 0; girl_names[i]; i++)
    advanced_source.AddItem(std::make_unique<AdvancedItem>(
        girl_names[i++], TBIDC("girl_item"), false));
  for (int i = 0; girl_names[i]; i++)
    name_source.AddItem(std::make_unique<GenericStringItem>(
        girl_names[i++], TBIDC("girl_item")));
  for (int i = 0; boy_names[i]; i++)
    name_source.AddItem(
        std::make_unique<GenericStringItem>(boy_names[i++], TBIDC("boy_item")));
  advanced_source.SetSort(Sort::kAscending);
  name_source.SetSort(Sort::kAscending);

  // Prepare a source with submenus (with eternal recursion) so we can test sub
  // menu support.
  popup_menu_source.AddItem(
      std::make_unique<GenericStringItem>("Option 1", TBIDC("opt 1")));
  popup_menu_source.AddItem(
      std::make_unique<GenericStringItem>("Option 2", TBIDC("opt 2")));
  popup_menu_source.AddItem(std::make_unique<GenericStringItem>("-"));
  popup_menu_source.AddItem(
      std::make_unique<GenericStringItem>("Same submenu", &popup_menu_source));
  popup_menu_source.AddItem(
      std::make_unique<GenericStringItem>("Long submenu", &name_source));
  // Give the first item a skin image
  popup_menu_source.GetItem(0)->SetSkinImage(TBIDC("Icon16"));

  new MainWindow();

  new EditWindow;

  new ListWindow(&name_source);

  new AdvancedListWindow(&advanced_source);

  new TabContainerWindow();

  if (num_failed_tests) {
    MessageWindow* msg_win = new MessageWindow(GetRoot(), TBIDC(""));
    msg_win->Show(
        "Testing results",
        tb::util::format_string(
            "There is %d failed tests!\nCheck the output for details.",
            num_failed_tests));
  }
  return true;
}

void DemoApplication::RenderFrame(int window_w, int window_h) {
  // Override RenderFrame without calling super, since we want
  // to inject code between BeginPaint/EndPaint.
  // Application::RenderFrame(window_w, window_h);

  // Render
  Renderer::get()->BeginPaint(window_w, window_h);
  GetRoot()->InvokePaint(Element::PaintProps());

  frame_counter++;
  frame_counter_total++;

  // Update the FPS counter
  uint64_t time = util::GetTimeMS();
  if (time > frame_counter_reset_time + 1000) {
    fps = (int)((frame_counter / double(time - frame_counter_reset_time)) *
                1000.0);
    frame_counter_reset_time = time;
    frame_counter = 0;
  }

  // Draw FPS
  ElementValue* continuous_repaint_val =
      ValueGroup::get()->GetValue(TBIDC("continous-repaint"));
  bool continuous_repaint =
      continuous_repaint_val ? !!continuous_repaint_val->GetInt() : 0;

  std::string str;
  if (continuous_repaint) {
    str = tb::util::format_string("FPS: %d Frame %d", fps, frame_counter_total);
  } else {
    str = tb::util::format_string("Frame %d", frame_counter_total);
  }
  GetRoot()->GetFont()->DrawString(5, 5, Color(255, 255, 255), str);

  Renderer::get()->EndPaint();

  // If we want continous updates or got animations running, reinvalidate
  // immediately
  if (continuous_repaint || AnimationManager::HasAnimationsRunning()) {
    GetRoot()->Invalidate();
  }
}

int app_main() {
  application = new DemoApplication();

  ApplicationBackend* application_backend =
      ApplicationBackend::Create(application, 1280, 700, "Demo");
  if (!application_backend) return 1;

  tb::Initialize(application_backend->GetRenderer(),
                 "resources/language/lng_en.tb.txt");
  CodeTextBox::RegisterInflater();

  // Load the default skin, and override skin that contains the graphics
  // specific to the demo.
  Skin::get()->Load("resources/default_skin/skin.tb.txt",
                    "Demo/demo01/skin/skin.tb.txt");

// Register font renderers.
#ifdef TB_FONT_RENDERER_TBBF
  void register_tbbf_font_renderer();
  register_tbbf_font_renderer();
#endif
#ifdef TB_FONT_RENDERER_STB
  void register_stb_font_renderer();
  register_stb_font_renderer();
#endif
#ifdef TB_FONT_RENDERER_FREETYPE
  void register_freetype_font_renderer();
  register_freetype_font_renderer();
#endif

// Add fonts we can use to the font manager.
#if defined(TB_FONT_RENDERER_STB) || defined(TB_FONT_RENDERER_FREETYPE)
  FontManager::get()->AddFontInfo("resources/vera.ttf", "Vera");
#endif
#ifdef TB_FONT_RENDERER_TBBF
  FontManager::get()->AddFontInfo(
      "resources/default_font/segoe_white_with_shadow.tb.txt", "Segoe");
  FontManager::get()->AddFontInfo("Demo/fonts/neon.tb.txt", "Neon");
  FontManager::get()->AddFontInfo("Demo/fonts/orangutang.tb.txt", "Orangutang");
  FontManager::get()->AddFontInfo("Demo/fonts/orange.tb.txt", "Orange");
#endif

  // Set the default font description for elements to one of the fonts we just
  // added
  FontDescription fd;
#ifdef TB_FONT_RENDERER_TBBF
  fd.SetID(TBIDC("Segoe"));
#else
  fd.SetID(TBIDC("Vera"));
#endif
  fd.SetSize(Skin::get()->GetDimensionConverter()->DpToPx(14));
  FontManager::get()->SetDefaultFontDescription(fd);

  // Create the font now.
  FontFace* font = FontManager::get()->CreateFontFace(
      FontManager::get()->GetDefaultFontDescription());

  // Render some glyphs in one go now since we know we are going to use them. It
  // would work fine
  // without this since glyphs are rendered when needed, but with some extra
  // updating of the glyph bitmap.
  if (font)
    font->RenderGlyphs(
        " !\"#$%&'()*+,-./"
        "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
        "abcdefghijklmnopqrstuvwxyz{|}~•·åäöÅÄÖ");

  // Give the root element a background skin
  application->GetRoot()->SetSkinBg(TBIDC("background"));

  application->Init();
  application->Run();
  application->ShutDown();

  delete application;

  return 0;
}
