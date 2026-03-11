#ifndef GLFW3NATIVE_H
#define GLFW3NATIVE_H

#include <GLFW/glfw3.h>

#if defined(_WIN32)
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

GLFWAPI HWND glfwGetWin32Window(GLFWwindow* window);

#ifdef __cplusplus
}
#endif
#endif

#endif