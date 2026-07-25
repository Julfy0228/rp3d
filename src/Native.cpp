#include "Native.h"

#if defined(_WIN32)
  #include <dwmapi.h>
  #pragma comment(lib, "dwmapi.lib")
  #define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3native.h>

void SetDarkTitlebar(GLFWwindow* window, bool darkMode)
{
#if defined(_WIN32)
    HWND hwnd = glfwGetWin32Window(window);
    if (hwnd)
    {
        BOOL useDarkMode = darkMode ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    }
#endif
}