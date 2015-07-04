#ifndef DEMO_H
#define DEMO_H

#include "tb_widgets.h"
#include "tb_widgets_common.h"
#include "tb_widgets_listener.h"
#include "tb_message_window.h"
#include "tb_msg.h"
#include "tb_scroller.h"
#include "platform/Application.h"

using namespace tb;

class DemoApplication : public Application {
 public:
  DemoApplication() : Application() {}

  virtual bool Init();
  virtual void RenderFrame(int window_w, int window_h);
};

class DemoWindow : public Window {
 public:
  DemoWindow();
  bool LoadResourceFile(const std::string& filename);
  void LoadResourceData(const char* data);
  void LoadResource(Node& node);

  virtual bool OnEvent(const ElementEvent& ev);
};

class MainWindow : public DemoWindow, public MessageHandler {
 public:
  MainWindow();
  virtual bool OnEvent(const ElementEvent& ev);

  // Implement MessageHandler
  virtual void OnMessageReceived(Message* msg);
};

class ImageWindow : public DemoWindow {
 public:
  ImageWindow();
  virtual bool OnEvent(const ElementEvent& ev);
};

class PageWindow : public DemoWindow, public ScrollerSnapListener {
 public:
  PageWindow();
  virtual bool OnEvent(const ElementEvent& ev);
  virtual void OnScrollSnap(Element* target_element, int& target_x,
                            int& target_y);
};

class AnimationsWindow : public DemoWindow {
 public:
  AnimationsWindow();
  void Animate();
  virtual bool OnEvent(const ElementEvent& ev);
};

class LayoutWindow : public DemoWindow {
 public:
  LayoutWindow(const std::string& filename);
  virtual bool OnEvent(const ElementEvent& ev);
};

class TabContainerWindow : public DemoWindow {
 public:
  TabContainerWindow();
  virtual bool OnEvent(const ElementEvent& ev);
};

class ConnectionWindow : public DemoWindow {
 public:
  ConnectionWindow();
  virtual bool OnEvent(const ElementEvent& ev);
};

class ScrollContainerWindow : public DemoWindow, public MessageHandler {
 public:
  ScrollContainerWindow();
  virtual bool OnEvent(const ElementEvent& ev);

  // Implement MessageHandler
  virtual void OnMessageReceived(Message* msg);
};

#endif  // DEMO_H
