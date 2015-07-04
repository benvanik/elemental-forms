#include "Application.h"

#include <cstdio>

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
  Renderer::get()->BeginPaint(window_w, window_h);
  GetRoot()->InvokePaint(Element::PaintProps());
  Renderer::get()->EndPaint();

  // If animations are running, reinvalidate immediately
  if (AnimationManager::HasAnimationsRunning()) GetRoot()->Invalidate();
}
