#include "Application.h"

#include <cstdio>

#include "tb/animation_manager.h"

using namespace tb;

void Application::Run() { m_backend->Run(); }

bool Application::Init() { return true; }

void Application::ShutDown() {
  delete m_backend;
  m_backend = nullptr;
}

void Application::Process() {
  AnimationManager::Update();
  GetRoot()->InvokeProcessStates();
  GetRoot()->InvokeProcess();
}

void Application::RenderFrame(int window_w, int window_h) {
  graphics::Renderer::get()->BeginPaint(window_w, window_h);
  GetRoot()->InvokePaint(Element::PaintProps());
  graphics::Renderer::get()->EndPaint();

  // If animations are running, reinvalidate immediately
  if (AnimationManager::has_running_animations()) GetRoot()->Invalidate();
}
