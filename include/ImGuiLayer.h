#ifndef IMGUI_LAYER_H
#define IMGUI_LAYER_H

struct GLFWwindow;

class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    bool Initialize(GLFWwindow* window, const char* glslVersion = "#version 330");
    void BeginFrame();
    void EndFrame();
    void Shutdown();

    bool IsInitialized() const { return m_Initialized; }

private:
    bool m_Initialized = false;
};

#endif