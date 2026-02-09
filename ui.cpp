#include "ui.hpp"
#include <iostream>

// Forward declaration for texture loading function
unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

Logo::Logo() : VAO(0), VBO(0), texture(0), uiShader(nullptr), loaded(false),
               posLeft(0.7f), posBottom(0.7f), posRight(0.95f), posTop(0.9f) {
}

Logo::~Logo() {
    Cleanup();
}

bool Logo::Initialize(const char* texturePath, const char* textureDirectory) {
    // Default position: top-right corner
    return Initialize(texturePath, textureDirectory, 0.7f, 0.7f, 0.95f, 0.9f);
}

bool Logo::Initialize(const char* texturePath, const char* textureDirectory, 
                     float left, float bottom, float right, float top) {
    // Store position
    posLeft = left;
    posBottom = bottom;
    posRight = right;
    posTop = top;
    
    // Generate and configure VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    // Create initial vertices
    UpdateVertices();
    
    // Load texture
    texture = TextureFromFile(texturePath, textureDirectory);
    loaded = (texture != 0);
    
    if (loaded) {
        std::cout << "Logo texture loaded successfully with ID: " << texture << std::endl;
    } else {
        std::cout << "Warning: Failed to load logo texture from " << texturePath << std::endl;
        return false;
    }

    // Create UI shader
    uiShader = new Shader("ui.vert", "ui.frag");
    
    return true;
}

void Logo::SetPosition(float left, float bottom, float right, float top) {
    posLeft = left;
    posBottom = bottom;
    posRight = right;
    posTop = top;
    UpdateVertices();
}

void Logo::UpdateVertices() {
    // Define quad with current position (NDC coordinates: x, y, u, v)
    float logoVertices[] = {
        //  Positions          // TexCoords (flipped on Y axis)
        posLeft,  posTop,      0.0f, 0.0f, // Top Left
        posRight, posTop,      1.0f, 0.0f, // Top Right
        posLeft,  posBottom,   0.0f, 1.0f, // Bottom Left
        posRight, posBottom,   1.0f, 1.0f  // Bottom Right
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(logoVertices), &logoVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

void Logo::Render() {
    if (!loaded || !uiShader) {
        return;
    }

    // Disable depth test to ensure logo is on top
    glDisable(GL_DEPTH_TEST);
    
    // Use UI shader and set texture
    uiShader->use();
    uiShader->setInt("screenTexture", 1);
    
    // Bind VAO and texture
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Draw the quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    // Reset state
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Logo::Cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (uiShader != nullptr) {
        delete uiShader;
        uiShader = nullptr;
    }
    loaded = false;
}

// Simple texture loading function that uses the existing model.hpp implementation
extern unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma);