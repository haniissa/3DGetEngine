#pragma once
/**
    @file EngineSimulation.h
    @brief Real-time Brayton cycle thermodynamic simulation.

    this is the "brain of the engine. it simulates:
    1.Intake (isentropic deceleration)"
    2. Compressor (isentropic compression, work input)
    3. Combustor (constant-pressure heat addition)
    4. Turbine (isentropic expansion, work extraction)
    5. Nozzle  (isentropic acceleration -> thrust)

    All valuse are normalized for visualization but follow real physics trends
*/

#include <array>
#include <glm/glm.hpp>

// Thermodynamic station data (5 stations for a trubojet)
struct StationData {
  float pressure = 101.325F;   // Kpa
  float temperature = 288.15F; // K (15'C sea level)
  float entropy = 0.0F;        // kJ(kg-K) - relative for diagram
  float velocity = 0.0F;       // m/s
  float density = 1.225F;      // kg/m^3
};

enum class ProcessStage {
  Intake = 0,
  Compression = 1,
  Combustion = 2,
  Expansion = 3,
  Exhaust = 4,
  Count
};

class EngineSimulation {
	private:
	  void ComputeBraytonCycle();
	  void SpoolupDown(float dt);

	  std::array<StationData, 6> m_stations; // 0-intake, 1-comp-in,

	  // State variables
	  bool m_running = false;
	  float m_throttle = 0.0F; // Target throttle
	  float m_rpm = 0.0F;      //   Current RPM (0 to 100%)
	  float m_thrust = 0.0F;
	  float m_tsfc = 0.0f;
	  float m_efficiency = 0.0F;
	  float m_Wc = 0.0F;  // Compressor work
	  float m_Wt = 0.0F;  // Turbine work
	  float m_Qin = 0.0F; // Heat input

	  // Constants (simplififed for visulization)
	  static constexpr float GAMMA = 1.4F;
	  static constexpr float CP = 1005.0F;   // J/(kg-K)
	  static constexpr float R_AIR = 287.0F; // j/(kg-K)
	  static constexpr float MAX_RPM = 15000.0F;
	  static constexpr float SPOOL_RATE =
	      0.03F; // RPM change per second (normalized)

	  // Fault system
	  bool m_faultActive = false;
	  const char *m_faultName = nullptr;
	  float m_faultSeverity = 0.0F;
	public:
		EngineSimulation();
		// = default;

		//Call every frame with delta time (seconds)
		void Update(float dt);

		//User controls
		void SetThrottle(float t); // 0.0 to 1.0
		void ToggleEngine();
		void SetRunning(bool run){m_running = run;}
		bool IsRunning() const {return m_running;}
		float GetThrottle() const {return m_throttle;}
		float GetRPM() const {return m_rpm;}

		//Station accessors (for UI display)
		 const StationData& GetStation(size_t idx) const {return m_stations[idx];}
			const std::array<StationData, 6>& GetAllStations() const {return m_stations;}

			//Cycle performance
			float GetThrust() const {return m_thrust;}
			float GetTSFC() const { return m_tsfc;}
			float GetEfficiency() const {return m_efficiency;}
			float GetCompressorWork() const {return m_Wc;}
			float GetTurbineWork() const {return m_Wt;}
			float GetHeatInput() const {return m_Qin;}

			//For diagram animation: where  is the "current" point in the cycle?
			glm::vec2 GetCurrentTSPoint() const; // (s,T)
			glm::vec2 GetCurrentPVPoint() const; // (v, P)
			ProcessStage GetActiveStage() const;

			//Fault injection for challenges
			void InjectFault(const char* faultName);
			void ClearFault();
			bool HasFault() const {return m_faultActive;}
			const char* GetFaultName() const {return m_faultName;}
};
