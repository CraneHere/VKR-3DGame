#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <PxPhysicsAPI.h>

using namespace physx;

class PhysXErrorCallback : public PxErrorCallback
{
public:
    void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        std::cerr << "[PhysX Error] " << message << " (" << file << ":" << line << ")" << std::endl;
    }
};

class PhysicsSystem {
public:
    PxFoundation* foundation = nullptr;
    PxPhysics* physics = nullptr;
    PxScene* scene = nullptr;
    PxDefaultCpuDispatcher* dispatcher = nullptr;
    PxMaterial* defaultMaterial = nullptr;
    PxControllerManager* controllerManager = nullptr;
    
    PhysXErrorCallback errorCallback;
    PxDefaultAllocator allocator;
    
    bool initialize() {
        foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
        if (!foundation) {
            std::cerr << "PxCreateFoundation failed!" << std::endl;
            return false;
        }
        
        physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale());
        if (!physics) {
            std::cerr << "PxCreatePhysics failed!" << std::endl;
            return false;
        }
        
        PxSceneDesc sceneDesc(physics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        
        dispatcher = PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = dispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        
        scene = physics->createScene(sceneDesc);
        if (!scene) {
            std::cerr << "createScene failed!" << std::endl;
            return false;
        }
        
        defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.3f);
        controllerManager = PxCreateControllerManager(*scene);
        
        return true;
    }
    
    void simulate(float timestep) {
        scene->simulate(timestep);
        scene->fetchResults(true);
    }
    
    void shutdown() {
        if (controllerManager) controllerManager->release();
        if (scene) scene->release();
        if (dispatcher) dispatcher->release();
        if (physics) physics->release();
        if (foundation) foundation->release();
    }
    
    // Создать пол
    PxRigidStatic* createFloor() {
        PxRigidStatic* floorActor = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *defaultMaterial);
        floorActor->userData = nullptr;
        scene->addActor(*floorActor);
        return floorActor;
    }
    
    // Создать статический бокс
    PxRigidStatic* createStaticBox(const glm::vec3& pos, const glm::vec3& halfExtents) {
        PxShape* shape = physics->createShape(PxBoxGeometry(halfExtents.x, halfExtents.y, halfExtents.z), *defaultMaterial);
        PxRigidStatic* actor = physics->createRigidStatic(PxTransform(pos.x, pos.y, pos.z));
        actor->attachShape(*shape);
        scene->addActor(*actor);
        shape->release();
        return actor;
    }
    
    // Создать динамический бокс
    PxRigidDynamic* createDynamicBox(const glm::vec3& pos, const glm::vec3& halfExtents, float density = 10.0f) {
        PxShape* shape = physics->createShape(PxBoxGeometry(halfExtents.x, halfExtents.y, halfExtents.z), *defaultMaterial);
        PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(pos.x, pos.y, pos.z));
        actor->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*actor, density);
        scene->addActor(*actor);
        shape->release();
        return actor;
    }
    
    // Запечь triangle mesh из вершин модели (вызывается один раз на модель)
    PxTriangleMesh* cookTriangleMesh(const std::vector<glm::vec3>& vertices,
                                     const std::vector<unsigned int>& indices)
    {
        PxTriangleMeshDesc meshDesc;
        meshDesc.points.count  = static_cast<PxU32>(vertices.size());
        meshDesc.points.stride = sizeof(glm::vec3);
        meshDesc.points.data   = vertices.data();

        meshDesc.triangles.count  = static_cast<PxU32>(indices.size() / 3);
        meshDesc.triangles.stride = 3 * sizeof(PxU32);
        meshDesc.triangles.data   = indices.data();

        PxDefaultMemoryOutputStream writeBuffer;
        PxCookingParams params(physics->getTolerancesScale());
        params.midphaseDesc = PxMeshMidPhase::eBVH34;

        if (!PxCookTriangleMesh(params, meshDesc, writeBuffer)) {
            std::cerr << "PxCookTriangleMesh failed!" << std::endl;
            return nullptr;
        }

        PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
        PxTriangleMesh* triMesh = physics->createTriangleMesh(readBuffer);
        return triMesh;
    }

    // Создать статический актор из запеченного triangle mesh
    PxRigidStatic* createStaticTriangleMesh(PxTriangleMesh* triMesh,
                                            const glm::vec3& pos,
                                            float scale,
                                            float rotationY = 0.0f)
    {
        PxMeshScale meshScale(PxVec3(scale, scale, scale));
        PxTriangleMeshGeometry geom(triMesh, meshScale);

        PxQuat rotation(glm::radians(rotationY), PxVec3(0, 1, 0));
        PxTransform transform(PxVec3(pos.x, pos.y, pos.z), rotation);

        PxRigidStatic* actor = physics->createRigidStatic(transform);
        PxShape* shape = physics->createShape(geom, *defaultMaterial);
        actor->attachShape(*shape);
        shape->release();
        scene->addActor(*actor);
        return actor;
    }

    // Создать контроллер персонажа
    PxController* createCharacterController(const glm::vec3& pos, float height, float radius) {
        PxCapsuleControllerDesc controllerDesc;
        controllerDesc.height = height - radius * 2;
        controllerDesc.radius = radius;
        controllerDesc.position = PxExtendedVec3(pos.x, pos.y, pos.z);
        controllerDesc.material = defaultMaterial;
        controllerDesc.stepOffset = 0.3f;
        controllerDesc.slopeLimit = cosf(glm::radians(45.0f));
        controllerDesc.contactOffset = 0.05f;
        
        return controllerManager->createController(controllerDesc);
    }
};
