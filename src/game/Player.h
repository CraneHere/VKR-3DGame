#pragma once

#include <SDL.h>
#include <glm/glm.hpp>
#include <PxPhysicsAPI.h>
#include "../core/Constants.h"
#include "ODMGear.h"

using namespace physx;

class Player {
public:
    PxController* controller = nullptr;
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 slideVelocity = glm::vec3(0.0f);  // Инерция от притягивания
    glm::vec3 airMomentum = glm::vec3(0.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 previousPosition = glm::vec3(0.0f);
    float realSpeed = 0.0f;
    float horizontalSpeed = 0.0f;
    
    float yaw = -90.0f;
    float pitch = 0.0f;
    float mouseSensitivity = 0.1f;
    
    bool isGrounded = true;
    bool flyMode = false;      // Режим полёта
    bool flyTogglePressed = false;  // Защита от повторного срабатывания
    
    ODMGear odmGear;
    bool wasInODMAir = false;
    
    Player() = default;
    
    void setController(PxController* ctrl) {
        controller = ctrl;
        updatePosition();
    }
    
    void updatePosition() {
        if (controller) {
            PxExtendedVec3 pos = controller->getPosition();
            position = glm::vec3((float)pos.x, (float)pos.y, (float)pos.z);
        }
    }
    
    glm::vec3 getForward() const {
        return glm::normalize(glm::vec3(cos(glm::radians(yaw)), 0.0f, sin(glm::radians(yaw))));
    }
    
    glm::vec3 getRight() const {
        return glm::normalize(glm::cross(getForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    
    glm::vec3 getCameraForward() const {
        return glm::normalize(glm::vec3(
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        ));
    }
    
    glm::vec3 getCameraPosition() const {
        return position + glm::vec3(0.0f, PLAYER_HEIGHT * 0.45f, 0.0f);
    }
    
    void handleMouseMovement(float xrel, float yrel) {
        yaw += xrel * mouseSensitivity;
        pitch -= yrel * mouseSensitivity;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }
    
    void update(float deltaTime, const Uint8* keys) {
        glm::vec3 forward = getForward();
        glm::vec3 right = getRight();
        glm::vec3 camForward = getCameraForward();
        
        glm::vec3 displacement(0.0f);
        
        odmGear.isActive = odmGear.leftGrapple.active || odmGear.rightGrapple.active;
        
        // Переключение режима полёта: Space + Alt
        bool flyToggle = keys[SDL_SCANCODE_SPACE] && keys[SDL_SCANCODE_LALT];
        if (flyToggle && !flyTogglePressed) {
            flyMode = !flyMode;
            velocity = glm::vec3(0.0f);  // Сбросить скорость
        }
        flyTogglePressed = flyToggle;
        
        // Режим полёта
        if (flyMode) {
            float flySpeed = keys[SDL_SCANCODE_LSHIFT] ? 100.0f : 30.0f;
            
            glm::vec3 moveDir(0.0f);
            if (keys[SDL_SCANCODE_W]) moveDir += camForward;
            if (keys[SDL_SCANCODE_S]) moveDir -= camForward;
            if (keys[SDL_SCANCODE_A]) moveDir -= right;
            if (keys[SDL_SCANCODE_D]) moveDir += right;
            if (keys[SDL_SCANCODE_SPACE] && !keys[SDL_SCANCODE_LALT]) moveDir.y += 1.0f;  // Вверх
            if (keys[SDL_SCANCODE_LCTRL]) moveDir.y -= 1.0f;  // Вниз
            
            if (glm::length(moveDir) > 0.0f) {
                moveDir = glm::normalize(moveDir);
            }
            
            displacement = moveDir * flySpeed * deltaTime;
            
        } else if (odmGear.isActive && !isGrounded) {
            // Восстановить скорость если только что перезацепились
            float horizSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
            float momentumSpeed = glm::length(airMomentum);
            if (horizSpeed < 1.0f && momentumSpeed > 1.0f) {
                velocity.x = airMomentum.x;
                velocity.z = airMomentum.z;
                airMomentum = glm::vec3(0.0f);
            }
            
            wasInODMAir = true;
            
            // Применяем гравитацию
            velocity.y -= GRAVITY * deltaTime;
            
            // W - подтягивание + плавный поворот в направлении взгляда
            if (keys[SDL_SCANCODE_W]) {
                float currentSpeed = glm::length(velocity);
                if (currentSpeed > 1.0f) {
                    glm::vec3 currentDir = velocity / currentSpeed;
                    float steerRate = 3.0f;
                    float t = glm::clamp(steerRate * deltaTime, 0.0f, 1.0f);
                    glm::vec3 newDir = glm::normalize(glm::mix(currentDir, camForward, t));
                    velocity = newDir * currentSpeed;
                }
                velocity += camForward * GRAPPLE_REEL_SPEED * 0.3f * deltaTime;
            }
            if (keys[SDL_SCANCODE_S]) {
                velocity -= camForward * GRAPPLE_REEL_SPEED * 0.3f * deltaTime;
            }

            if (keys[SDL_SCANCODE_SPACE]) {
                velocity += camForward * GAS_BOOST_FORCE * deltaTime;
            }
            
            // A/D - поворот в воздухе
            {
                float currentSpeed = glm::length(velocity);
                if (currentSpeed > 1.0f) {
                    glm::vec3 currentDir = velocity / currentSpeed;
                    float sideSteerRate = 2.0f;
                    float t = glm::clamp(sideSteerRate * deltaTime, 0.0f, 1.0f);
                    if (keys[SDL_SCANCODE_A]) {
                        glm::vec3 targetDir = glm::normalize(currentDir - right * 0.5f);
                        velocity = glm::normalize(glm::mix(currentDir, targetDir, t)) * currentSpeed;
                    }
                    if (keys[SDL_SCANCODE_D]) {
                        glm::vec3 targetDir = glm::normalize(currentDir + right * 0.5f);
                        velocity = glm::normalize(glm::mix(currentDir, targetDir, t)) * currentSpeed;
                    }
                }
            }
            
            // Применяем физику тросов
            odmGear.applyGrapplePhysics(velocity, position, controller, deltaTime, keys);
            
            // Воздушное сопротивление
            float airResistance = 1.0f - (AIR_DRAG * deltaTime);
            velocity.x *= airResistance;
            velocity.z *= airResistance;
            
            // 6. Ограничение максимальной скорости
            float maxSpeed = 100.0f;
            float currentSpeed = glm::length(velocity);
            if (currentSpeed > maxSpeed) {
                velocity = glm::normalize(velocity) * maxSpeed;
            }
            
            displacement = velocity * deltaTime;
            
        } else {
            // Обычная физика

            if (wasInODMAir && !isGrounded) {
                airMomentum = glm::vec3(velocity.x, 0.0f, velocity.z);
                velocity.x = 0.0f;
                velocity.z = 0.0f;
            }
            wasInODMAir = false;

            glm::vec3 moveDir(0.0f);
            if (keys[SDL_SCANCODE_W]) moveDir += forward;
            if (keys[SDL_SCANCODE_S]) moveDir -= forward;
            if (keys[SDL_SCANCODE_A]) moveDir -= right;
            if (keys[SDL_SCANCODE_D]) moveDir += right;
            
            if (glm::length(moveDir) > 0.0f) moveDir = glm::normalize(moveDir);
            
            glm::vec3 horizontalMove;

            if (isGrounded) {
                horizontalMove = moveDir * WALK_SPEED * deltaTime;
                airMomentum = moveDir * WALK_SPEED;
            }
            else {
                if (glm::length(airMomentum) > WALK_SPEED) {
                    float decayRate = 3.0f;
                    float decay = exp(-decayRate * deltaTime);
                    float newSpeed = WALK_SPEED + (glm::length(airMomentum) - WALK_SPEED) * decay;
                    airMomentum = glm::normalize(airMomentum) * newSpeed;
                }

                horizontalMove = airMomentum * deltaTime;
            }
            
            // Движение с тросами на земле (Shift + тросы активны)
            if (keys[SDL_SCANCODE_LSHIFT] && odmGear.isActive && isGrounded) {
                // Вычислить центр между точками крепления
                glm::vec3 targetCenter(0.0f);
                int activeCount = 0;
                
                if (odmGear.leftGrapple.active) {
                    targetCenter += odmGear.leftGrapple.attachPoint;
                    activeCount++;
                }
                if (odmGear.rightGrapple.active) {
                    targetCenter += odmGear.rightGrapple.attachPoint;
                    activeCount++;
                }
                
                if (activeCount > 0) {
                    targetCenter /= (float)activeCount;
                    
                    // Горизонтальное направление к центру
                    glm::vec3 toTarget = targetCenter - position;
                    float horizontalDist = glm::length(glm::vec2(toTarget.x, toTarget.z));
                    
                    if (horizontalDist > 0.5f) {
                        glm::vec3 toCenter = glm::normalize(glm::vec3(toTarget.x, 0.0f, toTarget.z));
                        // Перпендикулярные направления (скольжение по дуге)
                        glm::vec3 slideLeft = glm::vec3(toCenter.z, 0.0f, -toCenter.x);
                        glm::vec3 slideRight = glm::vec3(-toCenter.z, 0.0f, toCenter.x);
                        
                        float reelSpeed = GROUND_REEL_SPEED;
                        float brakeDistance = 4.0f;
                        if (horizontalDist < brakeDistance) {
                            float t = horizontalDist / brakeDistance;
                            reelSpeed *= t * t;
                        }

                        // Проверка расстояния между тросами для обрыва при скольжении
                        bool bothGrapplesActive = odmGear.leftGrapple.active && odmGear.rightGrapple.active;
                        float grappleSeparation = 0.0f;
                        if (bothGrapplesActive) {
                            grappleSeparation = glm::length(odmGear.leftGrapple.attachPoint - odmGear.rightGrapple.attachPoint);
                        }
                        
                        bool pressA = keys[SDL_SCANCODE_A];
                        bool pressD = keys[SDL_SCANCODE_D];
                        bool pressW = keys[SDL_SCANCODE_W];
                        bool pressS = keys[SDL_SCANCODE_S];
                        
                        glm::vec3 grappleDir(0.0f);
                        bool hasMovement = false;
                        bool movingLeft = false;
                        bool movingRight = false;
                        
                        // Горизонтальная составляющая
                        if (pressA && !pressD) {
                            grappleDir += slideLeft;
                            movingLeft = true;
                            hasMovement = true;
                        } else if (pressD && !pressA) {
                            grappleDir += slideRight;
                            movingRight = true;
                            hasMovement = true;
                        }
                        
                        // Вертикальная составляющая
                        if (pressW && !pressS) {
                            grappleDir += toCenter;
                            hasMovement = true;
                        } else if (pressS && !pressW) {
                            grappleDir -= toCenter;
                            hasMovement = true;
                        }
                        
                        // Применить движение если есть направление
                        if (hasMovement && glm::length(grappleDir) > 0.1f) {
                            grappleDir = glm::normalize(grappleDir);
                            
                            float speed = reelSpeed;
                            if (pressS && !pressW && !pressA && !pressD) {
                                speed = WALK_SPEED * 2.0f;
                            } else if (pressW && !pressA && !pressD) {
                                speed = reelSpeed * 1.5f;
                            }
                            
                            slideVelocity = grappleDir * speed;
                            horizontalMove = glm::vec3(0.0f);
                            
                            // Обрыв тросов при скольжении в стороны
                            if (bothGrapplesActive && grappleSeparation > GRAPPLE_BREAK_DISTANCE) {
                                if (movingLeft) {
                                    odmGear.releaseRightGrapple();
                                } else if (movingRight) {
                                    odmGear.releaseLeftGrapple();
                                }
                            }
                        }
                    }
                }
            }
            
            // Затухание slideVelocity
            float decayRate = 6.0f;  // Скорость затухания
            float decay = exp(-decayRate * deltaTime);
            slideVelocity *= decay;
            
            // Останавливаем если совсем маленькая
            if (glm::length(slideVelocity) < 0.1f) {
                slideVelocity = glm::vec3(0.0f);
            }
            
            if (keys[SDL_SCANCODE_SPACE] && isGrounded) {
                velocity.y = JUMP_FORCE;
                isGrounded = false;
            }
            
            velocity.y -= GRAVITY * deltaTime;
            if (velocity.y < -40.0f) velocity.y = -40.0f;
            
            float dragFactor = isGrounded ? 0.85f : 0.99f;
            velocity.x *= dragFactor;
            velocity.z *= dragFactor;
            
            displacement = horizontalMove + slideVelocity * deltaTime + velocity * deltaTime;
        }
        
        PxControllerFilters filters;
        PxControllerCollisionFlags collisionFlags = controller->move(
            PxVec3(displacement.x, displacement.y, displacement.z),
            0.0001f,
            deltaTime,
            filters
        );
        
        // Проверка земли через raycast
        bool groundedByRaycast = false;
        if (controller && velocity.y <= 0.1f) {
            PxScene* scene = controller->getScene();
            PxExtendedVec3 pos = controller->getPosition();

            float capsuleHalfHeight = (PLAYER_HEIGHT - 2.0f * PLAYER_RADIUS) * 0.5f;
            float footY = (float)pos.y - capsuleHalfHeight - PLAYER_RADIUS;
            
            PxVec3 origin((float)pos.x, footY + 0.05f, (float)pos.z);
            PxVec3 direction(0.0f, -1.0f, 0.0f);
            PxRaycastBuffer hit;

            float rayDistance = 0.15f;
            
            PxQueryFilterData filterData;
            filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;
            
            if (scene->raycast(origin, direction, rayDistance, hit, PxHitFlag::eDEFAULT, filterData)) {
                groundedByRaycast = true;
            }
        }

        if ((collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN) || groundedByRaycast) {
            isGrounded = true;
            velocity.y = 0.0f;
        } else {
            isGrounded = false;
        }
        
        if (collisionFlags & PxControllerCollisionFlag::eCOLLISION_UP) {
            velocity.y = 0.0f;
        }
        
        if (odmGear.isActive && (collisionFlags & PxControllerCollisionFlag::eCOLLISION_SIDES)) {
            velocity.x *= 0.5f;
            velocity.z *= 0.5f;
        }
        
        glm::vec3 oldPos = position;
        updatePosition();
        
        // Вычисляем реальную скорость по изменению позиции
        glm::vec3 posDelta = position - oldPos;
        realSpeed = glm::length(posDelta) / deltaTime;
        horizontalSpeed = glm::length(glm::vec2(posDelta.x, posDelta.z)) / deltaTime;
        
        previousPosition = position;
        
        odmGear.update(deltaTime, isGrounded);
        odmGear.updateGrappleStartPoints(position, getRight());
    }
};
