#include "glfw_extra.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <algorithm>

#include "tb_skin.h"
#include "tb_system.h"
#include "tb_widgets.h"
#include "tb_msg.h"
#include "tb_renderer_gl.h"
#include "tb_font_renderer.h"
#include "Application.h"

#ifdef TB_TARGET_MACOSX
#include <unistd.h>
#include <mach-o/dyld.h>
#endif

using namespace tb;

#define TB_USE_CURRENT_DIRECTORY

int mouse_x = 0;
int mouse_y = 0;
bool key_alt = false;
bool key_ctrl = false;
bool key_shift = false;
bool key_super = false;

class ApplicationBackendGLFW;

void SetBackend(GLFWwindow* window, ApplicationBackendGLFW* backend) {
  glfwSetWindowUserPointer(window, backend);
}

ApplicationBackendGLFW* GetBackend(GLFWwindow* window) {
  return static_cast<ApplicationBackendGLFW*>(glfwGetWindowUserPointer(window));
}

// The root of all elements in a GLFW window.
class RootElement : public Element {
 public:
  RootElement(ApplicationBackendGLFW* backend) : m_backend(backend) {}
  virtual void OnInvalid();

 private:
  ApplicationBackendGLFW* m_backend;
};

class ApplicationBackendGLFW : public ApplicationBackend {
 public:
  bool Init(Application* app, int width, int height, const char* title);
  ApplicationBackendGLFW()
      : m_application(nullptr),
        m_renderer(nullptr),
        m_root(this),
        mainWindow(0),
        has_pending_update(false) {}
  ~ApplicationBackendGLFW();

  virtual void Run();
  virtual Element* GetRoot() { return &m_root; }
  virtual Renderer* GetRenderer() { return m_renderer; }

  int width() const { return m_root.rect().w; }
  int height() const { return m_root.rect().h; }

  Application* m_application;
  RendererGL* m_renderer;
  RootElement m_root;
  GLFWwindow* mainWindow;
  bool has_pending_update;
};

void RootElement::OnInvalid() {
  if (!m_backend->has_pending_update) {
    m_backend->has_pending_update = true;
  }
}

ModifierKeys GetModifierKeys() {
  ModifierKeys code = ModifierKeys::kNone;
  if (key_alt) code |= ModifierKeys::kAlt;
  if (key_ctrl) code |= ModifierKeys::kCtrl;
  if (key_shift) code |= ModifierKeys::kShift;
  if (key_super) code |= ModifierKeys::kSuper;
  return code;
}

ModifierKeys GetModifierKeys(int modifier) {
  ModifierKeys code = ModifierKeys::kNone;
  if (modifier & GLFW_MOD_ALT) code |= ModifierKeys::kAlt;
  if (modifier & GLFW_MOD_CONTROL) code |= ModifierKeys::kCtrl;
  if (modifier & GLFW_MOD_SHIFT) code |= ModifierKeys::kShift;
  if (modifier & GLFW_MOD_SUPER) code |= ModifierKeys::kSuper;
  return code;
}

static bool ShouldEmulateTouchEvent() {
  // Used to emulate that mouse events are touch events when alt, ctrl and shift
  // are pressed.
  // This makes testing a lot easier when there is no touch screen around :)
  return any(GetModifierKeys() &
             (ModifierKeys::kAlt | ModifierKeys::kCtrl | ModifierKeys::kShift));
}

// @return Return the upper case of a ascii charcter. Only for shortcut
// handling.
static int toupr_ascii(int ascii) {
  if (ascii >= 'a' && ascii <= 'z') return ascii + 'A' - 'a';
  return ascii;
}

