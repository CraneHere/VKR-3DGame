#pragma once

#include <cmath>
#include <utility>
#include <glm/glm.hpp>
#include <PxPhysicsAPI.h>
#include "../core/Constants.h"
#include "GameObject.h"

using namespace physx;

struct GrappleHook {
    bool active = false;
    glm::vec3 attachPoint = glm::vec3(0);
    glm::vec3 startPoint = glm::vec3(0);       // Для физики (невидимая)
    glm::vec3 visualStartPoint = glm::vec3(0); // Для рендеринга (видимая)
    float length = 0.0f;
    float initialLength = 0.0f;
    bool reeling = false;
    
    void reset() {
        active = false;
        attachPoint = glm::vec3(0);
        startPoint = glm::vec3(0);
        visualStartPoint = glm::vec3(0);
        length = 0.0f;
        initialLength = 0.0f;
        reeling = false;
    }
};

class ODMGear {
public:
    GrappleHook leftGrapple;
    GrappleHook rightGrapple;
    bool isActive = false;
    
    ODMGear() = default;
    
    // Поиск точки для троса
    std::pair<bool, glm::vec3> findGrapplePoint(
        PxScene* scene,
        const glm::vec3& origin,
        const glm::vec3& forward,
        const glm::vec3& right,
        int direction,
        const glm::vec3* otherGrapplePoint
    ) {
        for (float angle = 0.0f; angle <= GRAPPLE_SCAN_ANGLE; angle += GRAPPLE_SCAN_STEP) {
            float radAngle = glm::radians(angle * direction);
            glm::vec3 rayDir = glm::normalize(
                forward * cos(radAngle) + right * sin(radAngle)
            );
            
            PxRaycastBuffer hit;
            PxVec3 pxOrigin(origin.x, origin.y, origin.z);
            PxVec3 pxDir(rayDir.x, rayDir.y, rayDir.z);
            
            if (scene->raycast(pxOrigin, pxDir, GRAPPLE_MAX_DISTANCE, hit)) {
                if (hit.hasBlock) {
                    PxRigidActor* hitActor = hit.block.actor;
                    
                    if (hitActor && hitActor->userData) {
                        GameObject* gameObj = static_cast<GameObject*>(hitActor->userData);
                        
                        if (!gameObj->isGrappleable) {
                            continue;
                        }
                        
                        glm::vec3 hitPoint(hit.block.position.x, hit.block.position.y, hit.block.position.z);
                        
                        if (otherGrapplePoint != nullptr) {
                            float separation = glm::length(hitPoint - *otherGrapplePoint);
                            if (separation < GRAPPLE_MIN_SEPARATION) {
                                continue;
                            }
                        }
                        
                        return {true, hitPoint};
                    }
                }
            }
        }
        return {false, glm::vec3(0)};
    }
    
    // Выпустить левый трос
    bool fireLeftGrapple(PxScene* scene, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& right) {
        glm::vec3 hipOffset = glm::vec3(0.0f, -PLAYER_HEIGHT * 0.3f, 0.0f);
        glm::vec3 leftHip = position + hipOffset - right * 0.3f;
        const glm::vec3* otherPoint = rightGrapple.active ? &rightGrapple.attachPoint : nullptr;
        
        auto [found, hitPoint] = findGrapplePoint(scene, leftHip, forward, right, -1, otherPoint);
        
        if (found) {
            leftGrapple.active = true;
            leftGrapple.attachPoint = hitPoint;
            leftGrapple.startPoint = leftHip;
            leftGrapple.visualStartPoint = position + hipOffset - right * 0.4f; // Визуальная - с левого бока
            leftGrapple.length = glm::length(hitPoint - leftHip);
            leftGrapple.initialLength = leftGrapple.length;
            leftGrapple.reeling = true;
            return true;
        }
        return false;
    }
    
