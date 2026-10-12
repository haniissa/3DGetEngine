/**
    @file EngineModel.h
    @brif Procedurally generates all 3D geometry fo rthe turbojet engine.

    Instead of loading .obj files. from disk, we BILD the engine form math.
    This is a powerful technique: we define a "lathe" profile and spin it
    around the X-axis to create the compressor, combustor, trubine, and nozzle
 */

#include "App.h"
#include "Renderer.h"
#include <vector>
#include <glm/glm.hpp>

enum class EngineComponent{
    Intake,  // Station 0-1
    Compressor,  //Station 1 - 2 (rotating blades)
    Combustor,   // Station 2- 3 (Where fuel burns)
    Turbine,     // Station  3 - 4 (extracts energy)
    Nozzle,      // Station 4  - 5 (accelerates exhaust)
};

struct ComponentInfo{
    EngineComponent type;
    const char* name;
    glm::vec3 baseColor;
    float xStart;
    float xEnd;
    float radiusIn;
    float radiusOut;
};

class EngineModel{
    public:
        EngineModel();
        ~EngineModel() = default;

        //Generat all engine parts. Call once at startup.
        void GenerateGeometry();

        //Access the generated mesh data
        const std::vector<Vertex>& GetVertices() const {return m_vertices;}
        const std::vector<uint32_t>& GetIndices() const {return m_indices;}

        //Get metdata for UI labeling
        const std::vector<ComponentInfo>& GetComponents() const {return m_components;}

        //Update component transforms for exploded view
        void SetExploadedView(bool enabled, float t);
        void SetSectionView(bool enabled);

        //Get transform for a specific component (for rendering)
        glm::mat4 GetComponentTransform(size_t index) const;
    private:
        void GenerateLathe(const std::vector<glm::vec2>& profile ,
               const glm::vec3& color, float xOffset,
               bool isHollow = false);
        void GenerateBladeRing(float radius, float xPos, int bladeCount,
                float bladeHeight, const glm::vec3& color);

        void GenerateShaft();
        void GenerateIntake();
        void GenerateCompressor();
        void GenerateCombustor();
        void GenerateTurbine();
        void GenerateNozzle();

        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<ComponentInfo> m_components;

        bool m_exploded =false;
        bool m_section  = false;
        float m_explodedT = 0.0F;// 0=assebled, 1-fully exploeded

};
