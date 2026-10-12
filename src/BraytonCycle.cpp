#include "BraytonCycle.h"
#include <cmath>
#include <vector>

std::vector<CyclePoint> BraytonCycle::GenerateIdealCycle(float pressureRatio, float T_max){
	std::vector<CyclePoint> points;
	float T1 = 288.15F;
	float P1 = 101.325F;
	float gamma = 1.4F;
	float cp = 1005.0F;
	float R = 287.0F;

	//Point 1: Intake exit / Compressor inlet
	float T2 = T1 * std::pow(pressureRatio, (gamma - 1.0F)/ gamma);
	float P2 = P1 * pressureRatio;
	float s2 = points[0].ts.x + cp * std::log(T2 / T1) / 1000.0F;
	points.push_back({{s2, T2}, {R * T2 / P2,  P2}, "2: Compressor"});

	//Point 3: Combustor outlet (constant pressure heat addition)
	float T3 = T_max;
	float P3 = P2;
	float s3 = s2 + cp * std::log(T3 / T2) / 1000.0F;
	points.push_back({{s3, T3}, {R * T3 / P3, P3}, "3: Combustor"});

	//Point 4: Tubine outLet (isentropic expansion)
	float T4 = T3 * std::pow(1.0F / pressureRatio, (gamma - 1.0F) / gamma);
	float P4 = P3 / pressureRatio;
	float s4 = s3 + cp * std::log(T4 / T3) / 1000.0F;
	points.push_back({{s4, T4}, {R * T4 / P4, P4}, "4: Turbine"});

	//Point 5P: Nozzle exit (back to ambient pressure)
	float T5 = T4 * std::pow(P1 / P4, (gamma - 1.0F) / gamma);
	float s5 = s4 + cp * std::log(T5 / T4) / 1000.0F;
	points.push_back({{s5, T5}, {R * T5 / P1, P1}, "s: Nozzle"});

	return points;
}

std::vector<glm::vec2> BraytonCycle::GeneratIsentropicTS(float s1, float T1, float T2, int segments){
	std::vector<glm::vec2> points;
	for(int i{}; i <= segments; ++i){
		float t = static_cast<float>(i) / segments;
		float T = T1 + t * (T2 - T1);
		float s = s1 + 1005.0F * std::log(T / T1) / 1000.0F;
		points.push_back({s, T});
	}
	return points;
}

std::vector<glm::vec2> BraytonCycle::GenerateIsobarPV(float P, float v1, float v2, int segments){
	std::vector<glm::vec2>points;
	for(int i{}; i <= segments; ++i){
		float t = static_cast<float>(i) / segments;
		float v = v1 +  t * (v2 - v1);
		points.push_back({v, P});
	}
	return points;
}

const char* BraytonCycle::GetProcessname(int stage){
	const char* names[] = {
		"Intake (Isentropic Deceleration)",
        "Compression (Isentropic)",
        "Combustion (Isobaric Heat Addition)",
        "Expansion (Isentropic)",
        "Exhaust (Isentropic Acceleration)"
	};
	return (stage >= 0 && stage < 5) ? names[stage] : "Unknow";
}

const char* BraytonCycle::GetFormula(int stage){
	const char* formulas[] = {
		"T0/T = 1 + ((gamma-1)/ 2) * M^2",
		"T2/T1 = (P2/P1)^((gamma-1)/gamma)",
		"Qin = cp * (T3 - T2)",
		"T4/T3 = (P4 / P3)^((gamma-1)/gamma)",
		"v_exit = sqrt(2 * cp * (T4 - T5)"
	};
	return (stage >= 0 && stage < 5) ? formulas[stage] :"Unknown";
}