static bool InvokeShortcut(int key, SpecialKey special_key,
                           ModifierKeys modifierkeys, bool down) {
#ifdef MACOSX
  bool shortcut_key = any(modifierkeys & ModifierKeys::kSuper);
#else
  bool shortcut_key = any(modifierkeys & ModifierKeys::kCtrl);
#endif
  if (!Element::focused_element || !down || !shortcut_key) return false;
  bool reverse_key = any(modifierkeys & ModifierKeys::kShift);
  int upper_key = toupr_ascii(key);
  TBID id;
  if (upper_key == 'X')
    id = TBIDC("cut");
  else if (upper_key == 'C' || special_key == SpecialKey::kInsert)
    id = TBIDC("copy");
  else if (upper_key == 'V' ||
           (special_key == SpecialKey::kInsert && reverse_key))
    id = TBIDC("paste");
  else if (upper_key == 'A')
    id = TBIDC("selectall");
  else if (upper_key == 'Z' || upper_key == 'Y') {
    bool undo = upper_key == 'Z';
    if (reverse_key) undo = !undo;
    id = undo ? TBIDC("undo") : TBIDC("redo");
  } else if (upper_key == 'N')
    id = TBIDC("new");
  else if (upper_key == 'O')
    id = TBIDC("open");
  else if (upper_key == 'S')
    id = TBIDC("save");
  else if (upper_key == 'W')
    id = TBIDC("close");
  else if (special_key == SpecialKey::kPageUp)
    id = TBIDC("prev_doc");
  else if (special_key == SpecialKey::kPageDown)
    id = TBIDC("next_doc");
  else
    return false;

  ElementEvent ev(EventType::kShortcut);
  ev.modifierkeys = modifierkeys;
  ev.ref_id = id;
  return Element::focused_element->InvokeEvent(ev);
}

static bool InvokeKey(GLFWwindow* window, unsigned int key,
                      SpecialKey special_key, ModifierKeys modifierkeys,
                      bool down) {
  if (InvokeShortcut(key, special_key, modifierkeys, down)) return true;
  GetBackend(window)->GetRoot()->InvokeKey(key, special_key, modifierkeys,
                                           down);
  return true;
}

