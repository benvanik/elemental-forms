#ifndef APPLICATION_H
#define APPLICATION_H

#include "tb_system.h"
#include "tb_widget_animation.h"

/** Called from the platform main function. */
int app_main();

class Application;

/** Backend interface that handles platform window & creating the renderer. */
class ApplicationBackend {
 public:
  static ApplicationBackend* Create(Application* app, int width, int height,
                                    const char* title);
  virtual ~ApplicationBackend() {}
  virtual void Run() = 0;
  virtual tb::TBWidget* GetRoot() = 0;
  virtual tb::TBRenderer* GetRenderer() = 0;
};

/** Application interface, for setting up the application using turbo badger. */
class Application {
 public:
  Application() {}
  virtual ~Application() {}

  tb::TBWidget* GetRoot() { return m_backend->GetRoot(); }

  /** Run the message loop. */
  void Run();

  virtual void OnBackendAttached(ApplicationBackend* backend) {
    m_backend = backend;
  }
  virtual void OnBackendDetached() { m_backend = nullptr; }

  virtual bool Init();
  virtual void ShutDown();
  virtual void Process();
  virtual void RenderFrame(int window_w, int window_h);

 protected:
  ApplicationBackend* m_backend;
};

#endif  // APPLICATION_H
