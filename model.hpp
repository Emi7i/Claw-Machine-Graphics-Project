#ifndef MODEL_H
#define MODEL_H
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glew.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.hpp"
#include "shader.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <reactphysics3d/reactphysics3d.h>

using namespace std;

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

class Model
{
public:
    // model data 
    vector<Texture> textures_loaded;
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;

    Model() {}

    // constructor, expects a filepath to a 3D model.
    Model(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, but look out for transparency order
    void Draw(Shader& shader)
    {
        // Draw opaque meshes first
        for (unsigned int i = 0; i < meshes.size(); i++)
        {
            if(meshes[i].opacity >= 1.0f)  // Only opaque
                meshes[i].Draw(shader);
        }
    
        // Then draw transparent meshes (disable depth writing)
        glDepthMask(GL_FALSE);
        for (unsigned int i = 0; i < meshes.size(); i++)
        {
            if(meshes[i].opacity < 1.0f)  // Only transparent
                meshes[i].Draw(shader);
        }
        glDepthMask(GL_TRUE);  // Re-enable depth writing
    }
    
    // NEW - Extract mesh data for physics collision
    void GetMeshDataForPhysics(std::vector<rp3d::Vector3>& outVertices, 
                               std::vector<int>& outIndices, 
                               glm::vec3 scale) {
        outVertices.clear();
        outIndices.clear();
        
        // Iterate through all meshes in the model
        for (const auto& mesh : meshes) {
            size_t baseVertex = outVertices.size();  // Fixed: use size_t instead of int
            
            // Add vertices with scale applied
            for (const auto& vertex : mesh.vertices) {
                outVertices.push_back(rp3d::Vector3(
                    vertex.Position.x * scale.x,
                    vertex.Position.y * scale.y,
                    vertex.Position.z * scale.z
                ));
            }
            
            // Add indices (offset by base vertex for multiple meshes)
            for (unsigned int index : mesh.indices) {
                outIndices.push_back(static_cast<int>(baseVertex + index));  // Fixed: explicit cast
            }
        }
        
        std::cout << "  Extracted " << outVertices.size() << " vertices and " 
                  << outIndices.size() / 3 << " triangles for physics" << std::endl;
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != string::npos) {
            directory = path.substr(0, lastSlash);
        } else {
            directory = ".";
        }

        processNode(scene->mRootNode, scene);   
    }

    // processes a node in a recursive fashion
    void processNode(aiNode* node, const aiScene* scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;
        glm::vec3 diffuseColor(0.8f);

        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            
            // texture coordinates
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        
        // process faces
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        // diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "uDiffMap");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        // specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "uSpecMap");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        
        // Get the diffuse color
        aiColor3D color(0.8f, 0.8f, 0.8f);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        diffuseColor = glm::vec3(color.r, color.g, color.b);
        
        float opacity = 1.0f;
        material->Get(AI_MATKEY_OPACITY, opacity);

        return Mesh(vertices, indices, textures, diffuseColor, opacity);
    }

    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            
            if (!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

#endif