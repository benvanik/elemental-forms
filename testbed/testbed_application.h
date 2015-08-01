/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * ©2015 Ben Vanik. All rights reserved. Released under the BSD license.      *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#ifndef TESTBED_APPLICATION_H_
#define TESTBED_APPLICATION_H_

#include "el/config.h"
#include "el/elements.h"
#include "el/elements/parts/scroller.h"

namespace testbed {

using namespace el;

class TestbedApplication;

// Called from the platform main function.
int app_main();

// Backend interface that handles platform window & creating the renderer.
class ApplicationBackend {
 public:
  static ApplicationBackend* Create(TestbedApplication* app, int width,
                                    int height, const char* title);
  virtual ~ApplicationBackend() = default;
  virtual void Run() = 0;
  virtual Element* GetRoot() = 0;
  virtual graphics::Renderer* GetRenderer() = 0;
};

// Application interface, for setting up the application using elemental.
class TestbedApplication {
 public:
  TestbedApplication() = default;
  virtual ~TestbedApplication() = default;

  Element* GetRoot() { return m_backend->GetRoot(); }

  // Runs the message loop. Does not return until exiting.
  void Run();

  bool Init();
  void ShutDown();
  void Process();
  void RenderFrame(int window_w, int window_h);

  void OnBackendAttached(ApplicationBackend* backend) { m_backend = backend; }
  void OnBackendDetached() { m_backend = nullptr; }

 private:
  ApplicationBackend* m_backend;
};

class DemoWindow : public elements::Form {
 public:
  DemoWindow();
  bool LoadResourceFile(const std::string& filename);
  void LoadResourceData(const char* data);
  void LoadResource(parsing::ParseNode& node);

  virtual bool OnEvent(const Event& ev);
};

class MainWindow : public DemoWindow, public MessageHandler {
 public:
  MainWindow();
  virtual bool OnEvent(const Event& ev);

  // Implement MessageHandler
  virtual void OnMessageReceived(Message* msg);
};

class ImageWindow : public DemoWindow {
 public:
  ImageWindow();
  virtual bool OnEvent(const Event& ev);
};

class PageWindow : public DemoWindow,
                   public el::elements::parts::ScrollerSnapListener {
 public:
  PageWindow();
  virtual bool OnEvent(const Event& ev);
  virtual void OnScrollSnap(Element* target_element, int& target_x,
                            int& target_y);
};

class AnimationsWindow : public DemoWindow {
 public:
  AnimationsWindow();
  void Animate();
  virtual bool OnEvent(const Event& ev);
};

class EditWindow : public DemoWindow {
 public:
  EditWindow();
  void OnProcessStates() override;
  bool OnEvent(const Event& ev) override;
};

class LayoutWindow : public DemoWindow {
 public:
  LayoutWindow(const std::string& filename);
  virtual bool OnEvent(const Event& ev);
};

class TabContainerWindow : public DemoWindow {
 public:
  TabContainerWindow();
  virtual bool OnEvent(const Event& ev);
};

class ConnectionWindow : public DemoWindow {
 public:
  ConnectionWindow();
  virtual bool OnEvent(const Event& ev);
};

class ScrollContainerWindow : public DemoWindow, public MessageHandler {
 public:
  ScrollContainerWindow();
  virtual bool OnEvent(const Event& ev);

  // Implement MessageHandler
  virtual void OnMessageReceived(Message* msg);
};

}  // namespace testbed

#endif  // TESTBED_APPLICATION_H_
