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
#include <algorithm>

class GameObject {
public: 
    Model* model;
    glm::mat4 transform;
    std::vector<GameObject*> children;
    
private:
    // Keep collision data alive - ReactPhysics3D stores pointers, not copies!
    std::vector<rp3d::Vector3> collisionVertices;
    std::vector<int> collisionIndices;
    std::vector<float> collisionVertexArray;

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
    
    void SetTransform(const glm::mat4& newTransform) {
        transform = newTransform;
        position = glm::vec3(transform[3]);
        rotation = glm::quat_cast(transform);
        SyncPhysicsFromTransform();
    }
    
    void AddChild(GameObject* child) {
        children.push_back(child);
    }
    
    void RemoveChild(GameObject* child) {
        auto it = std::remove(children.begin(), children.end(), child);
        children.erase(it, children.end());
    }

    void Rotate(float angle, glm::vec3 axis) {
        transform = glm::rotate(transform, glm::radians(angle), axis);
        // Extract new rotation from the updated matrix
        rotation = glm::quat_cast(transform); 
        SyncPhysicsFromTransform();
    }

    void Translate(glm::vec3 positionOffset) {
        transform = glm::translate(transform, positionOffset);
        // Update local position variable from matrix translation column
        this->position = glm::vec3(transform[3]); 
        SyncPhysicsFromTransform();
    }

    void Scale(glm::vec3 newScale) {
        this->scale = newScale;
        // Rebuild the transform matrix from position, rotation, and scale
        transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = transform * glm::mat4_cast(rotation);
        transform = glm::scale(transform, newScale);
        SyncPhysicsFromTransform();
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
    
    void SyncPhysicsFromTransform() {
        if (!rigidBody) return;

        // Update Position and Rotation (RigidBody level)
        rp3d::Vector3 rp3dPos(position.x, position.y, position.z);
        rp3d::Quaternion rp3dRot(rotation.x, rotation.y, rotation.z, rotation.w);
        rp3d::Transform physicsTransform(rp3dPos, rp3dRot);
    
        rigidBody->setTransform(physicsTransform);

        // Update Scale (Collider level)
        // ReactPhysics3D requires updating the scale on each collider attached to the body
        for (uint32_t i = 0; i < rigidBody->getNbColliders(); i++) {
            rp3d::Collider* collider = rigidBody->getCollider(i);
        }
    }
    
    void SyncTransformFromPhysics() {
        if (!rigidBody) return;
    
        const rp3d::Transform& pTransform = rigidBody->getTransform();
    
        // Update local Pos/Rot variables
        const rp3d::Vector3& pos = pTransform.getPosition();
        position = glm::vec3(pos.x, pos.y, pos.z);
    
        const rp3d::Quaternion& rot = pTransform.getOrientation();
        rotation = glm::quat(rot.w, rot.x, rot.y, rot.z);
    
        // Rebuild the Model Matrix
        transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = transform * glm::mat4_cast(rotation);
        transform = glm::scale(transform, scale);
    }
    
    void CreatePhysicsBody(rp3d::PhysicsWorld* world, rp3d::BodyType bodyType) {
        rp3d::Vector3 pos(position.x, position.y, position.z);
        rp3d::Quaternion rot(rotation.x, rotation.y, rotation.z, rotation.w);
        rp3d::Transform physicsTransform(pos, rot);
        rigidBody = world->createRigidBody(physicsTransform);
        rigidBody->setType(bodyType);
    }
    
    void AddConcaveCollision(rp3d::PhysicsCommon& physicsCommon) {
        if (!rigidBody || !model) return;
    
        // Store in member variables so the data stays alive!
        collisionVertices.clear();
        collisionIndices.clear();
    
        model->GetMeshDataForPhysics(collisionVertices, collisionIndices, scale);
    
        // Create triangle vertex array for concave mesh
        rp3d::TriangleVertexArray triangleArray(
            collisionVertices.size(),
            &collisionVertices[0],
            sizeof(rp3d::Vector3),
            collisionIndices.size() / 3,
            collisionIndices.data(),
            3 * sizeof(int),
            rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
            rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE
        );
        
    
        // Create the triangle mesh directly from the array
        std::vector<rp3d::Message> messages;
        rp3d::TriangleMesh* triangleMesh = physicsCommon.createTriangleMesh(triangleArray, messages);
        
        std::cout << "=== Triangle Mesh Messages ===" << std::endl;
        for (const auto& msg : messages) {
            std::cout << "Message: " << msg.text << std::endl;
        }
    
        rp3d::ConcaveMeshShape* concaveShape = physicsCommon.createConcaveMeshShape(triangleMesh);
    
        rigidBody->addCollider(concaveShape, rp3d::Transform::identity());
    
        std::cout << "Added Concave collision with " << collisionIndices.size() / 3 << " triangles." << std::endl;
    }
    
    // Add perfect mesh collision to this GameObject
    void AddConvexCollision(rp3d::PhysicsCommon& physicsCommon) {
        if (!rigidBody) {
            std::cout << "Error: RigidBody must be created before adding mesh collision!" << std::endl;
            return;
        }
        
        if (!model) {
            std::cout << "Error: Model not loaded!" << std::endl;
            return;
        }
        
        // Store in member variables so the data stays alive!
        collisionVertices.clear();
        collisionIndices.clear();
        collisionVertexArray.clear();
        
        // Get mesh data from model
        model->GetMeshDataForPhysics(collisionVertices, collisionIndices, scale);
        
        // Convert to float arrays for ReactPhysics3D
        for (const auto& v : collisionVertices) {
            collisionVertexArray.push_back(v.x);
            collisionVertexArray.push_back(v.y);
            collisionVertexArray.push_back(v.z);
        }
        
        // Create TriangleVertexArray
        rp3d::TriangleVertexArray triangleArray(
            static_cast<rp3d::uint32>(collisionVertices.size()),
            collisionVertexArray.data(),
            3 * sizeof(float),
            static_cast<rp3d::uint32>(collisionIndices.size() / 3),
            collisionIndices.data(),
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
        
        std::cout << "Added mesh collision with " << collisionIndices.size() / 3 << " triangles" << std::endl;
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
    }
    
    void AddSphereCollision(rp3d::PhysicsCommon& physicsCommon, float radius) {
        if (!rigidBody) {
            std::cout << "Error: RigidBody must be created before adding sphere collision!" << std::endl;
            return;
        }
    
        rp3d::SphereShape* sphereShape = physicsCommon.createSphereShape(radius);
        rigidBody->addCollider(sphereShape, rp3d::Transform::identity());
    }

};

#endif // GAMEOBJECT_HPP