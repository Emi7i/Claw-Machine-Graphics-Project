#pragma once
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "opengl32.lib") // Added so Rider will stop complaining

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <set>
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
GameObject* colliders[9];
GameObject* trigger;
GameObject* lightCube;

std::vector<GameObject*> birbs; 
std::set<GameObject*> collectedBirbs;

// Physics objects
rp3d::PhysicsCommon physicsCommon;
rp3d::PhysicsWorld* physicsWorld = nullptr;

// Camera
Camera* camera = nullptr;

// UI
Logo* logo = nullptr;
Logo* birbIcon = nullptr;

// Game state
bool GameStarted = false;

// Depth buffer and backface culling state
bool depthTestEnabled = true;
bool backfaceCullingEnabled = false;

// Claw movement speed
float clawSpeed = 5.0f;

// Claw movement state
bool shouldMoveDown = false;
bool shouldMoveUp = false;
bool hasToy = false;
glm::vec3 originalPosition;
bool canMoveByKeys = true;
GameObject* pickedUpBirb = nullptr; // Track which birb is picked up
int birbsCollected = 0; // Track how many birbs collected

void InitializeGameObjects();
void MoveClaw(GLFWwindow* window, double deltaTime);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
GameObject* CheckTriggerCollision(); // Returns the birb that collided
GameObject* CanDirectPickupBirb(); // Returns the birb that can be picked up
void UpdateBirbPhysics();

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
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Claw Machine", monitor, NULL);
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
    unifiedShader.setVec3("uAmbientColor", 1, 1, 1);

    glm::mat4 projection = glm::perspective(glm::radians(camera->zoom), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
    unifiedShader.setMat4("uP", projection);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- LOGO SETUP ---
    logo = new Logo();
    logo->Initialize("Logo.png", "res");

    // --- BIRB ICON SETUP (Bottom Left Corner) ---
    birbIcon = new Logo();
    birbIcon->Initialize("birb.png", "res", -0.95f, -0.9f, -0.7f, -0.65f);

    // Disable vsync to allow custom frame rate
    glfwSwapInterval(0);

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

        // Toggle depth buffer with 'G' key
        static bool gPressed = false;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !gPressed)
        {
            gPressed = true;
            depthTestEnabled = !depthTestEnabled;
        }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
        {
            gPressed = false;
        }
        
        if (depthTestEnabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        // Toggle backface culling with 'F' key
        static bool fPressed = false;
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fPressed)
        {
            fPressed = true;
            backfaceCullingEnabled = !backfaceCullingEnabled;
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
        {
            fPressed = false;
        }
        
        if (backfaceCullingEnabled)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CW);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }

        // Check for E key to start game when looking at claw machine
        static bool ePressed = false;
        if (!GameStarted && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !ePressed)
        {
            ePressed = true;
            
            // Check for direct birb pickup first
            GameObject* directPickupBirb = CanDirectPickupBirb();
            if (directPickupBirb) {
                // Pick up birb directly
                pickedUpBirb = directPickupBirb;
                pickedUpBirb->rigidBody->setType(rp3d::BodyType::KINEMATIC);
                collectedBirbs.insert(directPickupBirb); // ADD THIS LINE
                birbsCollected++;
    
                std::cout << "Birb picked up directly! Total collected: " << birbsCollected << std::endl;
            }
            else if (camera->IsLookingAt(claw_machine->position)) {
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
        const float orbitSpeed = 45.0f;
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

        // Update light position to light cube location
        unifiedShader.setVec3("uLightPos", 0.0f, 5.0f, 0.0f);
        
        // Set light type to point (0) for top light
        unifiedShader.setInt("uLightType", 0);

        // Update light color based on game state
        if (GameStarted) {
            unifiedShader.setVec3("uLightColor", 0.0f, 1.0f, 0.0f);
        } else {
            unifiedShader.setVec3("uLightColor", 1.0f, 0.2f, 0.6f); // Pink
        }

        // Update view matrix
        glm::mat4 view = camera->GetViewMatrix();
        unifiedShader.setMat4("uV", view);

        // Update projection with current zoom
        projection = glm::perspective(glm::radians(camera->zoom), (float)mode->width / (float)mode->height, 0.1f, 100.0f);
        unifiedShader.setMat4("uP", projection);

        MoveClaw(window, deltaTime);

        // Handle Space key for claw movement 
        static bool spacePressed = false; 
        if (GameStarted && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed && !shouldMoveDown && !shouldMoveUp)
        {
            spacePressed = true;
    
            if (pickedUpBirb) {
                // Drop the birb
                claw->RemoveChild(pickedUpBirb);
                
                // Calculate birb's current world position from parent transform
                glm::mat4 birbWorldTransform = claw->GetTransform() * pickedUpBirb->GetTransform();
                glm::vec3 dropPosition = glm::vec3(birbWorldTransform[3]);
                dropPosition.y -= 0.2f;
                
                // Reset birb's transform components to world position
                pickedUpBirb->position = dropPosition;
                pickedUpBirb->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                pickedUpBirb->scale = glm::vec3(0.4f, 0.4f, 0.4f);
                
                // Rebuild transform matrix
                pickedUpBirb->transform = glm::mat4(1.0f);
                pickedUpBirb->transform = glm::translate(pickedUpBirb->transform, pickedUpBirb->position);
                pickedUpBirb->transform = glm::scale(pickedUpBirb->transform, pickedUpBirb->scale);
                
                // Re-enable dynamic physics so it falls
                pickedUpBirb->rigidBody->setType(rp3d::BodyType::DYNAMIC);
                pickedUpBirb->SyncPhysicsFromTransform();
                
                pickedUpBirb = nullptr;
        
                // End game
                GameStarted = false;
                std::cout << "Birb dropped! Game ended." << std::endl;
            } else {
                // Normal claw descent
                shouldMoveDown = true;
                canMoveByKeys = false;
                originalPosition = claw->position;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
        {
            spacePressed = false;
        }
        
        // Handle claw vertical movement
        if (shouldMoveDown)
        {
            const float descentSpeed = 2.0f;
            glm::vec3 movement(0.0f, -descentSpeed * static_cast<float>(deltaTime), 0.0f);
            glm::vec3 oldPosition = claw->position;
            glm::mat4 oldTransform = claw->GetTransform();
                        
            claw->Translate(movement);
            
            // Update birb physics if it's being carried
            if (pickedUpBirb) {
                UpdateBirbPhysics();
            }
            
            // Check for collision with claw machine or ground
            if (physicsWorld->testOverlap(claw->rigidBody, claw_machine->rigidBody) ||
                physicsWorld->testOverlap(claw->rigidBody, ground->rigidBody))
            {
                // Collision detected, restore position and start moving up
                claw->position = oldPosition;
                claw->transform = oldTransform;
                claw->SyncPhysicsFromTransform();
                
                shouldMoveDown = false;
                shouldMoveUp = true;
            }
        }
    
        if (shouldMoveUp)
        {
            const float ascentSpeed = 3.0f;
            glm::vec3 currentPos = claw->position;
            glm::vec3 direction = glm::normalize(originalPosition - currentPos);
            float distance = glm::distance(currentPos, originalPosition);
            
            if (distance > 0.1f)
            {
                glm::vec3 movement = direction * ascentSpeed * static_cast<float>(deltaTime);
                if (glm::length(movement) > distance)
                {
                    movement = direction * distance;
                }
                claw->Translate(movement);
                
                // Update birb physics if it's being carried
                if (pickedUpBirb) {
                    UpdateBirbPhysics();
                }
            }
            else
            {
                // Reached original position
                claw->position = originalPosition;
                claw->transform = glm::translate(glm::mat4(1.0f), originalPosition) * glm::scale(glm::mat4(1.0f), glm::vec3(0.4f, 0.4f, 0.4f));
                claw->SyncPhysicsFromTransform();
                
                // Update birb one last time at final position
                if (pickedUpBirb) {
                    UpdateBirbPhysics();
                }
                
                shouldMoveUp = false;
                canMoveByKeys = true;
                
                // End game if birb was not picked up after one cycle
                if (!pickedUpBirb) {
                    GameStarted = false;
                    std::cout << "Game ended - birb not picked up" << std::endl;
                }
            }
        }
        
        // Toggle crouch with 'C' key
        static bool cPressed = false;
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cPressed)
        {
            cPressed = true;
            camera->ToggleCrouch();
            std::cout << (camera->isCrouching ? "Crouching" : "Standing") << std::endl;
        }
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE)
        {
            cPressed = false;
        }
        
        // Check for trigger collision and pick up birb
        GameObject* collidedBirb = CheckTriggerCollision();
        if (collidedBirb && !pickedUpBirb) {
            // Reset birb scale to original
            collidedBirb->Scale(glm::vec3(1.0f, 1.0f, 1.0f));
    
            claw->AddChild(collidedBirb);
     
            // Make birb kinematic so physics doesn't interfere while carried
            collidedBirb->rigidBody->setType(rp3d::BodyType::KINEMATIC);
     
            pickedUpBirb = collidedBirb;
            std::cout << "Birb picked up!" << std::endl;
        }
        
        // Update birb physics to follow claw while picked up (for horizontal movement)
        if (pickedUpBirb && canMoveByKeys) {
            UpdateBirbPhysics();
        }
        
        // Update physics simulation
        physicsWorld->update(static_cast<rp3d::decimal>(deltaTime));
        
        // Update physics for all dynamic birbs (only when not picked up)
        for (GameObject* birb : birbs) {
            if (birb != pickedUpBirb && collectedBirbs.find(birb) == collectedBirbs.end()) {
                birb->SyncTransformFromPhysics();
            }
        }
        
        // Draw
        // Disable backface culling for claw_machine to prevent disappearing
        if (backfaceCullingEnabled) {
            glDisable(GL_CULL_FACE);
        }
        claw_machine->Draw(unifiedShader);
        if (backfaceCullingEnabled) {
            glEnable(GL_CULL_FACE);
        }
        claw->Draw(unifiedShader);
        ground->Draw(unifiedShader); 
        
        // Draw all birbs that aren't picked up or collected
        for (GameObject* birb : birbs) {
            // Skip if birb has been collected
            if (collectedBirbs.find(birb) != collectedBirbs.end()) {
                continue;
            }
    
            // Check if this birb is being carried by the claw
            bool isBeingCarried = (birb == pickedUpBirb && claw->IsChild(birb));
    
            // Only draw if not being carried
            if (!isBeingCarried) {
                birb->Draw(unifiedShader);
            }
        }

        // Draw UI Overlay
        if (logo && logo->IsLoaded()) {
            logo->Render();
        }
        
        // Switch back to unified shader for 3D rendering
        unifiedShader.use();
        
        
        // Draw light cube with color based on game state
        if (GameStarted) {
            unifiedShader.setVec3("uDiffuseColor", 0.0f, 1.0f, 0.0f);
        } else {
            unifiedShader.setVec3("uDiffuseColor", 1.0f, 0.2f, 0.6f); // Pink
        }
        
        // Reset diffuse color to white for other objects
        unifiedShader.setVec3("uDiffuseColor", 1.0f, 1.0f, 1.0f);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // High-precision frame limiter
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        
        double elapsed = static_cast<double>(currentTime.QuadPart - lastFrameTime.QuadPart) / frequency.QuadPart;
        
        if (elapsed < targetFrameTime)
        {
            LARGE_INTEGER targetTime;
            targetTime.QuadPart = lastFrameTime.QuadPart + static_cast<long long>(targetFrameTime * frequency.QuadPart);
            
            while (currentTime.QuadPart < targetTime.QuadPart)
            {
                QueryPerformanceCounter(&currentTime);
                if ((currentTime.QuadPart % 1000) == 0)
                {
                    std::this_thread::yield();
                }
            }
        }
        
        lastFrameTime = currentTime;
    }

    // Cleanup
    physicsCommon.destroyPhysicsWorld(physicsWorld);
    delete camera;
    delete logo;
    delete birbIcon;
    delete claw;
    delete claw_machine;
    delete ground;
    for (GameObject* birb : birbs) {
        delete birb;
    }
    birbs.clear();
    delete lightCube;
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
    claw->AddBoxCollision(physicsCommon, glm::vec3(0.1f, 0.3f, 0.1f));
    
    // ==================== TRIGGER ====================
    trigger = new GameObject("res/trigger.obj", physicsWorld, rp3d::BodyType::KINEMATIC);
    trigger->AddSphereCollision(physicsCommon, 0.2f);
    
    //claw->AddChild(trigger);
    
    // ==================== BIRBS (Multiple) ====================
    // Birb 1
    GameObject* birb1 = new GameObject("res/birb.obj", physicsWorld, rp3d::BodyType::DYNAMIC);
    birb1->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
    birb1->Translate(glm::vec3(0.0f, -1.0f, 0.0f));
    birb1->AddBoxCollision(physicsCommon, glm::vec3(0.12f, 0.12f, 0.12f));
    birbs.push_back(birb1);
    
    // Birb 2
    GameObject* birb2 = new GameObject("res/birb.obj", physicsWorld, rp3d::BodyType::DYNAMIC);
    birb2->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
    birb2->Translate(glm::vec3(0.5f, -1.0f, 0.5f)); // Different position
    birb2->AddBoxCollision(physicsCommon, glm::vec3(0.12f, 0.12f, 0.12f));
    birbs.push_back(birb2);
    
    // ==================== LIGHT CUBE ====================
    lightCube = new GameObject("res/trigger.obj", physicsWorld, rp3d::BodyType::STATIC);
    lightCube->Scale(glm::vec3(0.4f, 0.4f, 0.4f));
    lightCube->Translate(glm::vec3(2.0f, 1.0f, 0.0f));
}