static void char_callback(GLFWwindow* window, unsigned int character) {
  // glfw on osx seems to send us characters from the private
  // use block when using f.ex arrow keys on osx.
  if (character >= 0xE000 && character <= 0xF8FF) return;

  InvokeKey(window, character, SpecialKey::kUndefined, GetModifierKeys(), true);
  InvokeKey(window, character, SpecialKey::kUndefined, GetModifierKeys(),
            false);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int glfwmod) {
  ModifierKeys modifier = GetModifierKeys(glfwmod);
  bool down = (action == GLFW_PRESS || action == GLFW_REPEAT);
  switch (key) {
    case GLFW_KEY_F1:
      InvokeKey(window, 0, SpecialKey::kF1, modifier, down);
      break;
    case GLFW_KEY_F2:
      InvokeKey(window, 0, SpecialKey::kF2, modifier, down);
      break;
    case GLFW_KEY_F3:
      InvokeKey(window, 0, SpecialKey::kF3, modifier, down);
      break;
    case GLFW_KEY_F4:
      InvokeKey(window, 0, SpecialKey::kF4, modifier, down);
      break;
    case GLFW_KEY_F5:
      InvokeKey(window, 0, SpecialKey::kF5, modifier, down);
      break;
    case GLFW_KEY_F6:
      InvokeKey(window, 0, SpecialKey::kF6, modifier, down);
      break;
    case GLFW_KEY_F7:
      InvokeKey(window, 0, SpecialKey::kF7, modifier, down);
      break;
    case GLFW_KEY_F8:
      InvokeKey(window, 0, SpecialKey::kF8, modifier, down);
      break;
    case GLFW_KEY_F9:
      InvokeKey(window, 0, SpecialKey::kF9, modifier, down);
      break;
    case GLFW_KEY_F10:
      InvokeKey(window, 0, SpecialKey::kF10, modifier, down);
      break;
    case GLFW_KEY_F11:
      InvokeKey(window, 0, SpecialKey::kF11, modifier, down);
      break;
    case GLFW_KEY_F12:
      InvokeKey(window, 0, SpecialKey::kF12, modifier, down);
      break;
    case GLFW_KEY_LEFT:
      InvokeKey(window, 0, SpecialKey::kLeft, modifier, down);
      break;
    case GLFW_KEY_UP:
      InvokeKey(window, 0, SpecialKey::kUp, modifier, down);
      break;
    case GLFW_KEY_RIGHT:
      InvokeKey(window, 0, SpecialKey::kRight, modifier, down);
      break;
    case GLFW_KEY_DOWN:
      InvokeKey(window, 0, SpecialKey::kDown, modifier, down);
      break;
    case GLFW_KEY_PAGE_UP:
      InvokeKey(window, 0, SpecialKey::kPageUp, modifier, down);
      break;
    case GLFW_KEY_PAGE_DOWN:
      InvokeKey(window, 0, SpecialKey::kPageDown, modifier, down);
      break;
    case GLFW_KEY_HOME:
      InvokeKey(window, 0, SpecialKey::kHome, modifier, down);
      break;
    case GLFW_KEY_END:
      InvokeKey(window, 0, SpecialKey::kEnd, modifier, down);
      break;
    case GLFW_KEY_INSERT:
      InvokeKey(window, 0, SpecialKey::kInsert, modifier, down);
      break;
    case GLFW_KEY_TAB:
      InvokeKey(window, 0, SpecialKey::kTab, modifier, down);
      break;
    case GLFW_KEY_DELETE:
      InvokeKey(window, 0, SpecialKey::kDelete, modifier, down);
      break;
    case GLFW_KEY_BACKSPACE:
      InvokeKey(window, 0, SpecialKey::kBackspace, modifier, down);
      break;
    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
      InvokeKey(window, 0, SpecialKey::kEnter, modifier, down);
      break;
    case GLFW_KEY_ESCAPE:
      InvokeKey(window, 0, SpecialKey::kEsc, modifier, down);
      break;
    case GLFW_KEY_MENU:
      if (Element::focused_element && !down) {
        ElementEvent ev(EventType::kContextMenu);
        ev.modifierkeys = modifier;
        Element::focused_element->InvokeEvent(ev);
      }
      break;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
      key_shift = down;
      break;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
      key_ctrl = down;
      break;
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_RIGHT_ALT:
      key_alt = down;
      break;
    case GLFW_KEY_LEFT_SUPER:
    case GLFW_KEY_RIGHT_SUPER:
      key_super = down;
      break;
    default:
// GLFW has some platform issues with keyboard handling that needs workaround
// until
// it's fixed properly in glfw. This code must not run on linux with glfw 3.0.4
// (it
// has changed several times in the past). issue #10.
#ifndef TB_TARGET_LINUX
      // glfw calls key_callback instead of char_callback
      // when pressing a character while ctrl is also pressed.
      if (key_ctrl && !key_alt && key >= 32 && key <= 255)
        InvokeKey(window, key, SpecialKey::kUndefined, modifier, down);
#endif
      break;
  }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                  int glfwmod) {
  ModifierKeys modifier = GetModifierKeys(glfwmod);
  int x = mouse_x;
  int y = mouse_y;
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      // This is a quick fix with n-click support :)
      static uint64_t last_time = 0;
      static int last_x = 0;
      static int last_y = 0;
      static int counter = 1;

      uint64_t time = TBSystem::GetTimeMS();
      if (time < last_time + 600 && last_x == x && last_y == y)
        counter++;
      else
        counter = 1;
      last_x = x;
      last_y = y;
      last_time = time;

      GetBackend(window)->GetRoot()->InvokePointerDown(
          x, y, counter, modifier, ShouldEmulateTouchEvent());
    } else
      GetBackend(window)->GetRoot()->InvokePointerUp(x, y, modifier,
                                                     ShouldEmulateTouchEvent());
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
    GetBackend(window)->GetRoot()->InvokePointerMove(x, y, modifier,
                                                     ShouldEmulateTouchEvent());
    if (Element::hovered_element) {
      Element::hovered_element->ConvertFromRoot(x, y);
      ElementEvent ev(EventType::kContextMenu, x, y, false, modifier);
      Element::hovered_element->InvokeEvent(ev);
    }
  }
}

