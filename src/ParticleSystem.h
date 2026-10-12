#pragma once
/**
    @file ParticleSystem.h
    @brief Visualizes ariflow through the engine with colored particles.
    Particles change color based on temperature:
        Blue ->     Cold intaker air
        White ->    Compressed (warmer)
        Ornage->    Combustion (hot)
        Red   ->    Exhoust (very hot)
*/
#include <vector>
#include <glm/glm.hpp>
#include "App.h"
#include "Renderer.h"

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life = 0.0F;
    float temperature = 288.0F;
    float size = 0.05F;
};

class ParticleSystem{
    public:
        ParticleSystem();

        void Update(float dt, float engineSpeed, float throttle);
        void GenerateVertices(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
        void SetEnabled(bool enabled) {m_enabled = enabled;}
    private:
        void      SpawnParticle();
        glm::vec3 GetEnginePath(float t) const; // t from 0 (intaker) to 1 (nozzle exit)
        glm::vec3 GetTemperatureColor(float tempK) const;

        std::vector<Particle> m_particles;
        static constexpr size_t MAX_PARTICLES = 500;
        float m_spawnTimer = 0.0F;
        bool m_enabled = true;
};
