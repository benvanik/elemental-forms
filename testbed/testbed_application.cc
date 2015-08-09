/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include <cstdarg>
#include <cstdio>

// TODO(benvanik): remove when real testing implemented.
#include "el/testing/testing.h"

#include "el/animation_manager.h"
#include "el/element_animation.h"
#include "el/element_animation_manager.h"
#include "el/elemental_forms.h"
#include "el/elements.h"
#include "el/io/file_manager.h"
#include "el/io/memory_file_system.h"
#include "el/io/posix_file_system.h"
#include "el/io/win32_res_file_system_win.h"
#include "el/parsing/parse_node.h"
#include "el/text/font_manager.h"
#include "el/text/font_renderer.h"
#include "el/text/utf8.h"
#include "el/util/debug.h"
#include "el/util/metrics.h"
#include "el/util/string.h"
#include "el/util/string_builder.h"
#include "el/util/string_table.h"
#include "testbed/list_window.h"
#include "testbed/resource_edit_window.h"
#include "testbed/scratch/code_text_box.h"
#include "testbed/testbed_application.h"

namespace testbed {

using namespace el;
using el::graphics::Renderer;
using el::parsing::ParseNode;

AdvancedItemSource advanced_source;
GenericStringItemSource name_source;
GenericStringItemSource popup_menu_source;

static TestbedApplication* application;

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

bool TestbedApplication::Init() {
  // Block new animations during Init.
  AnimationBlocker anim_blocker;

  // Run unit tests
  int num_failed_tests = TBRunTests();

  // ListBox and DropDownButton elements have a default item source that
  // are fed with any items
  // specified in the resource files. But it is also possible to set any source
  // which can save memory
  // and improve performance. Then you don't have to populate each instance with
  // its own set of items,
  // for elements that occur many times in a UI, always with the same items.
  // Here we prepare the name source, that is used in a few places.
  for (int i = 0; boy_names[i]; i++)
    advanced_source.push_back(std::make_unique<AdvancedItem>(
        boy_names[i++], TBIDC("boy_item"), true));
  for (int i = 0; girl_names[i]; i++)
    advanced_source.push_back(std::make_unique<AdvancedItem>(
        girl_names[i++], TBIDC("girl_item"), false));
  for (int i = 0; girl_names[i]; i++)
    name_source.push_back(std::make_unique<GenericStringItem>(
        girl_names[i++], TBIDC("girl_item")));
  for (int i = 0; boy_names[i]; i++)
    name_source.push_back(
        std::make_unique<GenericStringItem>(boy_names[i++], TBIDC("boy_item")));
  advanced_source.set_sort(Sort::kAscending);
  name_source.set_sort(Sort::kAscending);

  // Prepare a source with submenus (with eternal recursion) so we can test sub
  // menu support.
  popup_menu_source.push_back(
      std::make_unique<GenericStringItem>("Option 1", TBIDC("opt 1")));
  popup_menu_source.push_back(
      std::make_unique<GenericStringItem>("Option 2", TBIDC("opt 2")));
  popup_menu_source.push_back(std::make_unique<GenericStringItem>("-"));
  popup_menu_source.push_back(
      std::make_unique<GenericStringItem>("Same submenu", &popup_menu_source));
  popup_menu_source.push_back(
      std::make_unique<GenericStringItem>("Long submenu", &name_source));
  // Give the first item a skin image
  popup_menu_source[0]->set_icon(TBIDC("Icon16"));

  new MainWindow();

  new EditWindow();

  new ListWindow(&name_source);

  new AdvancedListWindow(&advanced_source);

  new TabContainerWindow();

  if (num_failed_tests) {
    auto msg_win = new MessageForm(GetRoot(), TBIDC(""));
    msg_win->Show(
        "Testing results",
        el::util::format_string(
            "There is %d failed tests!\nCheck the output for details.",
            num_failed_tests));
  }
  return true;
}

void TestbedApplication::Run() { m_backend->Run(); }

void TestbedApplication::ShutDown() {
  delete m_backend;
  m_backend = nullptr;
}

void TestbedApplication::Process() {
  AnimationManager::Update();
  GetRoot()->InvokeProcessStates();
  GetRoot()->InvokeProcess();
}

void TestbedApplication::RenderFrame(int window_w, int window_h) {
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
      ElementValueGroup::get()->GetValue(TBIDC("continous-repaint"));
  bool continuous_repaint =
      continuous_repaint_val ? !!continuous_repaint_val->as_integer() : 0;

  std::string str;
  if (continuous_repaint) {
    str = el::util::format_string("FPS: %d Frame %d", fps, frame_counter_total);
  } else {
    str = el::util::format_string("Frame %d", frame_counter_total);
  }
  GetRoot()->computed_font()->DrawString(5, 5, Color(255, 255, 255), str);

  Renderer::get()->EndPaint();

  // If we want continous updates or got animations running, reinvalidate
  // immediately
  if (continuous_repaint || AnimationManager::has_running_animations()) {
    GetRoot()->Invalidate();
  }
}

int app_main() {
  application = new TestbedApplication();

  ApplicationBackend* application_backend =
      ApplicationBackend::Create(application, 1280, 700, "Demo");
  if (!application_backend) return 1;

  el::Initialize(application_backend->GetRenderer());
  CodeTextBox::RegisterInflater();

  // el::io::FileManager::RegisterFileSystem(
  //    std::make_unique<el::io::PosixFileSystem>("./resources"));
  el::io::FileManager::RegisterFileSystem(
      std::make_unique<el::io::PosixFileSystem>("./testbed/resources"));
  el::io::FileManager::RegisterFileSystem(
      std::make_unique<el::io::Win32ResFileSystem>("IDR_default_resources_"));
  el::io::FileManager::RegisterFileSystem(
      std::make_unique<el::io::Win32ResFileSystem>("IDR_testbed_resources_"));

  util::StringTable::get()->Load("default_language/language_en.tb.txt");

  // Load the default skin, and override skin that contains the graphics
  // specific to the demo.
  Skin::get()->Load("default_skin/skin.tb.txt");
  Skin::get()->Load("skin/skin.tb.txt");

// Register font renderers.
#ifdef EL_FONT_RENDERER_TBBF
  void register_tbbf_font_renderer();
  register_tbbf_font_renderer();
#endif
#ifdef EL_FONT_RENDERER_STB
  void register_stb_font_renderer();
  register_stb_font_renderer();
#endif
#ifdef EL_FONT_RENDERER_FREETYPE
  void register_freetype_font_renderer();
  register_freetype_font_renderer();
#endif
  auto font_manager = text::FontManager::get();

// Add fonts we can use to the font manager.
#if defined(EL_FONT_RENDERER_STB) || defined(EL_FONT_RENDERER_FREETYPE)
  font_manager->AddFontInfo("fonts/vera.ttf", "Vera");
  font_manager->AddFontInfo("fonts/vera-mono.ttf", "Vera Mono");
  font_manager->AddFontInfo("fonts/vera-mono-bold.ttf", "Vera Mono Bold");
  font_manager->AddFontInfo("fonts/vera-mono-bold-italic.ttf",
                            "Vera Mono Bold Italic");
  font_manager->AddFontInfo("fonts/vera-mono-italic.ttf", "Vera Mono Italic");
  font_manager->AddFontInfo("fonts/ProggyClean.ttf", "ProggyClean");
#endif
#ifdef EL_FONT_RENDERER_TBBF
  font_manager->AddFontInfo("fonts/segoe_white_with_shadow.tb.txt", "Segoe");
  font_manager->AddFontInfo("fonts/neon.tb.txt", "Neon");
  font_manager->AddFontInfo("fonts/orangutang.tb.txt", "Orangutang");
  font_manager->AddFontInfo("fonts/orange.tb.txt", "Orange");
#endif

  // Set the default font description for elements to one of the fonts we just
  // added
  FontDescription fd;
#ifdef EL_FONT_RENDERER_TBBF
  fd.set_id(TBIDC("Segoe"));
  fd.set_size(Skin::get()->dimension_converter()->DpToPx(14));
#else
  fd.set_id(TBIDC("ProggyClean"));
  fd.set_size(Skin::get()->dimension_converter()->DpToPx(10));
#endif
  font_manager->set_default_font_description(fd);

  // Create the font now.
  auto font =
      font_manager->CreateFontFace(font_manager->default_font_description());

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
  application->GetRoot()->set_background_skin(TBIDC("background"));

  application->Init();
  application->Run();
  application->ShutDown();

  delete application;

  return 0;
}

// == DemoWindow ==============================================================

DemoWindow::DemoWindow() { application->GetRoot()->AddChild(this); }

bool DemoWindow::LoadResourceFile(const std::string& filename) {
  // We could do ElementFactory::get()->LoadFile(this, filename) but we want
  // some extra data we store under "WindowInfo", so read into node tree.
  ParseNode node;
  if (!node.ReadFile(filename)) return false;
  LoadResource(node);
  return true;
}

void DemoWindow::LoadResourceData(const char* data) {
  // We could do ElementFactory::get()->LoadData(this, filename) but we want
  // some extra data we store under "WindowInfo", so read into node tree.
  ParseNode node;
  node.ReadData(data);
  LoadResource(node);
}

void DemoWindow::LoadResource(ParseNode& node) {
  this->LoadNodeTree(&node);

  // Get title from the WindowInfo section (or use "" if not specified)
  set_text(node.GetValueString("WindowInfo>title", ""));

  const Rect parent_rect(0, 0, parent()->rect().w, parent()->rect().h);
  auto dc = Skin::get()->dimension_converter();
  Rect window_rect = GetResizeToFitContentRect();

  // Use specified size or adapt to the preferred content size.
  ParseNode* tmp = node.GetNode("WindowInfo>size");
  if (tmp && tmp->value().array_size() == 2) {
    window_rect.w = dc->GetPxFromString(
        tmp->value().as_array()->at(0)->as_string(), window_rect.w);
    window_rect.h = dc->GetPxFromString(
        tmp->value().as_array()->at(1)->as_string(), window_rect.h);
  }

  // Use the specified position or center in parent.
  tmp = node.GetNode("WindowInfo>position");
  if (tmp && tmp->value().array_size() == 2) {
    window_rect.x = dc->GetPxFromString(
        tmp->value().as_array()->at(0)->as_string(), window_rect.x);
    window_rect.y = dc->GetPxFromString(
        tmp->value().as_array()->at(1)->as_string(), window_rect.y);
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

bool DemoWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kKeyDown && ev.special_key == SpecialKey::kEsc) {
    // We could call Die() to fade away and die, but click the close button
    // instead.
    // That way the window has a chance of intercepting the close and f.ex ask
    // if it really should be closed.
    Event click_ev(EventType::kClick);
    title_close_button_.InvokeEvent(std::move(click_ev));
    return true;
  }
  return Form::OnEvent(ev);
}

// == EditWindow ==============================================================

EditWindow::EditWindow() { LoadResourceFile("test_textwindow.tb.txt"); }

void EditWindow::OnProcessStates() {
  // Update the disabled state of undo/redo buttons, and caret info.

  if (TextBox* edit = GetElementById<TextBox>(TBIDC("text_box"))) {
    if (auto undo = GetElementById<Element>("undo"))
      undo->set_state(Element::State::kDisabled, !edit->text_view()->CanUndo());
    if (auto redo = GetElementById<Element>("redo"))
      redo->set_state(Element::State::kDisabled, !edit->text_view()->CanRedo());
    if (auto info = GetElementById<Label>(TBIDC("info"))) {
      info->set_text_format("Caret ofs: %d",
                            edit->text_view()->caret.global_offset());
    }
  }
}

bool EditWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick) {
    TextBox* edit = GetElementById<TextBox>(TBIDC("text_box"));
    if (!edit) return false;

    if (ev.target->id() == TBIDC("clear")) {
      edit->set_text("");
      return true;
    } else if (ev.target->id() == TBIDC("undo")) {
      edit->text_view()->Undo();
      return true;
    } else if (ev.target->id() == TBIDC("redo")) {
      edit->text_view()->Redo();
      return true;
    } else if (ev.target->id() == TBIDC("menu")) {
      static GenericStringItemSource source;
      if (!source.size()) {
        source.push_back(std::make_unique<GenericStringItem>(
            "Default font", TBIDC("default font")));
        source.push_back(std::make_unique<GenericStringItem>(
            "Default font (larger)", TBIDC("large font")));
        source.push_back(std::make_unique<GenericStringItem>(
            "RGB font (Neon)", TBIDC("rgb font Neon")));
        source.push_back(std::make_unique<GenericStringItem>(
            "RGB font (Orangutang)", TBIDC("rgb font Orangutang")));
        source.push_back(std::make_unique<GenericStringItem>(
            "RGB font (Orange)", TBIDC("rgb font Orange")));
        source.push_back(std::make_unique<GenericStringItem>("-"));
        source.push_back(std::make_unique<GenericStringItem>(
            "Glyph cache stresstest (CJK)", TBIDC("CJK")));
        source.push_back(std::make_unique<GenericStringItem>("-"));
        source.push_back(std::make_unique<GenericStringItem>(
            "Toggle wrapping", TBIDC("toggle wrapping")));
        source.push_back(std::make_unique<GenericStringItem>("-"));
        source.push_back(std::make_unique<GenericStringItem>(
            "Align left", TBIDC("align left")));
        source.push_back(std::make_unique<GenericStringItem>(
            "Align center", TBIDC("align center")));
        source.push_back(std::make_unique<GenericStringItem>(
            "Align right", TBIDC("align right")));
      }

      if (auto menu = new MenuForm(ev.target, TBIDC("popup_menu")))
        menu->Show(&source, PopupAlignment());
      return true;
    } else if (ev.target->id() == TBIDC("popup_menu")) {
      if (ev.ref_id == TBIDC("default font"))
        edit->set_font_description(FontDescription());
      else if (ev.ref_id == TBIDC("large font")) {
        auto fd = text::FontManager::get()->default_font_description();
        fd.set_size(28);
        edit->set_font_description(fd);
      } else if (ev.ref_id == TBIDC("rgb font Neon")) {
        FontDescription fd = edit->computed_font_description();
        fd.set_id(TBIDC("Neon"));
        edit->set_font_description(fd);
      } else if (ev.ref_id == TBIDC("rgb font Orangutang")) {
        FontDescription fd = edit->computed_font_description();
        fd.set_id(TBIDC("Orangutang"));
        edit->set_font_description(fd);
      } else if (ev.ref_id == TBIDC("rgb font Orange")) {
        FontDescription fd = edit->computed_font_description();
        fd.set_id(TBIDC("Orange"));
        edit->set_font_description(fd);
      } else if (ev.ref_id == TBIDC("CJK")) {
        util::StringBuilder buf;
        for (int i = 0, cp = 0x4E00; cp <= 0x9FCC; ++cp, ++i) {
          char utf8[8];
          int len = text::utf8::encode(cp, utf8);
          buf.Append(utf8, len);
          if (i % 64 == 63) buf.Append("\n", 1);
        }
        edit->text_view()->set_text(buf.c_str(), buf.GetAppendPos());
      } else if (ev.ref_id == TBIDC("toggle wrapping"))
        edit->set_wrapping(!edit->is_wrapping());
      else if (ev.ref_id == TBIDC("align left"))
        edit->set_text_align(TextAlign::kLeft);
      else if (ev.ref_id == TBIDC("align center"))
        edit->set_text_align(TextAlign::kCenter);
      else if (ev.ref_id == TBIDC("align right"))
        edit->set_text_align(TextAlign::kRight);
      return true;
    }
  }
  return DemoWindow::OnEvent(ev);
}

