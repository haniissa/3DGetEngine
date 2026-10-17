#include "ParticleSystem.h"
#include <algorithm>
#include <glm/gtc/constants.hpp>

ParticleSystem::ParticleSystem() = default;

void ParticleSystem::Update(float dt, float engineSpeed, float throttle){
	if(!m_enabled or engineSpeed < 1.0) return;

	//Spawn new particles based engine speed
	m_spawnTimer += dt;
	float spawnInterval = 0.05F / (throttle + 0.1F);

	while(m_spawnTimer > spawnInterval and m_particles.size() < MAX_PARTICLES){
		SpawnParticle();
		m_spawnTimer -= spawnInterval;
	}

	//Update exiting particles
	float speed = 2.0F *  (engineSpeed/ 15000.0F)+ 0.05F;

	for(auto& p:m_particles){
		p.life += dt * speed * 0.15F;
		if(p.life > 1.0F){
			p.life = 0.0F; //recycle
			p.position = GetEnginePath(0.0F);
		}else{
			p.position = GetEnginePath(p.life);
		}
		//Temperature changer along the path
		if(p.life < 0.2F){
			p.temperature = 288.0F + p.life * 200.0F;//Intake warming
		}else if(p.life < 0.4F){
			p.temperature = 330.0F + (p.life - 0.2F) * 2000.0F;// Compression
		}else if(p.life < 0.6F){
			p.temperature  = 730.0F + (p.life - 0.4F) * 6000.0F; //Combustion
		}else if(p.life < 0.8F){
			p.temperature  = 1930.0F + (p.life - 0.6F) * 3000.0F; //CExpansion

		}else{
			p.temperature  = 1330.0F + (p.life - 0.8F) * 2000.0F; //CExpansion

		}
	}
}

void ParticleSystem::GenerateVertices(std::vector<Vertex>& outVertices,
	std::vector<uint32_t>& outIndices){
		outVertices.clear();
		outIndices.clear();
		if(!m_enabled) return;

		for(const auto& p:m_particles){
			size_t base = outVertices.size();
			glm::vec3 color = GetTemperatureColor(p.temperature);

			// Fade in on birth, out as the gas leaves the nozzle
			float alpha = 1.0F;
			if(p.life > 0.12F) alpha = p.life / 0.12F;
			else if(p.life > 0.70F) alpha = (1.0F - p.life) / 0.30F;
			alpha = std::clamp(alpha, 0.0F, 1.0F) * 0.9F;


			//All four corner sitt ON the particle; the vertex shader pushes
			//them apart in view space so the quad always faces the camera
			//Small billboard quad for each particles
			float s = p.size;
			outVertices.push_back({p.position + glm::vec3(-s, -s, 0.0F), color, {0.0F,0.0F,1.0F}, {0.0F, 0.0F}});
			outVertices.push_back({p.position + glm::vec3(s, -s, 0.0F), color, {0.0F, 0.0F, 1.0f}, {1.0F, 0.0F}});
			outVertices.push_back({p.position + glm::vec3(s, s, 0.0F), color, {0.0F, 0.0F, 1.0F}, {1.0F,1.0F}});
			outVertices.push_back({p.position + glm::vec3(-s, s, 0.0F), color, {0.0F,0.0F,1.0F}, {0.0F,1.0F}});

			outIndices.push_back(static_cast<uint32_t>(base + 0));
			outIndices.push_back(static_cast<uint32_t>(base + 1));
			outIndices.push_back(static_cast<uint32_t>(base + 2));
			outIndices.push_back(static_cast<uint32_t>(base + 0));
			outIndices.push_back(static_cast<uint32_t>(base + 2));
			outIndices.push_back(static_cast<uint32_t>(base + 3));
		}
	}

	void ParticleSystem::SpawnParticle(){
		Particle p;
		p.life = 0.0F;
		p.position = GetEnginePath(0.0F);
		p.temperature = 288.0F;
		p.size = 0.03F + static_cast<float>(rand()) / RAND_MAX * 0.02F;
		m_particles.push_back(p);

		if(m_particles.size() > MAX_PARTICLES){
			m_particles.erase(m_particles.begin());
		}
	}

glm::vec3 ParticleSystem::GetEnginePath(float t) const {
	//Parametric path through the engine (X-axis is length)
	float x =t * 5.8F; // Engine length is ~5.8 units

	//Add some swirl/trubulence
	float swirl = sinf(t * 10.0F) * 0.1F * t;
	float radius = 0.2F + t * 0.1F;
	float angle = t * 20.0F; // Rotation as air moves through

	return glm::vec3(
		x,
		radius * cosf(angle) + swirl,
		radius * sinf(angle) + swirl
	);
}


glm::vec3 ParticleSystem::GetTemperatureColor(float tempK) const {
    // Color map: Blue (cold) → White → Yellow → Orange → Red (hot)
    if (tempK < 300.0f) return glm::vec3(0.3f, 0.5f, 1.0f); // Blue
    if (tempK < 500.0f) return glm::vec3(0.6f, 0.8f, 1.0f); // Light blue
    if (tempK < 800.0f) return glm::vec3(1.0f, 1.0f, 1.0f); // White
    if (tempK < 1200.0f) return glm::vec3(1.0f, 0.9f, 0.5f); // Yellow
    if (tempK < 1600.0f) return glm::vec3(1.0f, 0.6f, 0.2f); // Orange
    return glm::vec3(1.0f, 0.2f, 0.1f); // Red
}
