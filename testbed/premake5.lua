project_root = ".."
dofile(project_root.."/build_tools/scripts/platform_files.lua")

group("samples")
project("elemental-forms-testbed")
  uuid("d16c6513-eaeb-4a69-b0d0-009a846ba895")
  kind("WindowedApp")
  language("C++")
  links({
    "elemental-forms",
  })
  flags({
    "WinMain",  -- Use WinMain instead of main.
  })
  includedirs({
    project_root,
    project_root.."/src",
    project_root.."/third_party",
    project_root.."/third_party/glfw/include",
  })
  recursive_platform_files()
  files({
    "testbed_resources.rc",
  })
  resincludedirs({
    project_root,
  })
  debugdir(project_root)

  -- GLFW third_party dep.
  local glfw = project_root.."/third_party/glfw/src"
  files({
    glfw.."/context.c",
    glfw.."/init.c",
    glfw.."/input.c",
    glfw.."/monitor.c",
    glfw.."/window.c",
  })

  filter("platforms:Windows")
    defines({
      "_GLFW_USE_OPENGL",
      "_GLFW_WGL",
      "GLFW_EXPOSE_NATIVE_WIN32",
      "GLFW_EXPOSE_NATIVE_WGL",
      "_GLFW_WIN32",
      "_WINDOWS",
    })
    links({
      "winmm",
    })
    files({
      glfw.."/wgl_context.c",
      glfw.."/win32_init.c",
      glfw.."/win32_monitor.c",
      glfw.."/win32_time.c",
      glfw.."/win32_tls.c",
      glfw.."/win32_platform.h",
      glfw.."/win32_window.c",
      glfw.."/winmm_joystick.c",
    })
