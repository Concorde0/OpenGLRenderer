#ifndef SCENE_NODE_H
#define SCENE_NODE_H

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

#include "Model.h"

class SceneNode {
public:
    SceneNode(std::string name = "Node", const glm::mat4& localTransform = glm::mat4(1.0f), Model* model = nullptr)
        : m_Name(std::move(name)),
          m_LocalTransform(localTransform),
          m_Model(model)
    {
    }

    SceneNode* CreateChild(const std::string& name,
                           const glm::mat4& localTransform = glm::mat4(1.0f),
                           Model* model = nullptr)
    {
        std::unique_ptr<SceneNode> child = std::make_unique<SceneNode>(name, localTransform, model);
        SceneNode* raw = child.get();
        m_Children.push_back(std::move(child));
        return raw;
    }

    void SetLocalTransform(const glm::mat4& transform)
    {
        m_LocalTransform = transform;
    }

    const glm::mat4& GetLocalTransform() const
    {
        return m_LocalTransform;
    }

    void SetModel(Model* model)
    {
        m_Model = model;
    }

    void Draw(const Shader& shader, const glm::mat4& parentTransform = glm::mat4(1.0f)) const
    {
        const glm::mat4 worldTransform = parentTransform * m_LocalTransform;

        if (m_Model != nullptr) {
            m_Model->Draw(shader, worldTransform);
        }

        for (const std::unique_ptr<SceneNode>& child : m_Children) {
            child->Draw(shader, worldTransform);
        }
    }

private:
    std::string m_Name;
    glm::mat4 m_LocalTransform;
    Model* m_Model;
    std::vector<std::unique_ptr<SceneNode>> m_Children;
};

#endif