void cursor_position_callback(GLFWwindow* window, double x, double y) {
  mouse_x = (int)x;
  mouse_y = (int)y;
  if (GetBackend(window)->GetRoot() &&
      !(ShouldEmulateTouchEvent() && !Element::captured_element))
    GetBackend(window)->GetRoot()->InvokePointerMove(
        mouse_x, mouse_y, GetModifierKeys(), ShouldEmulateTouchEvent());
}

static void scroll_callback(GLFWwindow* window, double x, double y) {
  if (GetBackend(window)->GetRoot())
    GetBackend(window)->GetRoot()->InvokeWheel(mouse_x, mouse_y, (int)x,
                                               -(int)y, GetModifierKeys());
}

/** Reschedule the platform timer, or cancel it if fire_time is kNotSoon.
        If fire_time is 0, it should be fired ASAP.
        If force is true, it will ask the platform to schedule it again, even if
        the fire_time is the same as last time. */

#ifndef TB_TARGET_LINUX
static void ReschedulePlatformTimer(uint64_t fire_time, bool force) {
  static uint64_t set_fire_time = -1;
  if (fire_time == kNotSoon) {
    set_fire_time = -1;
    glfwKillTimer();
  } else if (fire_time != set_fire_time || force || fire_time == 0) {
    set_fire_time = fire_time;
    auto now = tb::TBSystem::GetTimeMS();
    if (fire_time < now) {
      glfwRescheduleTimer(0);
    } else {
      uint64_t delay = fire_time - now;
      unsigned int idelay = (unsigned int)std::max(delay, 0ull);
      glfwRescheduleTimer(idelay);
    }
  }
}

static void timer_callback() {
  uint64_t next_fire_time = MessageHandler::GetNextMessageFireTime();
  uint64_t now = tb::TBSystem::GetTimeMS();
  if (now < next_fire_time) {
    // We timed out *before* we were supposed to (the OS is not playing nice).
    // Calling ProcessMessages now won't achieve a thing so force a reschedule
    // of the platform timer again with the same time.
    ReschedulePlatformTimer(next_fire_time, true);
    return;
  }

  MessageHandler::ProcessMessages();

  // If we still have things to do (because we didn't process all messages,
  // or because there are new messages), we need to rescedule, so call
  // RescheduleTimer.
  TBSystem::RescheduleTimer(MessageHandler::GetNextMessageFireTime());
}

// This doesn't really belong here (it belongs in tb_system_[linux/windows].cpp.
// This is here since the proper implementations has not yet been done.
void TBSystem::RescheduleTimer(uint64_t fire_time) {
  ReschedulePlatformTimer(fire_time, false);
}
#endif /* TB_TARGET_LINUX */

static void window_refresh_callback(GLFWwindow* window) {
  ApplicationBackendGLFW* backend = GetBackend(window);

  backend->m_application->Process();

  backend->has_pending_update = false;

  // Bail out if we get here with invalid dimensions.
  // This may happen when minimizing windows (GLFW 3.0.4, Windows 8.1).
  if (backend->width() == 0 || backend->height() == 0) return;

  backend->m_application->RenderFrame(backend->width(),
                                      backend->height());

  glfwSwapBuffers(window);
}

static void window_size_callback(GLFWwindow* window, int w, int h) {
  ApplicationBackendGLFW* backend = GetBackend(window);
  if (backend->GetRoot()) backend->GetRoot()->set_rect({0, 0, w, h});
}

#if (GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1)
static void drop_callback(GLFWwindow* window, int count,
                          const char** files_utf8) {
  ApplicationBackendGLFW* backend = GetBackend(window);
  Element* target = Element::hovered_element;
  if (!target) target = Element::focused_element;
  if (!target) target = backend->GetRoot();
  if (target) {
    ElementEventFileDrop ev;
    for (int i = 0; i < count; i++)
      ev.files.Add(new std::string(files_utf8[i]));
    target->InvokeEvent(ev);
  }
}
#endif