void MoveClaw(GLFWwindow* window, double deltaTime)
{
    if (!GameStarted || !canMoveByKeys) return;
    
    const float clawSpeed = 3.0f;
    float dt = static_cast<float>(deltaTime);
    
    glm::vec3 oldPosition = claw->position;
    glm::mat4 oldTransform = claw->GetTransform();
                    
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
    
    claw->Translate(movement);
    
    if (physicsWorld->testOverlap(claw->rigidBody, claw_machine->rigidBody))
    {
        claw->position = oldPosition;
        claw->transform = oldTransform;
        claw->SyncPhysicsFromTransform();
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (camera)
    {
        camera->ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (camera)
    {
        camera->ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

GameObject* CheckTriggerCollision()
{
    if (!trigger || !claw || !trigger->rigidBody) return nullptr;
    
    // Get world position of trigger
    glm::mat4 triggerWorldTransform = claw->GetTransform() * trigger->GetTransform();
    glm::vec3 triggerWorldPos = glm::vec3(triggerWorldTransform[3]);
    
    // Check collision with all birbs
    for (GameObject* birb : birbs) {
        if (!birb || !birb->rigidBody) continue;
        
        // Get world position of birb
        rp3d::Vector3 birbPos = birb->rigidBody->getTransform().getPosition();
        glm::vec3 birbWorldPos = glm::vec3(birbPos.x, birbPos.y, birbPos.z);
        
        // Calculate distance between centers
        float distance = glm::distance(triggerWorldPos, birbWorldPos);
        
        // Check if collision occurs
        if (distance < 0.2f + 0.12f) {
            return birb; // Return the first birb that collides
        }
    }
    
    return nullptr;
}

GameObject* CanDirectPickupBirb()
{
    if (!camera) return nullptr;
    
    glm::vec3 cameraPos = camera->position;
    
    // Check all birbs for proximity
    for (GameObject* birb : birbs) {
        if (!birb || !birb->rigidBody) continue;
        
        // Skip if this birb is already picked up
        if (birb == pickedUpBirb) continue;
        
        glm::vec3 birbWorldPos = glm::vec3(
            birb->rigidBody->getTransform().getPosition().x,
            birb->rigidBody->getTransform().getPosition().y,
            birb->rigidBody->getTransform().getPosition().z
        );
        
        float distance = glm::distance(cameraPos, birbWorldPos);
        if (distance < 2.0f) {
            return birb; // Return the first birb within range
        }
    }
    
    return nullptr;
}

void UpdateBirbPhysics()
{
    if (!pickedUpBirb || !claw) return;
    
    // Calculate birb's world transform from parent-child hierarchy
    glm::mat4 birbWorldTransform = claw->GetTransform() * pickedUpBirb->GetTransform();
    
    // Extract position and rotation from world transform
    glm::vec3 worldPosition = glm::vec3(birbWorldTransform[3]);
    glm::quat worldRotation = glm::quat_cast(birbWorldTransform);
    
    // Update birb's physics body to match visual position
    rp3d::Vector3 rp3dPos(worldPosition.x, worldPosition.y, worldPosition.z);
    rp3d::Quaternion rp3dRot(worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w);
    rp3d::Transform physicsTransform(rp3dPos, rp3dRot);
    
    pickedUpBirb->rigidBody->setTransform(physicsTransform);
}