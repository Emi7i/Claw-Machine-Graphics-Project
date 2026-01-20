#pragma once
#include "model.hpp"

class GameObject {
private:
    Model* model;
    glm::mat4 transform;
    std::vector<GameObject*> children;

public:
    GameObject(const char* path) {
        model = new Model(path);
        transform = glm::mat4(1.0f);
    }

    ~GameObject() {
        delete model;
        for (auto child : children) {
            delete child;
        }
    }

    void AddChild(GameObject* child) {
        children.push_back(child);
    }

    void Rotate(float angle, glm::vec3 axis) {
        transform = glm::rotate(transform, glm::radians(angle), axis);
    }

    void Translate(glm::vec3 position) {
        transform = glm::translate(transform, position);
    }

    void Scale(glm::vec3 scale) {
        transform = glm::scale(transform, scale);
    }

    void Draw(Shader& shader) {
        shader.setMat4("uM", transform);
        model->Draw(shader);

        // Draw children with parent's transform applied
        for (auto child : children) {
            glm::mat4 childTransform = transform * child->GetTransform();
            shader.setMat4("uM", childTransform);
            child->model->Draw(shader);
        }
    }

    glm::mat4 GetTransform() const {
        return transform;
    }
};