// == LayoutWindow ============================================================

LayoutWindow::LayoutWindow(const std::string& filename) {
  LoadResourceFile(filename);
}

bool LayoutWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kChanged &&
      ev.target->id() == TBIDC("select position")) {
    LayoutPosition pos = LayoutPosition::kCenter;
    if (DropDownButton* select =
            GetElementById<DropDownButton>(TBIDC("select position")))
      pos = static_cast<LayoutPosition>(select->value());
    for (int i = 0; i < 3; i++)
      if (auto layout = GetElementById<LayoutBox>(i + 1))
        layout->set_layout_position(pos);
    return true;
  } else if (ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("toggle axis")) {
    static Axis axis = Axis::kY;
    for (int i = 0; i < 3; i++)
      if (auto layout = GetElementById<LayoutBox>(i + 1))
        layout->set_axis(axis);
    axis = axis == Axis::kX ? Axis::kY : Axis::kX;
    if (auto layout = GetElementById<LayoutBox>(TBIDC("switch_layout")))
      layout->set_axis(axis);
    ResizeToFitContent(ResizeFit::kCurrentOrNeeded);
    return true;
  }
  return DemoWindow::OnEvent(ev);
}

// == TabContainerWindow
// ============================================================

TabContainerWindow::TabContainerWindow() {
  LoadResourceFile("test_tabcontainer01.tb.txt");
}

