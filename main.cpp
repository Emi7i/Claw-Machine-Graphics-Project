#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gameobject.hpp"

const unsigned int wWidth = 800;
const unsigned int wHeight = 600;

GameObject* fox;
GameObject* bread;

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

    GLFWwindow* window = glfwCreateWindow(wWidth, wHeight, "LearnOpenGL", NULL, NULL);
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

        // Animate
        fox->Rotate(30.0f * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));

        // Draw
        fox->Draw(unifiedShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete fox;
    delete bread;

    glfwTerminate();
    return 0;
}

void InitializeGameObjects() {
    fox = new GameObject("res/low-poly-fox.obj");
    fox->Scale(glm::vec3(0.6f, 0.6f, 0.6f));

    bread = new GameObject("res/bread.obj");
    bread->Scale(glm::vec3(0.6f, 0.6f, 0.6f));
    bread->Rotate(-90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    bread->Translate(glm::vec3(2.0f, 0.5f, 0.0f));
    fox->AddChild(bread);
}


void MoveCamera(GLFWwindow* window, glm::mat4& viewMatrix, Shader shader, double deltaTime)
{
    const float cameraSpeed = 5.0f; 
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, -cameraSpeed * deltaTime));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, cameraSpeed * deltaTime));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(-cameraSpeed * deltaTime, 0.0f, 0.0f));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(cameraSpeed * deltaTime, 0.0f, 0.0f));

    shader.setMat4("uV", viewMatrix);
}