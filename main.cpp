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
#include "Camera.hpp"
#include "shader.hpp"
#include "ui.hpp"

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

// Camera
Camera* camera = nullptr;

// UI
Logo* logo = nullptr;

// Game state
bool GameStarted = false;

void InitializeGameObjects();
void MoveClaw(GLFWwindow* window, double deltaTime);

// Mouse callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (camera)
    {
        camera->ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

// Scroll callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (camera)
    {
        camera->ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

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

    // Get primary monitor and its video mode
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    // Use monitor's native resolution
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Physics Demo with Mesh Collision", monitor, NULL);
    if (window == NULL)
    {
        std::cout << "Window fail!\n" << std::endl;
        glfwTerminate();
        return -2;
    }
    glfwMakeContextCurrent(window);

    // Set up mouse input
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Capture and hide cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    // Initialize camera
    camera = new Camera(glm::vec3(0.0f, 0.0f, 5.0f));
    camera->movementSpeed = 5.0f;
    camera->mouseSensitivity = 0.1f;

    Shader unifiedShader("basic.vert", "basic.frag");
    unifiedShader.use();
    unifiedShader.setVec3("uLightPos", 0, 1, 3);
    unifiedShader.setVec3("uViewPos", camera->position.x, camera->position.y, camera->position.z);
    unifiedShader.setVec3("uLightColor", 1, 1, 1);

    glm::mat4 projection = glm::perspective(glm::radians(camera->zoom), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
    unifiedShader.setMat4("uP", projection);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- LOGO SETUP ---
    logo = new Logo();
    logo->Initialize("Logo.png", "res");
    
    // Disable vsync to allow custom frame rate
    glfwSwapInterval(0);
    
    // Debug
    physicsWorld->setIsDebugRenderingEnabled(true);
    rp3d::DebugRenderer& debugRenderer = physicsWorld->getDebugRenderer();
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);

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
        {
            glfwSetWindowShouldClose(window, true);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Check for E key to start game when looking at claw machine
        static bool ePressed = false;
        if (!GameStarted && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !ePressed)
        {
            ePressed = true;
            if (camera->IsLookingAt(claw_machine->position))
            {
                GameStarted = true;
                std::cout << "Game Started!" << std::endl;
            }
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE)
        {
            ePressed = false;
        }

        // Camera movement (only when not in game)
        if (!GameStarted)
        {
            camera->ProcessKeyboard(window, static_cast<float>(deltaTime));
        }

        // Arrow keys for orbital rotation around claw machine
        const float orbitSpeed = 45.0f; // degrees per second
        float horizontalOrbit = 0.0f;
        float verticalOrbit = 0.0f;
        
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            horizontalOrbit = orbitSpeed * static_cast<float>(deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            horizontalOrbit = -orbitSpeed * static_cast<float>(deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            verticalOrbit = orbitSpeed * static_cast<float>(deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            verticalOrbit = -orbitSpeed * static_cast<float>(deltaTime);
        }
        
        if (horizontalOrbit != 0.0f || verticalOrbit != 0.0f)
        {
            camera->OrbitAroundTarget(claw_machine->position, horizontalOrbit, verticalOrbit);
        }

        // Update view position for lighting
        unifiedShader.setVec3("uViewPos", camera->position.x, camera->position.y, camera->position.z);
        
        // Update view matrix
        glm::mat4 view = camera->GetViewMatrix();
        unifiedShader.setMat4("uV", view);
        
        // Update projection with current zoom
        projection = glm::perspective(glm::radians(camera->zoom), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
        unifiedShader.setMat4("uP", projection);

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

        // Draw UI Overlay
        if (logo && logo->IsLoaded()) {
            logo->Render();
        }
        unifiedShader.use(); // Switch back to unified shader

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // High-precision frame limiter using QueryPerformanceCounter
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        
        // Calculate elapsed time in seconds
        double elapsed = static_cast<double>(currentTime.QuadPart - lastFrameTime.QuadPart) / frequency.QuadPart;
        
        // If we haven't reached target frame time, busy-wait for precision
        if (elapsed < targetFrameTime)
        {
            LARGE_INTEGER targetTime;
            targetTime.QuadPart = lastFrameTime.QuadPart + static_cast<long long>(targetFrameTime * frequency.QuadPart);
            
            // Busy-wait until we reach the target time
            while (currentTime.QuadPart < targetTime.QuadPart)
            {
                QueryPerformanceCounter(&currentTime);
                // Optional: Yield CPU occasionally to prevent 100% usage
                if ((currentTime.QuadPart % 1000) == 0)
                {
                    std::this_thread::yield();
                }
            }
        }
        
        lastFrameTime = currentTime;
    }

    // Cleanup
    delete camera;
    delete logo;
    physicsCommon.destroyPhysicsWorld(physicsWorld);
    delete claw;
    delete claw_machine;
    delete ground;
    delete birb;
    
    glfwTerminate();
    return 0;
}

void InitializeGameObjects()
{
    // ==================== CLAW MACHINE ====================
    claw_machine = new GameObject("res/claw_machine.obj", physicsWorld, rp3d::BodyType::STATIC);
    claw_machine->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
    claw_machine->Translate(glm::vec3(0.0f, GROUND_HEIGHT, 0.0f));
    claw_machine->AddConcaveCollision(physicsCommon);
    
    // ==================== GROUND ====================
    ground = new GameObject("res/ground.obj", physicsWorld, rp3d::BodyType::STATIC);
    ground->Translate(glm::vec3(0.0f, -2.0f, 0.0f));
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

void MoveClaw(GLFWwindow* window, double deltaTime)
{
    if (!GameStarted) return;
    
    const float clawSpeed = 3.0f;
    float dt = static_cast<float>(deltaTime);
    
    // Store current position to restore if collision occurs
    glm::vec3 oldPosition = claw->position;
    glm::mat4 oldTransform = claw->GetTransform();
    
    // Calculate potential new position
    glm::vec3 movement(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        movement.z -= clawSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        movement.z += clawSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        movement.x -= clawSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        movement.x += clawSpeed * dt;
    }
    
    // Apply movement
    claw->Translate(movement);
    
    // Manually test overlap (kinematic vs static doesn't trigger callbacks!)
    if (physicsWorld->testOverlap(claw->rigidBody, claw_machine->rigidBody))
    {
        // Restore EVERYTHING - position, transform, and physics
        claw->position = oldPosition;
        claw->transform = oldTransform;
        claw->SyncPhysicsFromTransform();
    }
}