bool TabContainerWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick && ev.target->id() == TBIDC("set_align")) {
    if (TabContainer* tc = GetElementById<TabContainer>(TBIDC("tabcontainer")))
      tc->set_alignment(static_cast<Align>(ev.target->data.as_integer()));
    ResizeToFitContent(ResizeFit::kCurrentOrNeeded);
  } else if (ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("toggle_tab_axis")) {
    static Axis axis = Axis::kX;
    axis = axis == Axis::kX ? Axis::kY : Axis::kX;
    if (TabContainer* tc =
            GetElementById<TabContainer>(TBIDC("tabcontainer"))) {
      for (Element* child = tc->tab_bar()->first_child(); child;
           child = child->GetNext()) {
        if (Button* button = util::SafeCast<Button>(child)) {
          button->set_axis(axis);
        }
      }
    }
    ResizeToFitContent(ResizeFit::kCurrentOrNeeded);
  } else if (ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("start_spinner")) {
    if (ProgressSpinner* spinner =
            GetElementById<ProgressSpinner>(TBIDC("spinner")))
      spinner->set_value(1);
  } else if (ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("stop_spinner")) {
    if (ProgressSpinner* spinner =
            GetElementById<ProgressSpinner>(TBIDC("spinner")))
      spinner->set_value(0);
  }
  return DemoWindow::OnEvent(ev);
}

