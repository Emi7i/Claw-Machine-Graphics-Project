#pragma once
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "opengl32.lib")  // Added so Rider will stop complaining
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <windows.h>
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
GameObject* claw_machine;
GameObject* ground;
GameObject* birb;
GameObject* colliders[9];

// Physics objects
rp3d::PhysicsCommon physicsCommon;
rp3d::PhysicsWorld* physicsWorld = nullptr;

// Game state
bool GameStarted = false;

void InitializeGameObjects();
void MoveCamera(GLFWwindow* window, glm::mat4& viewMatrix, Shader shader, double deltaTime);
void MoveClaw(GLFWwindow* window, double deltaTime);
bool IsLookingAtClawMachine(const glm::mat4& viewMatrix, GameObject* target);

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
    
    // Disable vsync to allow custom frame rate
    glfwSwapInterval(0);
    
    // Debug
    physicsWorld->setIsDebugRenderingEnabled(true);
    rp3d::DebugRenderer& debugRenderer = physicsWorld->getDebugRenderer();
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);

    // Shader debugShader("debug_line.vert", "debug_line.frag");

    double timeLast = glfwGetTime();
    double deltaTime = 0.0;
    
    // Frame limiter settings
    const double targetFPS = 75.0;
    const double targetFrameTime = 1.0 / targetFPS;
    
    // High-precision timing variables
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastFrameTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastFrameTime);

    // ------------------------- MAIN LOOP -------------------------
    while (!glfwWindowShouldClose(window))
    {
        double timeNow = glfwGetTime();
        deltaTime = timeNow - timeLast;
        timeLast = timeNow;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Check for E key to start game when looking at claw machine
        if (!GameStarted && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            if (IsLookingAtClawMachine(view, claw_machine)) {
                GameStarted = true;
                std::cout << "Game Started!" << std::endl;
            }
        }

        MoveCamera(window, view, unifiedShader, deltaTime);
        MoveClaw(window, deltaTime);

        // Update physics simulation (cast to float/decimal for ReactPhysics3D)
        physicsWorld->update(static_cast<rp3d::decimal>(deltaTime));
        
        // Update physics for dynamic objects
        birb->SyncTransformFromPhysics();

        // Draw
        claw_machine->Draw(unifiedShader);
        claw->Draw(unifiedShader);
        ground->Draw(unifiedShader); 
        birb->Draw(unifiedShader);
        
        /*// Draw colliders
        for (int i = 0; i < 9; i++) {
            colliders[i]->Draw(unifiedShader);
        }*/
        

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // High-precision frame limiter using QueryPerformanceCounter
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        
        // Calculate elapsed time in seconds
        double elapsed = static_cast<double>(currentTime.QuadPart - lastFrameTime.QuadPart) / frequency.QuadPart;
        
        // If we haven't reached target frame time, busy-wait for precision
        if (elapsed < targetFrameTime) {
            LARGE_INTEGER targetTime;
            targetTime.QuadPart = lastFrameTime.QuadPart + static_cast<long long>(targetFrameTime * frequency.QuadPart);
            
            // Busy-wait until we reach the target time
            while (currentTime.QuadPart < targetTime.QuadPart) {
                QueryPerformanceCounter(&currentTime);
                // Optional: Yield CPU occasionally to prevent 100% usage
                if ((currentTime.QuadPart % 1000) == 0) {
                    std::this_thread::yield();
                }
            }
        }
        
        lastFrameTime = currentTime;
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
    claw_machine = new GameObject("res/claw_machine.obj", physicsWorld, rp3d::BodyType::STATIC);
    claw_machine->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
    claw_machine->Translate(glm::vec3(0.0f, GROUND_HEIGHT, 0.0f));
    
    claw_machine->AddConcaveCollision(physicsCommon);
    
        
    /*// ==================== COLLIDERS ====================
    for (int i = 0; i < 9; i++) {
        std::string colliderPath = "res/col" + std::to_string(i + 1) + ".obj";
        colliders[i] = new GameObject(colliderPath.c_str(), physicsWorld, rp3d::BodyType::STATIC);
        colliders[i]->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
        colliders[i]->Translate(glm::vec3(0.0f, GROUND_HEIGHT, 0.0f));
        colliders[i]->AddConvexCollision(physicsCommon);
    }*/
    
    // ==================== GROUND ====================
    ground = new GameObject("res/ground.obj", physicsWorld, rp3d::BodyType::STATIC);
    ground->Translate(glm::vec3(0.0f, -2.0f, 0.0f));
    
    // Add simple box collision for ground (it's just a flat plane)
    ground->AddBoxCollision(physicsCommon, glm::vec3(10.0f, 0.5f, 10.0f));
    
    // ==================== CLAW ====================
    claw = new GameObject("res/claw.obj", physicsWorld, rp3d::BodyType::KINEMATIC);
    claw->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
    claw->Translate(glm::vec3(0.0f, 1.0f, 0.0f));
    
    claw->AddBoxCollision(physicsCommon, glm::vec3(0.1f, 0.1f, 0.1f));
    
    // ==================== TOYS ====================
    birb = new GameObject("res/birb.obj", physicsWorld, rp3d::BodyType::DYNAMIC);
    birb->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
    birb->Translate(glm::vec3(0.0f, -1.0f, 0.0f));
    
    birb->AddBoxCollision(physicsCommon, glm::vec3(0.12f, 0.12f, 0.12f));
}

