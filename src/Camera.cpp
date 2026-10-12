#include "Camera.h"
#include <algorithm>
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>


Camera::Camera(float apsectRatio) : m_aspectRation(m_aspectRation){
    Recalculate();
}

void Camera::Update(float dt){
    if(m_autoRotate){
        m_yaw += m_autoRotateSpeed * dt;
        if(m_yaw > 360.0F) m_yaw -= 360.0F;
        Recalculate();
    }
}

void Camera::OnMouseMove(float dx, float dy, bool leftButton, bool rightButton){
    if(leftButton){
        //Orbit
        m_yaw += dx * 0.5F;
        m_pitch -= dy * 0.5F;
        m_pitch = std::clamp(m_pitch, -89.0F, 89.0F);
        Recalculate();
    }
    //Right button pan could be added here
}

void Camera::OnScroll(float delta){
    m_distance -= delta * 0.5F;
    m_distance = std::clamp(m_distance, 2.0F, 20.0F);
    Recalculate();
}

void Camera::OnResize(float width, float height){
    if(height > 0.0F){
        m_aspectRation = width / height;
    }
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_target, m_up);
}

glm::mat4 Camera::GetProjectionMatrix() const {
    return glm::perspective(glm::radians(m_fov), m_aspectRation, 0.1F, 100.0F);
}

void Camera::Recalculate(){
    //Convert spherical coordinates to Cartesian
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    m_position.x = m_target.x + m_distance * cosf(pitchRad) * cosf(yawRad);
    m_position.y = m_target.y + m_distance * sinf(pitchRad);
    m_position.z = m_target.z + m_distance * cosf(pitchRad) * sinf(yawRad);
}