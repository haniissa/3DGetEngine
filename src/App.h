#pragma once
/**
 * @file App.h
 * @brief Main application class -- the "director" that ties everything
 * together.
 *
 * Think of this as the stage manage: it opens the theter (window),
 * hires the crew (renderer, UI, simulation), and runs the show (main loop)
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <fmt/format.h>
#include <memory>

// Forward declarations --we only need pointers here, not full definitions.
// This speeds up compilation (a  key C++ best practice).
class Renderer;
class EngineModel;
class EngineSimulation;
class Camera;
class UIManager;
class SceneManager;
class ParticleSystem;

class App {
public:
  App();  // Constructor -- sets default values
  ~App(); // Destructor  -- cleans up Vulkan, SDL3

  bool Initialize(); // One-time setup  (window, Vulkan, resources)
  void Run();        // The famous "game loop"
  void Shutdown();   // Graceful cleanup
private:
  void ProcessEvents();  // SDL3: keyboard, mouse, window events
  void Update(float dt); // Simulation + animation (called every frame)
  void Render();         // Draw 3D engine + UI panels
  // void DrawBarytonDiagrams();  // T-s and P-v animated plots

  //-------------------Window & Context --------------
  SDL_Window *m_window{nullptr};
  bool m_running{true};
  bool m_engineOn{false};
  float m_throttle{0.0F}; // 0.0 idle, 1.0 = max thrust

  //----------Subsystems (owned by App, created in Initialize)
  std::unique_ptr<Renderer> m_renderer;
  std::unique_ptr<EngineModel> m_engineModel;
  std::unique_ptr<EngineSimulation> m_simulation;
  std::unique_ptr<Camera> m_camera;
  std::unique_ptr<UIManager> m_ui;
  std::unique_ptr<SceneManager> m_scene;
  std::unique_ptr<ParticleSystem> m_particales;

  //-------Timing -----
  Uint64 m_lastTime{};
  float m_deltaTime = 0.0F;

  //--------View States----------
  bool m_explodedView = false;
  bool m_sectionView = false;
  bool m_showAirflow = true;
  int m_currentChapter = 0;
};
