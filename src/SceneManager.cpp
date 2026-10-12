#include "SceneManager.h"
#include <algorithm>

SceneManager::SceneManager() = default;

void SceneManager::Update(float dt){
    //Smoothly interpolate current values toward targets
    float speed = ANIM_SPEED * dt;

    if(m_explodedTarget){
        m_explodedCurrent = std::min(1.0F, m_explodedCurrent + speed);
    }else{
        m_explodedCurrent = std::max(0.0F, m_explodedCurrent - speed);
    }

    if(m_sectionTarget)
        m_sectionCurrent = std::min(1.0F, m_sectionCurrent + speed);
    else m_sectionCurrent = std::max(0.0F, m_sectionCurrent - speed);
}

void SceneManager::SetExploded(bool enabled){
    m_explodedTarget = enabled;
}

void SceneManager::SetSection(bool enabled){
    m_sectionTarget = enabled;
}

glm::vec3 SceneManager::GetComponentOffset(size_t idx) const {
    //Each component slides outward along X when exploded
    static const float offsets[5] = {-1.5F, -0.5F, 0.0F, 0.5F, 1.5F};
    if(idx < 5){
        return glm::vec3(offsets[idx] * m_explodedCurrent, 0.0F, 0.0F);
    }
    return glm::vec3(0.0F);
}