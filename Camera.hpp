#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera
{
public:
    // Camera attributes
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Euler angles
    float yaw;
    float pitch;

    // Camera options
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

    // Mouse tracking
    bool firstMouse;
    float lastX;
    float lastY;
    
    // Crouch state
    bool isCrouching;
    float standHeight;
    float crouchHeight;

    // Constructor
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f)
        : front(glm::vec3(0.0f, 0.0f, -1.0f)),
        movementSpeed(5.0f),
        mouseSensitivity(0.1f),
        zoom(45.0f),
        firstMouse(true),
        lastX(400.0f),
        lastY(300.0f),
        isCrouching(false),
        standHeight(position.y), 
        crouchHeight(position.y - 1.0f)
    {
        this->position = position;
        this->worldUp = up;
        this->yaw = yaw;
        this->pitch = pitch;
        updateCameraVectors();
    }

    // Returns the view matrix calculated using Euler angles and the LookAt matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(position, position + front, up);
    }

    // Process keyboard input
    void ProcessKeyboard(GLFWwindow* window, float deltaTime)
    {
        float velocity = movementSpeed * deltaTime;

        // Create horizontal front vector (ignore Y component)
        glm::vec3 horizontalFront = front;
        horizontalFront.y = 0.0f;
        horizontalFront = glm::normalize(horizontalFront);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            position += horizontalFront * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            position -= horizontalFront * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position -= right * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            position += right * velocity;
    }
    
    // Toggle crouch
    void ToggleCrouch()
    {
        isCrouching = !isCrouching;
        if (isCrouching)
        {
            position.y = crouchHeight;
        }
        else
        {
            position.y = standHeight;
        }
    }
    
    // Process mouse movement
    void ProcessMouseMovement(float xpos, float ypos, bool constrainPitch = true)
    {
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
        lastX = xpos;
        lastY = ypos;

        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // Constrain pitch to prevent screen flip
        if (constrainPitch)
        {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }

        // Update front, right and up vectors using the updated Euler angles
        updateCameraVectors();
    }

    // Process mouse scroll for zoom
    void ProcessMouseScroll(float yoffset)
    {
        zoom -= yoffset;
        if (zoom < 1.0f) zoom = 1.0f;
        if (zoom > 45.0f) zoom = 45.0f;
    }

    // Check if looking at target (for interaction)
    bool IsLookingAt(glm::vec3 targetPos, float angleThreshold = 0.866f, float maxDistance = 5.0f)
    {
        glm::vec3 toTarget = glm::normalize(targetPos - position);
        float dotProduct = glm::dot(front, toTarget);
        float distance = glm::length(targetPos - position);
        
        return dotProduct > angleThreshold && distance < maxDistance;
    }

    // Orbit around a target point
    void OrbitAroundTarget(glm::vec3 targetPos, float horizontalAngle, float verticalAngle)
    {
        // Get current offset from target
        glm::vec3 offset = position - targetPos;
        float radius = glm::length(offset);
        
        // Horizontal rotation (around Y axis)
        if (horizontalAngle != 0.0f)
        {
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(horizontalAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            offset = glm::vec3(rotation * glm::vec4(offset, 1.0f));
        }
        
        // Vertical rotation (around right axis)
        if (verticalAngle != 0.0f)
        {
            // Calculate right vector for this offset
            glm::vec3 rightVec = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), offset));
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(verticalAngle), rightVec);
            offset = glm::vec3(rotation * glm::vec4(offset, 1.0f));
            
            // Clamp to prevent flipping
            float angle = glm::degrees(acos(glm::dot(glm::normalize(offset), glm::vec3(0.0f, 1.0f, 0.0f))));
            if (angle < 5.0f || angle > 175.0f)
            {
                // Revert vertical rotation if too close to poles
                offset = position - targetPos;
            }
        }
        
        // Update position
        position = targetPos + offset;
        
        // Make camera look at target
        glm::vec3 direction = glm::normalize(targetPos - position);
        front = direction;
        
        // Update yaw and pitch to match
        pitch = glm::degrees(asin(direction.y));
        yaw = glm::degrees(atan2(direction.z, direction.x));
        
        // Update camera vectors
        updateCameraVectors();
    }

private:
    // Calculate the front vector from the camera's Euler angles
    void updateCameraVectors()
    {
        // Calculate the new front vector
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        // Re-calculate the right and up vector
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
};