// static
ApplicationBackend* ApplicationBackend::Create(Application* app, int width,
                                               int height, const char* title) {
  ApplicationBackendGLFW* backend = new ApplicationBackendGLFW();
  if (backend->Init(app, width, height, title)) {
    return backend;
  }
  delete backend;
  return nullptr;
}

bool ApplicationBackendGLFW::Init(Application* app, int width, int height,
                                  const char* title) {
  if (!glfwInit()) return false;
  mainWindow = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!mainWindow) {
    glfwTerminate();
    return false;
  }
  SetBackend(mainWindow, this);
  glfwMakeContextCurrent(mainWindow);

  // Ensure we can capture the escape key being pressed below
  // glfwSetInputMode(mainWindow, GLFW_STICKY_KEYS, GL_TRUE);
  // glfwSetInputMode(mainWindow, GLFW_SYSTEM_KEYS, GL_TRUE);
  // glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  // Set callback functions
  glfwSetWindowSizeCallback(mainWindow, window_size_callback);
  glfwSetWindowRefreshCallback(mainWindow, window_refresh_callback);
  glfwSetCursorPosCallback(mainWindow, cursor_position_callback);
  glfwSetMouseButtonCallback(mainWindow, mouse_button_callback);
  glfwSetScrollCallback(mainWindow, scroll_callback);
  glfwSetKeyCallback(mainWindow, key_callback);
  glfwSetCharCallback(mainWindow, char_callback);

#ifndef TB_SYSTEM_LINUX
  glfwSetTimerCallback(timer_callback);
#endif

#if (GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1)
  glfwSetDropCallback(mainWindow, drop_callback);
#endif

  m_renderer = new RendererGL();
  m_root.set_rect({0, 0, width, height});

  // Create the application object for our demo

  m_application = app;
  m_application->OnBackendAttached(this);

#if !defined(TB_USE_CURRENT_DIRECTORY) && defined(TB_TARGET_MACOSX)
  // Put is in the root of the repository so the demo resources are found.
  char exec_path[2048];
  uint32_t exec_path_size = sizeof(exec_path);
  if (_NSGetExecutablePath(exec_path, &exec_path_size) == 0) {
    StringBuilder path;
    path.AppendPath(exec_path);
    chdir(path.GetData());
  }
// For linux?
// chdir("../");
#endif

  return true;
}

ApplicationBackendGLFW::~ApplicationBackendGLFW() {
  m_application->OnBackendDetached();
  m_application = nullptr;

  tb_core_shutdown();

  glfwTerminate();

  delete m_renderer;
}

void ApplicationBackendGLFW::Run() {
  do {
#ifdef TB_TARGET_LINUX
    TBSystem::PollEvents();

#endif
    glfwPollEvents();

    if (has_pending_update) window_refresh_callback(mainWindow);

  } while (!glfwWindowShouldClose(mainWindow));
}

#ifdef TB_TARGET_WINDOWS

#define NOMINMAX
#include <mmsystem.h>
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
#if !defined(TB_USE_CURRENT_DIRECTORY)
  // Set the current path to the directory of the app so we find assets also
  // when visual studio start it.
  char modname[MAX_PATH];
  GetModuleFileName(NULL, modname, MAX_PATH);
  StringBuilder buf;
  buf.AppendPath(modname);
  SetCurrentDirectory(buf.GetData());
#endif

  // Crank up windows timer resolution (it's awfully low res normally). Note:
  // This affects battery time!
  timeBeginPeriod(1);
  int ret = app_main();
  timeEndPeriod(1);
  return ret;
}

#else  // TB_TARGET_WINDOWS

int main(int argc, char** argv) { return app_main(); }

#endif  // !TB_TARGET_WINDOWS
