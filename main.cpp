#pragma once
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "opengl32.lib")  // Added so Rider will stop complaining
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <reactphysics3d/reactphysics3d.h>
#include "gameobject.hpp"

const unsigned int wWidth = 800;
const unsigned int wHeight = 600;

#define GROUND_HEIGHT 0.2f

GameObject* claw;
GameObject* ground;

// Physics objects
rp3d::PhysicsCommon physicsCommon;
rp3d::PhysicsWorld* physicsWorld = nullptr;

void InitializeGameObjects();
void MoveCamera(GLFWwindow* window, glm::mat4& viewMatrix, Shader shader, double deltaTime);

int main()
{
    // ------------------------- INIT -------------------------
    if (!glfwInit())
    {
        std::cout << "GLFW fail!\n" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(wWidth, wHeight, "Physics Demo with Mesh Collision", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Window fail!\n" << std::endl;
        glfwTerminate();
        return -2;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW fail! :(\n" << std::endl;
        return -3;
    }

    // Create physics world
    rp3d::PhysicsWorld::WorldSettings settings;
    settings.gravity = rp3d::Vector3(0.0f, -9.81f, 0.0f);
    physicsWorld = physicsCommon.createPhysicsWorld(settings);
    std::cout << "Physics world created!" << std::endl;

    InitializeGameObjects();

    Shader unifiedShader("basic.vert", "basic.frag");
    unifiedShader.use();
    unifiedShader.setVec3("uLightPos", 0, 1, 3);
    unifiedShader.setVec3("uViewPos", 0, 0, 5);
    unifiedShader.setVec3("uLightColor", 1, 1, 1);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)wWidth / (float)wHeight, 0.1f, 100.0f);
    unifiedShader.setMat4("uP", projection);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    unifiedShader.setMat4("uV", view);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double timeLast = glfwGetTime();
    double deltaTime = 0.0;

    // ------------------------- MAIN LOOP -------------------------
    while (!glfwWindowShouldClose(window))
    {
        double timeNow = glfwGetTime();
        deltaTime = timeNow - timeLast;
        timeLast = timeNow;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        MoveCamera(window, view, unifiedShader, deltaTime);

        // Update physics simulation (cast to float/decimal for ReactPhysics3D)
        physicsWorld->update(static_cast<rp3d::decimal>(deltaTime));
        
        // Update claw position from physics (if it's dynamic)
        claw->UpdateFromPhysics();

        // Draw
        claw->Draw(unifiedShader);
        ground->Draw(unifiedShader); 

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup physics
    physicsCommon.destroyPhysicsWorld(physicsWorld);
    
    delete claw;
    delete ground;
    glfwTerminate();
    return 0;
}

void InitializeGameObjects() {
    // ==================== CLAW MACHINE ====================
    claw = new GameObject("res/claw_machine.obj", physicsWorld, rp3d::BodyType::STATIC);
    claw->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
    claw->Translate(glm::vec3(0.0f, GROUND_HEIGHT, 0.0f));
    
    claw->UpdatePhysicsFromTransform();
    
    claw->AddMeshCollision(physicsCommon);
    
    // ==================== GROUND ====================
    ground = new GameObject("res/ground.obj", physicsWorld, rp3d::BodyType::STATIC);
    ground->Translate(glm::vec3(0.0f, -2.0f, 0.0f));
    
    // Add simple box collision for ground (it's just a flat plane)
    ground->AddBoxCollision(physicsCommon, glm::vec3(10.0f, 0.5f, 10.0f));
}

void MoveCamera(GLFWwindow* window, glm::mat4& viewMatrix, Shader shader, double deltaTime)
{
    const float cameraSpeed = 5.0f; 
    float dt = static_cast<float>(deltaTime);  // Cast once for clarity
    
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, -cameraSpeed * dt));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, cameraSpeed * dt));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(-cameraSpeed * dt, 0.0f, 0.0f));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(cameraSpeed * dt, 0.0f, 0.0f));

    shader.setMat4("uV", viewMatrix);
}