void MoveCamera(GLFWwindow* window, glm::mat4& viewMatrix, Shader shader, double deltaTime)
{
    const float cameraSpeed = 5.0f; 
    const float rotationSpeed = 26.0f;
    float dt = static_cast<float>(deltaTime);  // Cast once for clarity
    
    // WASD - Translation movement
    if (!GameStarted)
    {
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, -cameraSpeed * dt));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, cameraSpeed * dt));
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            viewMatrix = glm::translate(viewMatrix, glm::vec3(-cameraSpeed * dt, 0.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            viewMatrix = glm::translate(viewMatrix, glm::vec3(cameraSpeed * dt, 0.0f, 0.0f));
    }
    
    // Arrow Keys - Rotation around center (0, 0, 0)
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        // Rotate around Y axis (horizontal rotation)
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed * dt), glm::vec3(0.0f, 1.0f, 0.0f));
        viewMatrix = viewMatrix * rotation;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        // Rotate around Y axis (horizontal rotation)
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-rotationSpeed * dt), glm::vec3(0.0f, 1.0f, 0.0f));
        viewMatrix = viewMatrix * rotation;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        // Rotate around X axis (vertical rotation)
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed * dt), glm::vec3(1.0f, 0.0f, 0.0f));
        viewMatrix = viewMatrix * rotation;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        // Rotate around X axis (vertical rotation)
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-rotationSpeed * dt), glm::vec3(1.0f, 0.0f, 0.0f));
        viewMatrix = viewMatrix * rotation;
    }

    shader.setMat4("uV", viewMatrix);
}

bool IsLookingAtClawMachine(const glm::mat4& viewMatrix, GameObject* target) {
    // Extract camera position from view matrix (inverse of view matrix)
    glm::mat4 viewInverse = glm::inverse(viewMatrix);
    glm::vec3 cameraPos = glm::vec3(viewInverse[3]);
    
    // Extract camera forward direction from view matrix
    glm::vec3 cameraForward = glm::normalize(glm::vec3(-viewMatrix[0][2], -viewMatrix[1][2], -viewMatrix[2][2]));
    
    // Get target position
    glm::vec3 targetPos = target->position;
    
    // Calculate direction to target
    glm::vec3 toTarget = glm::normalize(targetPos - cameraPos);
    
    // Calculate dot product to check if we're looking at the target
    float dotProduct = glm::dot(cameraForward, toTarget);
    
    // Check if we're looking within a reasonable angle (cos(30 degrees) ≈ 0.866)
    float threshold = 0.866f;
    
    // Also check distance to make sure we're close enough
    float distance = glm::length(targetPos - cameraPos);
    float maxDistance = 5.0f;
    
    return dotProduct > threshold && distance < maxDistance;
}

void MoveClaw(GLFWwindow* window, double deltaTime)
{
    if (!GameStarted) return;
    
    const float clawSpeed = 3.0f;
    float dt = static_cast<float>(deltaTime);
    
    // Store current position to restore if collision occurs
    glm::vec3 oldPosition = claw->position;
    glm::mat4 oldTransform = claw->GetTransform(); // Store the whole transform!
    
    // Calculate potential new position
    glm::vec3 movement(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        movement.z -= clawSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        movement.z += clawSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        movement.x -= clawSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        movement.x += clawSpeed * dt;
    }
    
    // Apply movement
    claw->Translate(movement);
    
    // Manually test overlap (kinematic vs static doesn't trigger callbacks!)
    if (physicsWorld->testOverlap(claw->rigidBody, claw_machine->rigidBody)) {

        // Restore EVERYTHING - position, transform, and physics
        claw->position = oldPosition;
        claw->transform = oldTransform; // Need to add public access or add a setter
        claw->SyncPhysicsFromTransform();
    }
}
