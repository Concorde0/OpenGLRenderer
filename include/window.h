#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window {
public:
    Window();
    ~Window();
    
    GLFWwindow* Initialize();
    GLFWwindow* GetWindow() const;
    void ProcessInput();
    bool ShouldClose() const;
    void SwapBuffers() const;
    void PollEvents() const;
    
    static Window* instance;
    static const unsigned int SCR_WIDTH;
    static const unsigned int SCR_HEIGHT;
    
private:
    GLFWwindow* window;
    
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

#endif