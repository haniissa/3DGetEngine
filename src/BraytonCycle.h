/***
    @file BraytonCycle.h
    @brief Pure math helper for generating Brayton cycle diagram curves.

    Provides the ideal cycle points and isentropic curves for plotting
*/

#include <glm/glm.hpp>
#include <vector>


struct CyclePoint {
    glm::vec2 ts; //(entropy, temperature)
    glm::vec2 pv; // (specififc volume, prssure)
    const char* label;
};

class BraytonCycle{
    public:
        //Generate ideal cycle points given pressure ratio and max temptrature
        static std::vector<CyclePoint> GenerateIdealCycle(float pressureRatio, float T_max);

        //Gnerate isentropc curve points for drawing smooth lines
        static std::vector<glm::vec2> GeneratIsentropicTS(float s1, float T1, float T2, int segments);
        static std::vector<glm::vec2> GenerateIsobarPV(float P, float v1, float v2, int segments);


        //Format hepers for UI
        static const char* GetProcessname(int stage);
        static const char* GetFormula(int stage);
};
