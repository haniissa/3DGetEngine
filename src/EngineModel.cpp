#include "EngineModel.h"
#include "App.h"
#include "Renderer.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>

EngineModel::EngineModel() = default;

void EngineModel::GenerateGeometry(){
	m_vertices.clear();
	m_indices.clear();
	m_components.clear();

	GenerateIntake();
	GenerateCompressor();
	GenerateCombustor();
	GenerateTurbine();
	GenerateNozzle();
	GenerateShaft();
}

void EngineModel::GenerateLathe(const std::vector<glm::vec2>& profile,
	const glm::vec3&color, float xOffset,
	bool isHollow){
		const int segments{48};
		int profileSize = static_cast<int>(profile.size());
		size_t baseVertex = m_vertices.size();


		//Normal of a lathe surface = perpendicular to the profile tangent,
		// //rotated around the axis: n3 = (n.x, n.y * cosA, n.y * sinA)
		std::vector<glm::vec2> profileNormals(profileSize);
		for(int j{}; j < profileSize; ++j){
			glm::vec2 tangent(0.0F);
			if(j > 0){
				glm::vec2 d = profile[j] - profile[j - 1];
				if(glm::length(d) > 1e-6F) tangent += glm::normalize(d);
			}
			if(j < profileSize - 1){
				glm::vec2 d = profile[j + 1] - profile[j];
				if(glm::length(d) > 1e-6f) tangent += glm::normalize(d);
			}
			if(glm::length(tangent) < 1e-5F) tangent = glm::vec2(1.0F, 0.0F);
			tangent = glm::normalize(tangent);
			profileNormals[j] = glm::vec2(-tangent.y, tangent.x); //rotate 90 deg
		}

		//size_t baseVertes = m_vertices.size();
		for (int i{}; i <= segments; ++i){
			float angle = static_cast<float>(i) / segments * 2.0F * glm::pi<float>();
			float cosA = cosf(angle);
			float sinA = sinf(angle);

			for (int j{}; j < profileSize; ++j){
				const glm::vec2& pt =  profile[j];
				Vertex v;
				v.position = glm::vec3(pt.x + xOffset, pt.y * cosA, pt.y * sinA);
				v.color = color;
				v.normal   = glm::normalize(glm::vec3(profileNormals[j].x,
								 profileNormals[j].y * cosA,
								 profileNormals[j].y * sinA));
				v.texCoord  = glm::vec2(static_cast<float>(i) / segments, pt.y);
				m_vertices.push_back(v);
			}
		}
		//2. INDICES: join neighbouring vetices int triangles
		for(int i{}; i < segments; ++i){
			for(int j{}; j < profileSize - 1; ++j){
				uint32_t a = static_cast<uint32_t>(baseVertex + i * profileSize + j) ;
				uint32_t b = static_cast<uint32_t>(baseVertex + (i + 1) * profileSize + j) ;
				uint32_t c = static_cast<uint32_t>(baseVertex + (i + 1) * profileSize + (j + 1)) ;
				uint32_t d = static_cast<uint32_t>(baseVertex + i * profileSize + (j + 1)) ;
				m_indices.push_back(a); m_indices.push_back(b); m_indices.push_back(c);
				m_indices.push_back(a); m_indices.push_back(c); m_indices.push_back(d);
			}
		}

	}

void EngineModel::GenerateIntake(){
	std::vector<glm::vec2> profile = {
		{0.0F, 1.2F},
		{0.3F, 1.15F},
		{0.6F, 1.05F},
		{0.9F, 0.9F},
		{1.2F, 0.75F},
	};

	ComponentInfo info;
	info.type = EngineComponent::Intake;
	info.name = "Intake";
	info.baseColor = glm::vec3(0.7F, 0.75F, 0.8F);
	info.xStart = 0.0F;
	info.xEnd   = 1.2F;
	info.radiusIn = 0.75F;
	info.radiusOut = 1.2F;
	m_components.push_back(info);

	GenerateLathe(profile, info.baseColor, 0.0F);
}

void EngineModel::GenerateCompressor(){
	std::vector<glm::vec2> profile = {
		{1.2F, 0.75F},
		{1.3F, 0.78F},
		{1.5F, 0.8F},
		{1.7F, 0.78F},
		{1.9F, 0.75F},
		{2.1F, 0.75F},
	};

	ComponentInfo info;
	info.type = EngineComponent::Combustor;
	info.name = "Compressor";
	info.baseColor = glm::vec3(0.4F, 0.6F, 0.9F);
	info.xStart = 1.2F;
	info.xEnd = 2.1F;
	info.radiusIn = 0.72F;
	info.radiusOut = 0.8F;
	m_components.push_back(info);

	GenerateLathe(profile, info.baseColor, 0.0F);
	GenerateBladeRing(0.75F, 1.4F, 12, 0.08F, glm::vec3(0.3F, 0.5F, 0.8F ));
}
void EngineModel::GenerateCombustor(){
	std::vector<glm::vec2> profile = {
	 	{2.1f, 0.72f},
        {2.2f, 0.85f},
        {2.5f, 0.9f},
        {2.8f, 0.85f},
        {3.0f, 0.72f}
	};
	ComponentInfo info;
	info.type = EngineComponent::Combustor;
	info.name = "Combustor";
	info.baseColor = glm::vec3(0.9F, 0.5F, 0.2F);
	info.xStart = 2.1F;
	info.xEnd = 3.0F;
	info.radiusIn = 0.72F;
	info.radiusOut = 0.9F;
	m_components.push_back(info);

	GenerateLathe(profile, info.baseColor, 0.0F);
}

