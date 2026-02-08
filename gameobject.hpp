#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <reactphysics3d/reactphysics3d.h>
#include "model.hpp"
#include "shader.hpp"
#include <vector>
#include <iostream>

class GameObject {
public: 
    Model* model;
    
private:
    glm::mat4 transform;
    std::vector<GameObject*> children;

public:
    // Physics
    rp3d::RigidBody* rigidBody;
    
    // Transform properties for physics sync
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    // Constructor
    GameObject(const char* path, rp3d::PhysicsWorld* world, rp3d::BodyType bodyType = rp3d::BodyType::STATIC) {
        model = new Model(path);
        transform = glm::mat4(1.0f);
        rigidBody = nullptr;
        position = glm::vec3(0.0f);
        rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        scale = glm::vec3(1.0f);
    
        // Auto-create physics if world provided
        if (world != nullptr) {
            CreatePhysicsBody(world, bodyType);
        }
    }

    // Destructor
    ~GameObject() {
        delete model;
        for (auto child : children) {
            delete child;
        }
    }
    
    void AddChild(GameObject* child) {
        children.push_back(child);
    }

    void Rotate(float angle, glm::vec3 axis) {
        transform = glm::rotate(transform, glm::radians(angle), axis);
    }

    void Translate(glm::vec3 position) {
        transform = glm::translate(transform, position);
        this->position += position;
    }

    void Scale(glm::vec3 newScale) {
        this->scale = newScale;
    }

    void Draw(Shader& shader) {
        shader.setMat4("uM", transform);
        model->Draw(shader);

        // Draw children with parent's transform applied
        for (auto child : children) {
            glm::mat4 childTransform = transform * child->GetTransform();
            shader.setMat4("uM", childTransform);
            child->model->Draw(shader);
        }
    }

    glm::mat4 GetTransform() const {
        return transform;
    }
    
    // Sync physics position to GameObject
    void UpdateFromPhysics() {
        if (!rigidBody) return;
        
        const rp3d::Transform& physicsTransform = rigidBody->getTransform();
        
        // Get position
        const rp3d::Vector3& pos = physicsTransform.getPosition();
        position = glm::vec3(pos.x, pos.y, pos.z);
        
        // Get rotation
        const rp3d::Quaternion& rot = physicsTransform.getOrientation();
        rotation = glm::quat(rot.w, rot.x, rot.y, rot.z);
        
        // Update the transform matrix from physics
        transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = transform * glm::mat4_cast(rotation);
        transform = glm::scale(transform, scale);
    }
    
    // Sync GameObject transform to physics (for kinematic objects)
    void UpdatePhysicsFromTransform() {
        if (!rigidBody) return;
        
        // Extract position from transform matrix
        position = glm::vec3(transform[3]);
        
        rp3d::Vector3 rp3dPos(position.x, position.y, position.z);
        rp3d::Quaternion rp3dRot(rotation.x, rotation.y, rotation.z, rotation.w);
        rp3d::Transform physicsTransform(rp3dPos, rp3dRot);
        rigidBody->setTransform(physicsTransform);
    }
    
    void CreatePhysicsBody(rp3d::PhysicsWorld* world, rp3d::BodyType bodyType) {
        rp3d::Vector3 pos(position.x, position.y, position.z);
        rp3d::Quaternion rot(rotation.x, rotation.y, rotation.z, rotation.w);
        rp3d::Transform physicsTransform(pos, rot);
        rigidBody = world->createRigidBody(physicsTransform);
        rigidBody->setType(bodyType);
    }
    
    // Add perfect mesh collision to this GameObject
    void AddMeshCollision(rp3d::PhysicsCommon& physicsCommon) {
        if (!rigidBody) {
            std::cout << "Error: RigidBody must be created before adding mesh collision!" << std::endl;
            return;
        }
        
        if (!model) {
            std::cout << "Error: Model not loaded!" << std::endl;
            return;
        }
        
        // Get mesh data from model
        std::vector<rp3d::Vector3> vertices;
        std::vector<int> indices;
        model->GetMeshDataForPhysics(vertices, indices, scale);
        
        // Convert to float arrays for ReactPhysics3D
        std::vector<float> vertexArray;
        for (const auto& v : vertices) {
            vertexArray.push_back(v.x);
            vertexArray.push_back(v.y);
            vertexArray.push_back(v.z);
        }
        
        // Create TriangleVertexArray
        rp3d::TriangleVertexArray triangleArray(
            static_cast<rp3d::uint32>(vertices.size()),
            vertexArray.data(),
            3 * sizeof(float),
            static_cast<rp3d::uint32>(indices.size() / 3),
            indices.data(),
            3 * sizeof(int),
            rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
            rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE
        );
        
        // Create TriangleMesh
        std::vector<rp3d::Message> messages;
        rp3d::TriangleMesh* triangleMesh = physicsCommon.createTriangleMesh(triangleArray, messages);
        
        // Create ConcaveMeshShape (works with any geometry - hollow, concave, complex)
        rp3d::ConcaveMeshShape* meshShape = physicsCommon.createConcaveMeshShape(triangleMesh);
        
        // Add the collider to the rigid body
        rigidBody->addCollider(meshShape, rp3d::Transform::identity());
        
        std::cout << "Added mesh collision with " << indices.size() / 3 << " triangles" << std::endl;
    }
    
    // Helper to add simple box collision
    void AddBoxCollision(rp3d::PhysicsCommon& physicsCommon, glm::vec3 halfExtents) {
        if (!rigidBody) {
            std::cout << "Error: RigidBody must be created before adding box collision!" << std::endl;
            return;
        }
        
        rp3d::BoxShape* boxShape = physicsCommon.createBoxShape(
            rp3d::Vector3(halfExtents.x, halfExtents.y, halfExtents.z)
        );
        rigidBody->addCollider(boxShape, rp3d::Transform::identity());
        
        std::cout << "Added box collision" << std::endl;
    }
};

#endif // GAMEOBJECT_HPP