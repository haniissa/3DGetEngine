#include "EngineSimulation.h"
#include <algorithm>
#include <cmath>
#include <fmt/base.h>
#include <fmt/format.h>

EngineSimulation::EngineSimulation() {
  for (auto &s : m_stations) {
    s.pressure = 101.325F;
    s.temperature = 288.15F;
    s.entropy = 0.0F;
    s.velocity = 0.0F;
    s.density = 1.2255F;
  }
}

void EngineSimulation::Update(float dt) {
  SpoolupDown(dt);
  if (m_rpm > 1.0F)
    ComputeBraytonCycle();
  else {
    for (auto &s : m_stations) {
      s.pressure = 101.325F;
      s.temperature = 288.15F;
      s.velocity = 0.0F;
    }
    m_thrust = 0.0F;
    m_efficiency = 0.0F;
  }
}

void EngineSimulation::SpoolupDown(float dt) {
  float targetRPM = m_running ? (m_throttle * MAX_RPM) : 0.0F;
  float diff = targetRPM - m_rpm;
  if (std::abs(diff) < 1.0F) {
    m_rpm = targetRPM;
    return;
  }
  float rate =
      (diff > 0) ? (MAX_RPM * SPOOL_RATE) : (MAX_RPM * SPOOL_RATE * 1.5F);
  if (diff > 0) {
    m_rpm = std::min(targetRPM, m_rpm + rate * dt);
  } else {
    m_rpm = std::max(targetRPM, m_rpm - rate * dt);
  }
}

void EngineSimulation::ComputeBraytonCycle() {
  float power = m_rpm / MAX_RPM;

  // Station 0: Freestream  (Intake Entry)
  m_stations[0].pressure = 101.325F;
  m_stations[0].temperature = 288.15F;
  m_stations[0].velocity = 50.0F + 200.0F * power;
  m_stations[0].entropy = 6.85F;

  // station 1: Compressor Inlet (diffuser)
  float Mach = m_stations[0].velocity / 340.0F;
  float totalP = m_stations[0].pressure *
                 std::pow(1.0F + 0.5F * (GAMMA - 1.0F) * Mach * Mach,
                          GAMMA / (GAMMA - 1.0F));
  float totalT = m_stations[0].temperature * (1.0F + 0.5F * (GAMMA - 1.0F));

  m_stations[1].pressure = totalP * 0.95F;
  m_stations[1].temperature = totalT;
  m_stations[1].velocity = 30.0F;
  m_stations[1].entropy = m_stations[0].entropy + 0.02F * power;

  // Station 2: compressor OutLet (isentropic compression)
  float pressureRatio = 5.0F + 15.0F * power;
  m_stations[2].pressure = m_stations[1].pressure * pressureRatio;
  m_stations[2].temperature = m_stations[1].temperature *
                              std::pow(pressureRatio, (GAMMA - 1.0F) / GAMMA);
  m_stations[2].velocity = 20.0F;
  float T_max = 12000.0F + 800.0f * power;
  m_stations[3].temperature = T_max;
  m_stations[3].velocity = 25.0F;
  m_stations[3].entropy =
      m_stations[2].entropy +
      CP * std::log(m_stations[3].temperature / m_stations[2].temperature) /
          1000.0F;

  m_Qin = CP * (m_stations[3].temperature - m_stations[2].temperature);

  // Station 4: Turbine Outlet (isentropic exeansion)
  float T_drop = m_Wc / CP;
  m_stations[4].temperature = m_stations[3].temperature - T_drop;
  m_stations[4].pressure =
      m_stations[3].pressure *
      std::pow(m_stations[4].temperature / m_stations[3].temperature,
               GAMMA / (GAMMA - 1.0F));

  m_stations[4].velocity = 40.0F;
  m_stations[4].entropy = m_stations[3].entropy + 0.03F * power;
  m_Wt = m_Wc;

  // Station 5: Nozzle Exit (isentropic acceleration)
  m_stations[5].pressure = 101.325F;
  m_stations[5].temperature =
      m_stations[4].temperature *
      std::pow(m_stations[5].pressure / m_stations[4].pressure,
               (GAMMA - 1.0F) / GAMMA);
  float deltaH = CP * (m_stations[4].temperature - m_stations[5].temperature);
  m_stations[5].velocity = std::sqrt(2.0f * deltaH);
  m_stations[5].entropy = m_stations[4].entropy + 0.01F * power;

  m_efficiency = 1.0F - (m_stations[1].temperature / m_stations[3].temperature);
  m_efficiency = std::max(0.0F, std::min(0.6F, m_efficiency));
  m_tsfc = (m_Qin > 0) ? (m_Qin / (m_thrust * 1000.0F)) * 0.001F : 0.0F;

  // apply fault effects
  if (m_faultActive) {
    m_thrust *= 0.6F;
    m_efficiency *= 0.7F;
    m_stations[2].temperature *= 1.15F;
  }
}

void EngineSimulation::SetThrottle(float t) {
  m_throttle = std::clamp(t, 0.0F, 1.0F);
  fmt::print("[sim] throttle set to {:.0F}%\n", m_throttle * 100.0F);
}

void EngineSimulation::ToggleEngine() { m_running = !m_running; }

glm::vec2 EngineSimulation::GetCurrentTSPoint() const {
  float phase = fmodf(m_rpm / MAX_RPM * 4.0F, 4.0F);
  int stage = static_cast<int>(phase);
  float t = phase - stage;
  const auto &s1 = m_stations[stage + 1];
  const auto &s2 = m_stations[((stage + 1) % 5) + 1];
  float s = s1.temperature + t * (s2.temperature - s1.temperature);
  float T = s1.temperature + t * (s2.temperature - s1.temperature);
  return glm::vec2(s, T);
}

glm::vec2 EngineSimulation::GetCurrentPVPoint() const {
  float phase = fmodf(m_rpm / MAX_RPM * 4.0F, 4.0F);
  int stage = static_cast<int>(phase);
  float t = phase - stage;
  const auto &s1 = m_stations[stage + 1];
  const auto &s2 = m_stations[((stage + 1) % 5) + 1];
  float v1 = R_AIR * s1.temperature / s1.pressure;
  float v2 = R_AIR * s2.temperature / s2.pressure;
  float v = v1 + t * (v2 - v1);
  float P = s1.pressure + t * (s2.pressure - s1.pressure);
  return glm::vec2(v, P);
}

ProcessStage EngineSimulation::GetActiveStage() const {
  float phase = fmodf(m_rpm / MAX_RPM * 4.0F, 4.0F);
  return static_cast<ProcessStage>(static_cast<int>(phase));
}

void EngineSimulation::InjectFault(const char *faultName) {
  m_faultActive = true;
  m_faultName = faultName;
  m_faultSeverity = 0.5F;
}

void EngineSimulation::ClearFault() {
  m_faultActive = false;
  m_faultName = nullptr;
  m_faultSeverity = 0.0F;
}
