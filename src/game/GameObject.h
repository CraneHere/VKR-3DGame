#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <PxPhysicsAPI.h>

using namespace physx;

struct GameObject
{
    PxRigidActor* actor = nullptr;
    glm::vec3 size = glm::vec3(1.0f);
    glm::vec3 color = glm::vec3(1.0f);
    bool isDynamic = false;
    bool isGrappleable = false;
    
    glm::vec3 getPosition() const {
        if (!actor) return glm::vec3(0);
        PxTransform t = actor->getGlobalPose();
        return glm::vec3(t.p.x, t.p.y, t.p.z);
    }
    
    glm::mat4 getTransform() const {
        if (!actor) return glm::mat4(1.0f);
        PxTransform t = actor->getGlobalPose();
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat, glm::vec3(t.p.x, t.p.y, t.p.z));
        PxQuat q = t.q;
        glm::quat rotation(q.w, q.x, q.y, q.z);
        mat *= glm::mat4_cast(rotation);
        mat = glm::scale(mat, size);
        return mat;
    }
};
