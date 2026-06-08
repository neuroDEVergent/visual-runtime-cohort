#include "visual_runtime_module.h"
#include "input.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__linux__)
#include <GLFW/glfw3native.h>
#if defined(VRT_GLFW_HAS_NATIVE_X11)
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#endif
#endif

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

struct AppState {
  VisualRuntimeModule *runtime = nullptr;
};

Input input{0};

void glfw_error(int code, const char *description) {
  std::fprintf(stderr, "[glfw-minimal] GLFW error %d: %s\n", code,
               description ? description : "unknown");
}

bool env_set(const char *name) {
  const char *value = std::getenv(name);
  return value && value[0] != '\0';
}

void framebuffer_resized(GLFWwindow *window, int width, int height) {
  auto *state = static_cast<AppState *>(glfwGetWindowUserPointer(window));
  if (!state || !state->runtime) {
    return;
  }

  state->runtime->resize(static_cast<uint32_t>(width),
                         static_cast<uint32_t>(height));
  std::fprintf(stderr, "[glfw-minimal] resized to %dx%d\n", width, height);
}

void scroll_callback(GLFWwindow *window, double dx, double dy) {
  input.SCROLL_Y += dy;
  if (input.SCROLL_Y < 1.0)
    input.SCROLL_Y = 1.0;
  if (input.SCROLL_Y > 30.0)
    input.SCROLL_Y = 30.0;
}

void mouse_callback(GLFWwindow *window, double x, double y) {
  int width, height;
  glfwGetWindowSize(window, &width, &height);
  
  input.MOUSE_X = x;
  input.MOUSE_Y = y;
  input.SCREEN_WIDTH = width;
  input.SCREEN_HEIGHT = height;
}

void click_callback(GLFWwindow *window, int button, int action, int mods) {

  // Pressed action for left and right click
  if (action == GLFW_PRESS) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      input.LCLICK_DOWN = true;
      input.LCLICK_PRESSED = true;
    }

    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      input.RCLICK_DOWN = true;
      input.RCLICK_PRESSED = true;
    }
  }

  // Released action for left and right click
  if (action == GLFW_RELEASE) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      input.LCLICK_DOWN = false;
      input.LCLICK_RELEASED = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      input.RCLICK_DOWN = false;
      input.RCLICK_RELEASED = true;
    }
  }
}


#if defined(__linux__)
#if defined(VRT_GLFW_HAS_NATIVE_WAYLAND)
bool attach_wayland_surface(GLFWwindow *window, VisualRuntimeModule &runtime) {
  wl_display *display = glfwGetWaylandDisplay();
  wl_surface *wayland_surface = glfwGetWaylandWindow(window);
  if (!display || !wayland_surface) {
    std::fprintf(stderr,
                 "[glfw-minimal] failed to get Wayland surface handles\n");
    return false;
  }

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);

  SurfaceDescriptor surface{
      SurfaceKind::LinuxWaylandSurface,
      display,
      reinterpret_cast<uintptr_t>(wayland_surface),
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
  };
  std::fprintf(stderr,
               "[glfw-minimal] attaching LinuxWaylandSurface surface (%ux%u)\n",
               surface.width, surface.height);
  runtime.attachSurface(surface);
  return true;
}
#endif

#if defined(VRT_GLFW_HAS_NATIVE_X11)
bool attach_xcb_surface(GLFWwindow *window, VisualRuntimeModule &runtime) {
  Display *display = glfwGetX11Display();
  if (!display) {
    std::fprintf(stderr, "[glfw-minimal] failed to get XCB surface handles\n");
    return false;
  }

  Window x11_window = glfwGetX11Window(window);
  xcb_connection_t *connection = XGetXCBConnection(display);
  if (x11_window == 0 || !connection || xcb_connection_has_error(connection)) {
    std::fprintf(stderr, "[glfw-minimal] failed to get XCB surface handles\n");
    return false;
  }

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);

  SurfaceDescriptor surface{
      SurfaceKind::LinuxXcbWindow,        connection,
      static_cast<uintptr_t>(x11_window), static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
  };
  std::fprintf(stderr,
               "[glfw-minimal] attaching LinuxXcbWindow surface (%ux%u)\n",
               surface.width, surface.height);
  runtime.attachSurface(surface);
  return true;
}
#endif

bool attach_surface(GLFWwindow *window, VisualRuntimeModule &runtime) {
#if defined(VRT_GLFW_HAS_PLATFORM_API)
  const int platform = glfwGetPlatform();
#if defined(VRT_GLFW_HAS_NATIVE_WAYLAND)
  if (platform == GLFW_PLATFORM_WAYLAND) {
    return attach_wayland_surface(window, runtime);
  }
#endif

#if defined(VRT_GLFW_HAS_NATIVE_X11)
  if (platform == GLFW_PLATFORM_X11) {
    return attach_xcb_surface(window, runtime);
  }
#endif

  std::fprintf(stderr, "[glfw-minimal] unsupported GLFW platform: %d\n",
               platform);
  return false;
#else
  const bool has_wayland_display = env_set("WAYLAND_DISPLAY");
  const bool has_x11_display = env_set("DISPLAY");
#if defined(VRT_GLFW_HAS_NATIVE_WAYLAND)
  if (has_wayland_display && attach_wayland_surface(window, runtime)) {
    return true;
  }
#endif

#if defined(VRT_GLFW_HAS_NATIVE_X11)
  if (has_x11_display && attach_xcb_surface(window, runtime)) {
    return true;
  }
#endif

#if defined(VRT_GLFW_HAS_NATIVE_WAYLAND)
  if (!has_wayland_display && attach_wayland_surface(window, runtime)) {
    return true;
  }
#endif

#if defined(VRT_GLFW_HAS_NATIVE_X11)
  if (!has_x11_display && attach_xcb_surface(window, runtime)) {
    return true;
  }
#endif

  std::fprintf(
      stderr,
      "[glfw-minimal] could not attach Wayland or XCB surface handles\n");
  return false;
#endif
}
#else
bool attach_surface(GLFWwindow *window, VisualRuntimeModule &runtime) {
  (void)window;
  (void)runtime;
  std::fprintf(stderr,
               "[glfw-minimal] native surface handles are not implemented for "
               "this platform\n");
  return false;
}
#endif
} // namespace

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  glfwSetErrorCallback(glfw_error);

  if (!glfwInit()) {
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "Visual Runtime", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }

  VisualRuntimeModule runtime =
      VisualRuntimeModule::open(VISUAL_RUNTIME_LIB_PATH);
  if (!runtime) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  AppState state{&runtime};
  glfwSetWindowUserPointer(window, &state);
  glfwSetFramebufferSizeCallback(window, framebuffer_resized);

  if (!attach_surface(window, runtime)) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  using clock = std::chrono::steady_clock;
  auto last = clock::now();

  // GLFW callback function setup
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetMouseButtonCallback(window, click_callback);
 
  while (!glfwWindowShouldClose(window)) {
    if (runtime.reloadIfChanged()) {
      std::printf("[host] reloaded (frame %llu)\n", runtime.frameCount());
    }

    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    // reset the flags
    input.LCLICK_PRESSED = false;
    input.LCLICK_RELEASED = false;
    input.RCLICK_PRESSED = false;
    input.RCLICK_RELEASED = false;
    glfwPollEvents();
  
    runtime.tick(&input, dt);
  }

  std::printf("[glfw-minimal] exiting after %llu frames\n",
              runtime.frameCount());
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
