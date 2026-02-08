#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include "shader.hpp"

class Logo {
public:
    Logo();
    ~Logo();
    
    bool Initialize(const char* texturePath, const char* textureDirectory);
    void Render();
    void Cleanup();
    
    bool IsLoaded() const { return loaded; }

private:
    unsigned int VAO, VBO;
    unsigned int texture;
    Shader* uiShader;
    bool loaded;
};

#endif
