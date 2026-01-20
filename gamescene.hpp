#pragma once
#include <vector>
#include "gameobject.hpp"
#include "shader.hpp"

class GameScene {
private:
    std::vector<GameObject> gameObjects;
public:
    void Draw(Shader& shader) {
        for (auto& obj : gameObjects) {
            obj.Draw(shader);
        }
    }

    void AddGameObject(GameObject obj) {
        gameObjects.push_back(obj);
    }

    GameObject& GetGameObject(int index) {
        return gameObjects[index];
    }
};