#include "Application.h"

#include <cstdio>

#include "tb_system.h"
#include "tb_widget_animation.h"

using namespace tb;

void Application::Run() { m_backend->Run(); }

bool Application::Init() {
  ElementAnimationManager::Init();

  return true;
}

void Application::ShutDown() {
  ElementAnimationManager::Shutdown();
  delete m_backend;
  m_backend = nullptr;
}

void Application::Process() {
  AnimationManager::Update();
  GetRoot()->InvokeProcessStates();
  GetRoot()->InvokeProcess();
}

void Application::RenderFrame(int window_w, int window_h) {
  g_renderer->BeginPaint(window_w, window_h);
  GetRoot()->InvokePaint(Element::PaintProps());
  g_renderer->EndPaint();

  // If animations are running, reinvalidate immediately
  if (AnimationManager::HasAnimationsRunning()) GetRoot()->Invalidate();
}