// == ConnectionWindow =========================================================

ConnectionWindow::ConnectionWindow() {
  LoadResourceFile("test_connections.tb.txt");
}

bool ConnectionWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick &&
      ev.target->id() == TBIDC("reset-master-volume")) {
    if (ElementValue* val =
            ElementValueGroup::get()->GetValue(TBIDC("master-volume")))
      val->set_integer(50);
  } else if (ev.type == EventType::kClick &&
             ev.target->id() == TBIDC("reset-user-name")) {
    if (ElementValue* val =
            ElementValueGroup::get()->GetValue(TBIDC("user-name")))
      val->set_text("");
  }
  return DemoWindow::OnEvent(ev);
}

// == ScrollContainerWindow ===================================================

ScrollContainerWindow::ScrollContainerWindow() {
  LoadResourceFile("test_scrollcontainer.tb.txt");

  if (DropDownButton* select =
          GetElementById<DropDownButton>(TBIDC("name dropdown")))
    select->set_source(&name_source);

  if (DropDownButton* select =
          GetElementById<DropDownButton>(TBIDC("advanced dropdown")))
    select->set_source(&advanced_source);
}

bool ScrollContainerWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick) {
    if (ev.target->id() == TBIDC("add img")) {
      Button* button = util::SafeCast<Button>(ev.target);
      IconBox* skin_image = new IconBox;
      skin_image->set_background_skin(TBIDC("Icon16"));
      button->content_root()->AddChild(skin_image, ElementZ::kBottom);
      return true;
    } else if (ev.target->id() == TBIDC("new buttons")) {
      for (int i = 0; i < ev.target->data.as_integer(); ++i) {
        Button* button = new Button;
        button->set_id(TBIDC("remove button"));
        button->set_text_format("Remove %d", i);
        ev.target->parent()->AddChild(button);
      }
      return true;
    } else if (ev.target->id() == TBIDC("new buttons delayed")) {
      for (int i = 0; i < ev.target->data.as_integer(); ++i) {
        auto msg_data = std::make_unique<MessageData>();
        msg_data->id1 = ev.target->parent()->id();
        msg_data->v1.set_integer(i);
        PostMessageDelayed(TBIDC("new button"), std::move(msg_data),
                           100 + i * 500);
      }
      return true;
    } else if (ev.target->id() == TBIDC("remove button")) {
      ev.target->parent()->RemoveChild(ev.target);
      delete ev.target;
      return true;
    } else if (ev.target->id() == TBIDC("showpopupmenu1")) {
      if (auto menu = new MenuForm(ev.target, TBIDC("popupmenu1")))
        menu->Show(&popup_menu_source, PopupAlignment());
      return true;
    } else if (ev.target->id() == TBIDC("popupmenu1")) {
      auto msg_win = new MessageForm(this, TBIDC("popup_dialog"));
      msg_win->Show("Info",
                    el::util::format_string("Menu event received!\nref_id: %d",
                                            (int)ev.ref_id));
      return true;
    }
  }
  return DemoWindow::OnEvent(ev);
}