    // Выпустить правый трос
    bool fireRightGrapple(PxScene* scene, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& right) {
        glm::vec3 hipOffset = glm::vec3(0.0f, -PLAYER_HEIGHT * 0.3f, 0.0f);
        glm::vec3 rightHip = position + hipOffset + right * 0.3f;
        const glm::vec3* otherPoint = leftGrapple.active ? &leftGrapple.attachPoint : nullptr;
        
        auto [found, hitPoint] = findGrapplePoint(scene, rightHip, forward, right, +1, otherPoint);
        
        if (found) {
            rightGrapple.active = true;
            rightGrapple.attachPoint = hitPoint;
            rightGrapple.startPoint = rightHip;
            rightGrapple.visualStartPoint = position + hipOffset + right * 0.4f; // Визуальная - с правого бока
            rightGrapple.length = glm::length(hitPoint - rightHip);
            rightGrapple.initialLength = rightGrapple.length;
            rightGrapple.reeling = true;
            return true;
        }
        return false;
    }
    
    void releaseLeftGrapple() {
        leftGrapple.reset();
    }
    
    void releaseRightGrapple() {
        rightGrapple.reset();
    }
    
    // Проверка, пересекает ли трос препятствие
    bool isRopeBlocked(PxScene* scene, const GrappleHook& grapple) {
        if (!grapple.active) return false;
        
        glm::vec3 dir = grapple.attachPoint - grapple.startPoint;
        float dist = glm::length(dir);
        if (dist < 0.5f) return false;
        
        dir = glm::normalize(dir);
        
        // Небольшой отступ от начала и конца, чтобы не попасть в игрока или якорь
        glm::vec3 rayStart = grapple.startPoint + dir * 0.3f;
        float rayDist = dist - 0.6f;  // Отступаем с обоих концов
        
        if (rayDist < 0.1f) return false;
        
        PxRaycastBuffer hit;
        PxVec3 pxStart(rayStart.x, rayStart.y, rayStart.z);
        PxVec3 pxDir(dir.x, dir.y, dir.z);
        
        if (scene->raycast(pxStart, pxDir, rayDist, hit)) {
            if (hit.hasBlock) {
                return true;  // Трос пересекает препятствие
            }
        }
        return false;
    }
    
    // Проверить и оборвать тросы при пересечении с геометрией
    void checkRopeCollisions(PxScene* scene) {
        if (isRopeBlocked(scene, leftGrapple)) {
            releaseLeftGrapple();
        }
        if (isRopeBlocked(scene, rightGrapple)) {
            releaseRightGrapple();
        }
    }
    
    void update(float deltaTime, bool isGrounded) {
        (void)deltaTime;
        (void)isGrounded;
        isActive = leftGrapple.active || rightGrapple.active;
    }
    
    // Обновить позиции тросов
    // startPoint - для физики (в направлении attachPoint)
    // visualStartPoint - для рендеринга (с боков тела)
    void updateGrappleStartPoints(const glm::vec3& playerPos, const glm::vec3& playerRight) {
        glm::vec3 hipOffset = glm::vec3(0.0f, -PLAYER_HEIGHT * 0.3f, 0.0f);
        glm::vec3 hipCenter = playerPos + hipOffset;
        
        const float visualSideOffset = 0.4f;
        
        if (leftGrapple.active) {
            // Физическая точка - в направлении к attachPoint
            glm::vec3 toAttach = leftGrapple.attachPoint - hipCenter;
            glm::vec3 horizDir = glm::normalize(glm::vec3(toAttach.x, 0.0f, toAttach.z));
            leftGrapple.startPoint = hipCenter + horizDir * 0.3f;
            leftGrapple.length = glm::length(leftGrapple.attachPoint - leftGrapple.startPoint);
            
            // Визуальная точка - с левой стороны тела
            leftGrapple.visualStartPoint = hipCenter - playerRight * visualSideOffset;
        }
        if (rightGrapple.active) {
            // Физическая точка - в направлении к attachPoint
            glm::vec3 toAttach = rightGrapple.attachPoint - hipCenter;
            glm::vec3 horizDir = glm::normalize(glm::vec3(toAttach.x, 0.0f, toAttach.z));
            rightGrapple.startPoint = hipCenter + horizDir * 0.3f;
            rightGrapple.length = glm::length(rightGrapple.attachPoint - rightGrapple.startPoint);
            
            // Визуальная точка - с правой стороны тела
            rightGrapple.visualStartPoint = hipCenter + playerRight * visualSideOffset;
        }
    }
};