void EngineModel::GenerateTurbine(){
	std::vector<glm::vec2> profile = {
		{3.0f, 0.72f},
        {3.1f, 0.75f},
        {3.3f, 0.78f},
        {3.5f, 0.75f},
        {3.7f, 0.72f},
        {3.9f, 0.7f}
	};

	ComponentInfo info;
	info.type = EngineComponent::Turbine;
	info.name = "Turbine";
	info.baseColor = glm::vec3(0.8F, 0.3F, 0.3F);
	info.xStart = 3.0F;
	info.xEnd = 3.9F;
	info.radiusIn = 0.7F;
	info.radiusOut = 0.78F;
	m_components.push_back(info);

	GenerateLathe(profile, info.baseColor, 0.0F);
	GenerateBladeRing(0.74F, 3.2F, 10, 0.0F, glm::vec3(0.7F, 0.2F, 0.2F));
	GenerateBladeRing(0.74F, 3.6f, 10, 0.06F, glm::vec3(0.7F, 0.2F, 0.2F));
}

void EngineModel::GenerateNozzle(){
	std::vector<glm::vec2> profile = {
		{3.9f, 0.7f},
        {4.1f, 0.65f},
        {4.3f, 0.6f},
        {4.5f, 0.55f},
        {4.7f, 0.5f},
        {4.9f, 0.45f},
        {5.2f, 0.5f},
        {5.5f, 0.7f},
        {5.8f, 0.9f}
	};

	ComponentInfo info;
	info.type = EngineComponent::Turbine;
	info.name = "Nozzle";
	info.baseColor = glm::vec3(0.5F, 0.5F, 0.5F);
	info.xStart = 3.9F;
	info.xEnd = 5.8F;
	info.radiusIn = 0.45F;
	info.radiusOut = 0.9F;
	m_components.push_back(info);

	GenerateLathe(profile, info.baseColor, 0.0F);
}

void EngineModel::GenerateShaft(){
	std::vector<glm::vec2> profile = {
		{1.2f, 0.15f},
        {2.1f, 0.15f},
        {2.1f, 0.12f},
        {3.0f, 0.12f},
        {3.0f, 0.15f},
        {3.9f, 0.15f},
        {3.9f, 0.1f},
        {1.2f, 0.1f}
	};
	GenerateLathe(profile, glm::vec3(0.3F, 0.3F, 0.3F), 0.0F);
}

void EngineModel::GenerateBladeRing(float radius, float xPos, int bladeCount,
	float bladeHeight, const glm::vec3& color){
		for(int i{}; i < bladeCount; ++i){
			float angle = static_cast<float>(i) / bladeCount * 2.0f * glm::pi<float>();
			float cosA = cosf(angle);
			float sinA = sinf(angle);

			glm::vec3 center(xPos, radius * cosA, radius * sinA);
			size_t base = m_vertices.size();

			glm::vec3 dx(bladeHeight, 0.0F, 0.0F);
			glm::vec3 dy(0.0F, bladeHeight * 0.3F * cosA, bladeHeight * 0.3F * sinA);
			glm::vec3 dz(0.0F, -bladeHeight * 0.1F * sinA, bladeHeight * 0.1F * cosA);

			m_vertices.push_back({center - dx - dy - dz, color, glm::vec3(cosA, sinA, 0.0F), {0,0}});
			m_vertices.push_back({center + dx - dy - dz, color, glm::vec3(cosA, sinA, 0.0F), {1,0}});
			m_vertices.push_back({center + dx + dy - dz, color, glm::vec3(cosA, sinA, 0.0F), {1,1}});
			m_vertices.push_back({center - dx + dy - dz, color, glm::vec3(cosA, sinA, 0.0F), {0,1}});
			m_vertices.push_back({center - dx - dy + dz, color, glm::vec3(cosA, sinA, 0.0F), {0,0}});
			m_vertices.push_back({center + dx - dy + dz, color, glm::vec3(cosA, sinA, 0.0F), {1,0}});
			m_vertices.push_back({center + dx + dy + dz, color, glm::vec3(cosA, sinA, 0.0F), {1,0}});
			m_vertices.push_back({center - dx + dy + dz, color, glm::vec3(cosA, sinA, 0.0F), {1,0}});

			uint32_t idx[] = {
				0,1,2, 0,2,3,
				4,6,5, 4,7,6,
				0,4,5, 0,5,1,
				2,6,7, 2,7,3,
				0,2,7, 0,7,4,
				1,5,6, 1,6,2
			};
			for (uint32_t id:idx){
				m_indices.push_back(static_cast<uint32_t>(base + id));
			}
		}
	}

void EngineModel::SetExploadedView(bool enabled, float t){
	m_exploded = enabled;
	m_exploded = t;
}
void EngineModel::SetSectionView(bool enabled){
	m_section = enabled;
}

glm::mat4 EngineModel::GetComponentTransform(size_t index) const {
	glm::mat4 transform = glm::mat4(1.0F);
	if(m_exploded && index < m_components.size()){
		float offsets[] ={-1.5F, -0.5F, 0.0F, 0.5F, 1.5F};
		transform = glm::translate(transform, glm::vec3(offsets[index] * m_explodedT, 0.0F, 0.0F));
	}
	return transform;
}
