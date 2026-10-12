#pragma once 
/**
    @file SceneManager.h
    @brief Manages visual states: assembled, exploded, sectioned.

    Smoothly animates component positions so the user can "see inside"
    the engine without loading separate models.
*/
#include "App.h"
#include <glm/glm.hpp>
#include <vector>

class SceneManager{
    public:
        SceneManager();

        void Update(float dt);

        //View toggles 
        void SetExploded(bool enabled);
        void SetSection(bool enabled);
        void SetShowAirflow(bool enabled){m_showAirflow = enabled;}

        bool IsExploded() const {return m_explodedTarget > 0.5F;}
        bool IsSection() const {return m_sectionTarget > 0.5F;}
        bool isShowingAirflow() const { return m_showAirflow;}

        //Current animation values (0 to 1)
        float GetExplodedFactor() const {return m_explodedCurrent;}
        float GetSectionFactor() const {return m_sectionCurrent;}

        //Component offsets for exploded view (5 components)
        glm::vec3 GetComponentOffset(size_t idx) const;
    private:
        bool m_explodedTarget = false;
        bool m_sectionTarget = false;
        float m_explodedCurrent = 0.0F;
        float m_sectionCurrent = 0.0F;
        bool m_showAirflow = true;
        static constexpr float ANIM_SPEED = 2.0F;
};