void ScrollContainerWindow::OnMessageReceived(Message* msg) {
  if (msg->message_id() == TBIDC("new button") && msg->data()) {
    if (auto target = GetElementById<Element>(msg->data()->id1)) {
      Button* button = new Button;
      button->set_id(TBIDC("remove button"));
      button->set_text(
          el::util::format_string("Remove %d", msg->data()->v1.as_integer()));
      target->AddChild(button);
    }
  }
}

// == ImageWindow =============================================================

ImageWindow::ImageWindow() { LoadResourceFile("test_image_widget.tb.txt"); }

bool ImageWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick && ev.target->id() == TBIDC("remove")) {
    Element* image = ev.target->parent();
    image->parent()->RemoveChild(image);
    delete image;
    return true;
  }
  return DemoWindow::OnEvent(ev);
}

// == PageWindow =============================================================

PageWindow::PageWindow() {
  LoadResourceFile("test_scroller_snap.tb.txt");

  // Listen to the pagers scroller
  if (auto pager = GetElementById<Element>(TBIDC("page-scroller")))
    pager->scroller()->SetSnapListener(this);
}

bool PageWindow::OnEvent(const Event& ev) { return DemoWindow::OnEvent(ev); }

void PageWindow::OnScrollSnap(Element* target_element, int* target_x,
                              int* target_y) {
  int page_w = target_element->padding_rect().w;
  int target_page = (*target_x + page_w / 2) / page_w;
  *target_x = target_page * page_w;
}

