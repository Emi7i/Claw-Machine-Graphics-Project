#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include "shader.hpp"

class Logo {
public:
    Logo();
    ~Logo();
    
    // Initialize with texture path and directory
    bool Initialize(const char* texturePath, const char* textureDirectory);
    
    // Initialize with custom position and size (NDC coordinates: -1 to 1)
    bool Initialize(const char* texturePath, const char* textureDirectory, 
                   float left, float bottom, float right, float top);
    
    void Render();
    void Cleanup();
    
    // Set position after initialization (NDC coordinates)
    void SetPosition(float left, float bottom, float right, float top);
    
    bool IsLoaded() const { return loaded; }

private:
    unsigned int VAO, VBO;
    unsigned int texture;
    Shader* uiShader;
    bool loaded;
    
    // Store current position for updates
    float posLeft, posBottom, posRight, posTop;
    
    void UpdateVertices();
};

#endif