// == AnimationsWindow ========================================================

AnimationsWindow::AnimationsWindow() {
  LoadResourceFile("test_animations.tb.txt");
  Animate();
}

void AnimationsWindow::Animate() {
  // Abort any still unfinished animations.
  ElementAnimationManager::AbortAnimations(this);

  AnimationCurve curve = AnimationCurve::kSlowDown;
  uint64_t duration = 500;
  bool fade = true;

  if (ListBox* curve_select = GetElementById<ListBox>("curve")) {
    curve = static_cast<AnimationCurve>(curve_select->value());
  }
  if (SpinBox* duration_select = GetElementById<SpinBox>("duration")) {
    duration = uint64_t(duration_select->double_value());
  }
  if (CheckBox* fade_check = GetElementById<CheckBox>("fade")) {
    fade = fade_check->value() ? true : false;
  }

  // Start move animation
  Animation* anim = new RectElementAnimation(
      this, rect().Offset(-rect().x - rect().w, 0), rect());
  AnimationManager::StartAnimation(anim, curve, duration);
  // Start fade animation
  if (fade) {
    auto anim = new OpacityElementAnimation(
        this, ElementAnimation::kAlmostZeroOpacity, 1, false);
    AnimationManager::StartAnimation(anim, AnimationCurve::kSlowDown, duration);
  }
}

bool AnimationsWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick && ev.target->id() == TBIDC("Animate!"))
    Animate();
  return DemoWindow::OnEvent(ev);
}

class FullScreenWindow : public DemoWindow {
 public:
  FullScreenWindow() : DemoWindow() {
    set_settings(FormSettings::kFullScreen);
  }
};

class DslWindow : public DemoWindow {
 public:
  DslWindow() {
    set_size(640, 480);

    using namespace el::dsl;
    auto node = LayoutBoxNode()
                    .axis(Axis::kX)
                    .child(LabelNode("foo"))
                    .child(LabelNode("bar"));
    LoadNodeTree(node);

    LoadNodeTree(BuildUI());
  }

  dsl::Node BuildSomeControl() {
    using namespace el::dsl;
    return LayoutBoxNode()
        .axis(Axis::kX)
        .child(LabelNode("foo"))
        .child(LabelNode("bar"));
  }
  dsl::Node BuildUI() {
    using namespace el::dsl;
    auto rep_tree = LayoutBoxNode().axis(Axis::kX).child_list({
        LabelNode("item"), ButtonNode("button"),
    });
    return LayoutBoxNode()
        .id("foo")
        .position(LayoutPosition::kLeftTop)
        .axis(Axis::kY)
        .children(
            TabContainerNode()
                .gravity(Gravity::kAll)
                .axis(Axis::kX)
                .tab(ButtonNode("Foo"), CloneNode(rep_tree))
                .tab(ButtonNode("Foo0"), CloneNode(rep_tree))
                .tab(ButtonNode("Foo1"), LabelNode("bar1")),
            GroupBoxNode("controls:")
                .content(LayoutBoxNode()
                             .child(BuildSomeControl())
                             .child(BuildSomeControl())),
            LabelNode("distribution: preferred").width(32_dp).font_size(3_mm),
            LayoutBoxNode()
                .distribution(LayoutDistribution::kPreferred)
                .child(ButtonNode("tab 0"))
                .child(TextBoxNode("foo").type(EditType::kPassword))
                .children(ToggleContainerNode()
                              .value(true)
                              .toggle_action(ToggleAction::kExpanded)
                              .child(TextBoxNode()
                                         .placeholder("@search")
                                         .gravity(Gravity::kLeftRight)
                                         .type(EditType::kSearch)),
                          ButtonNode("fffoo").is_enabled(false))
                .child(ButtonNode("tab 0"))
                .child(ButtonNode("tab 0"))
                .clone(rep_tree)
                .children(ButtonNode("tab 1"), ButtonNode("tab 2"),
                          ButtonNode("tab 3"), ButtonNode("tab 4")),
            SpinBoxNode(4, 0, 40), ListBoxNode().items({
                                       {1, "a"}, {2, "b"},
                                   }),
            ListBoxNode().item("a").item("id", "b").item(5, "c"),
            DropDownButtonNode().value(1).items({
                Item("1", "a"), Item("2", "b"),
            }),
            DropDownButtonNode().value(1).items({
                {1, "a"}, {2, "b"},
            }),
            DropDownButtonNode().value(1).items({
                "a", "b", "c",
            }));
  }

  bool OnEvent(const Event& ev) override {
    //
    return DemoWindow::OnEvent(ev);
  }
};

// == MainWindow ==============================================================

MainWindow::MainWindow() {
  LoadResourceFile("test_ui.tb.txt");

  set_opacity(0.97f);
}

void MainWindow::OnMessageReceived(Message* msg) {
  if (msg->message_id() == TBIDC("instantmsg")) {
    auto msg_win = new MessageForm(this, TBIDC("test_dialog"));
    msg_win->Show("Message window", "Instant message received!");
  } else if (msg->message_id() == TBIDC("busy")) {
    // Keep the message queue busy by posting another "busy" message.
    PostMessage(TBIDC("busy"), nullptr);
  } else if (msg->message_id() == TBIDC("delayedmsg")) {
    auto msg_win = new MessageForm(this, TBIDC(""));
    msg_win->Show("Message window",
                  el::util::format_string(
                      "Delayed message received!\n\n"
                      "It was received %d ms after its intended fire time.",
                      int(util::GetTimeMS() - msg->fire_time_millis())));
  }
}

bool MainWindow::OnEvent(const Event& ev) {
  if (ev.type == EventType::kClick) {
    if (ev.target->id() == TBIDC("new")) {
      new MainWindow();
      return true;
    }
    if (ev.target->id() == TBIDC("msg")) {
      PostMessage(TBIDC("instantmsg"), nullptr);
      return true;
    } else if (ev.target->id() == TBIDC("busymsg")) {
      if (ev.target->value() == 1) {
        // Post the first "busy" message when we check the checkbox.
        assert(!GetMessageById(TBIDC("busy")));
        if (!GetMessageById(TBIDC("busy"))) {
          PostMessage(TBIDC("busy"), nullptr);
          auto msg_win = new MessageForm(this, TBIDC("test_dialog"));
          msg_win->Show("Message window",
                        "The message loop is now constantly busy with messages "
                        "to process.\n\n"
                        "The main thread should be working hard, but input & "
                        "animations should still be running smoothly.");
        }
      } else {
        // Remove any pending "busy" message when we uncheck the checkbox.
        assert(GetMessageById(TBIDC("busy")));
        if (Message* busymsg = GetMessageById(TBIDC("busy")))
          DeleteMessage(busymsg);
      }
      return true;
    } else if (ev.target->id() == TBIDC("delayedmsg")) {
      PostMessageDelayed(TBIDC("delayedmsg"), nullptr, 2000);
      return true;
    } else if (ev.target->id() == TBIDC("Window.close")) {
      // Intercept the Window.close message and stop it from bubbling
      // to Window (prevent the window from closing)
      auto msg_win = new MessageForm(this, TBIDC("confirm_close_dialog"));
      MessageFormSettings settings(MessageFormButtons::kYesNo, TBIDC("Icon48"));
      settings.dimmer = true;
      settings.styling = true;
      msg_win->Show("Are you sure?",
                    "Really <color #0794f8>close</color> the window?",
                    &settings);
      return true;
    } else if (ev.target->id() == TBIDC("confirm_close_dialog")) {
      if (ev.ref_id == TBIDC("MessageWindow.yes")) Close();
      return true;
    } else if (ev.target->id() == TBIDC("reload skin bitmaps")) {
      int reload_count = 10;
      uint64_t t1 = util::GetTimeMS();
      for (int i = 0; i < reload_count; ++i) {
        Skin::get()->ReloadBitmaps();
      }
      uint64_t t2 = util::GetTimeMS();

      auto msg_win = new MessageForm(ev.target, TBID());
      msg_win->Show("GFX load performance",
                    el::util::format_string(
                        "Reloading the skin graphics %d times took %dms",
                        reload_count, (int)(t2 - t1)));
      return true;
    } else if (ev.target->id() == TBIDC("test context lost")) {
      Renderer::get()->InvokeContextLost();
      Renderer::get()->InvokeContextRestored();
      auto msg_win = new MessageForm(ev.target, TBID());
      msg_win->Show("Context lost & restore",
                    "Called InvokeContextLost and InvokeContextRestored.\n\n"
                    "Does everything look fine?");
      return true;
    } else if (ev.target->id() == TBIDC("test-layout")) {
      std::string resource_file("");
      resource_file.append(ev.target->data.as_string());
      new LayoutWindow(resource_file);
      return true;
    } else if (ev.target->id() == TBIDC("test-connections")) {
      new ConnectionWindow();
      return true;
    } else if (ev.target->id() == TBIDC("test-list")) {
      new AdvancedListWindow(&advanced_source);
      return true;
    } else if (ev.target->id() == TBIDC("test-image")) {
      new ImageWindow();
      return true;
    } else if (ev.target->id() == TBIDC("test-page")) {
      new PageWindow();
      return true;
    } else if (ev.target->id() == TBIDC("test-animations")) {
      new AnimationsWindow();
      return true;
    } else if (ev.target->id() == TBIDC("test-scroll-container")) {
      new ScrollContainerWindow();
      return true;
    } else if (ev.target->id() == TBIDC("test-skin-conditions")) {
      (new DemoWindow())->LoadResourceFile("test_skin_conditions01.tb.txt");
      (new DemoWindow())->LoadResourceFile("test_skin_conditions02.tb.txt");
      return true;
    } else if (ev.target->id() == TBIDC("test-fullscreen-window")) {
      new FullScreenWindow();
      return true;
    } else if (ev.target->id() == TBIDC("test-resource-edit")) {
      ResourceEditWindow* res_edit_win = new ResourceEditWindow();
      res_edit_win->Load("resource_edit_test.tb.txt");
      parent()->AddChild(res_edit_win);
      return true;
    } else if (ev.target->id() == TBIDC("test-dsl")) {
      new DslWindow();
      return true;
    } else if (ev.type == EventType::kClick &&
               ev.target->id() == TBIDC("debug settings")) {
#ifdef EL_RUNTIME_DEBUG_INFO
      util::ShowDebugInfoSettingsForm(parent_root());
#else
      MessageWindow* msg_win = new MessageWindow(ev.target, TBID());
      msg_win->Show("Debug settings",
                    "Debug settings is only available in builds "
                    "compiled with EL_RUNTIME_DEBUG_INFO defined.\n\n"
                    "Debug builds enable this by default.");
#endif
      return true;
    }
  }
  return DemoWindow::OnEvent(ev);
}

}  // namespace testbed
