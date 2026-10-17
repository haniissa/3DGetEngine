#include "App.h"
#include "Camera.h"
#include "EngineModel.h"
#include "EngineSimulation.h"
#include "ParticleSystem.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "UIManager.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <cstdio>
#include <fmt/base.h>
#include <fmt/format.h>
#include <imgui.h>
#include <vector>

App::App() = default;
App::~App() = default;

bool App::Initialize() {
  fmt::print("[App] Initializing\n");

  //-----1. Initialize SDL3 ------
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    fmt::print(stderr, "[App] SDL_Init failed: {}\n", SDL_GetError());
    return false;
  }

  // Create window with Vulkan flag SDL3 prepares a Vulkan-compatible surface
  m_window = SDL_CreateWindow(
      "3D Jet Engine Lab -- Turbojet Simulation", 1280, 720,
      SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (!m_window) {
    fmt::println(stderr, "[App Window  creation failed: {}\n", SDL_GetError());
    return false;
  }

  //--------2.Create subsystems -------------
  // We use std::make_unique -- the modern C++ way to create owned pointers.
  // // This means App OWNS these objects and deletes them automatically.
  //
  m_renderer = std::make_unique<Renderer>();
  if (!m_renderer->Initialize(m_window, 1280, 720)) {
    fmt::print(stderr, "[App] Renderer initialization failed.\n");
    return false;
  }

  m_camera = std::make_unique<Camera>(1280.0F / 720.0F);

  m_engineModel = std::make_unique<EngineModel>();
  m_engineModel->GenerateGeometry();

  // Upload the generated mesh to GPU
  m_renderer->UploadMesh(m_engineModel->GetVertices(),
                         m_engineModel->GetIndices());
  m_simulation = std::make_unique<EngineSimulation>();
  m_scene = std::make_unique<SceneManager>();
  m_particales = std::make_unique<ParticleSystem>();

  m_ui = std::make_unique<UIManager>();
  if (!m_ui->Initialize(m_window, m_renderer.get())) {
    fmt::print(stderr, "[App] UI initialization failed.\n");
    return false;
  }

  m_lastTime = SDL_GetTicks();
  fmt::print("[App] Initialization complete.\n");
  return true;
}

void App::Run() {
  fmt::print("[App] Entering main loop...\n");

  while (m_running) {
    //----------Calculate Delta Time----
    Uint64 now = SDL_GetTicks();
    m_deltaTime = static_cast<float>(now - m_lastTime) / 1000.0F;
    m_lastTime = now;
    // Clamp dt to avoid physics explosions on lag spikes
    if (m_deltaTime > 0.1F)
      m_deltaTime = 0.1F;

    ProcessEvents();
    Update(m_deltaTime);
    Render();
  }
}

void App::ProcessEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // let ImGui process evennts first (so UI gets priority)
    ImGui_ImplSDL3_ProcessEvent(&event);

    switch (event.type) {
    case SDL_EVENT_QUIT:
      m_running = false;
      break;
    case SDL_EVENT_WINDOW_RESIZED:{
    	int pw = 0;
     	int ph = 0;
      SDL_GetWindowSizeInPixels(m_window, &pw, &ph);
      m_camera->OnResize(static_cast<float>(pw), static_cast<float>(ph));
      m_renderer->RecreateSwapchain(static_cast<uint32_t>(pw), static_cast<uint32_t>(ph));
      break;
    }

    case SDL_EVENT_KEY_DOWN:
      switch (event.key.key) {
      case SDLK_ESCAPE:
        m_running = false;
        break;
      case SDLK_SPACE:
        m_simulation->ToggleEngine();
        m_engineOn = m_simulation->IsRunning();
        fmt::print("[App] Engien {}\n", m_engineOn ? "STARTED" : "STOPPED");
        break;
      case SDLK_UP:
        m_throttle = std::min(1.0F, m_throttle + 0.1F);
        m_simulation->SetThrottle(m_throttle);
        break;
      case SDLK_DOWN:
        m_throttle = std::max(0.0F, m_throttle - 0.1F);
        m_simulation->SetThrottle(m_throttle);
        break;
      case SDLK_E:
        m_explodedView = !m_explodedView;
        m_scene->SetExploded(m_explodedView);
        m_engineModel->SetExploadedView(m_explodedView,
                                       m_scene->GetExplodedFactor());

        fmt::print("[App] Exploded view: {}\n", m_explodedView ? "ON" : "OFF");
        break;
      case SDLK_S:
        m_sectionView = !m_sectionView;
        m_scene->SetSection(m_sectionView);
        fmt::print("[App] Section view: {}\n", m_sectionView ? "ON" : "OFF");
        break;
      case SDLK_A:
        m_showAirflow = !m_showAirflow;
        m_scene->SetShowAirflow(m_showAirflow);
        m_particales->SetEnabled(m_showAirflow);
        break;
      case SDLK_C:
        m_ui->NextChapter();
        m_currentChapter = m_ui->GetCurrentChapter();
        fmt::print("[App] Chapter: {}\n", m_currentChapter + 1);
        break;
      case SDLK_F:
        if (!m_simulation->HasFault()) {
          m_simulation->InjectFault("Compressor Stall");
          fmt::print("[App] FAULT INJECTED: Compressor Stall!\n");
        } else {
          m_simulation->ClearFault();
          fmt::print("[App] Fault cleared");
        }
        break;
      case SDLK_R:
        m_camera->SetAutoRotate(!m_camera->GetPosition().x); // toggle via dummy
        break;
      }
      break;
    case SDL_EVENT_MOUSE_MOTION:{
    	if(ImGui::GetIO().WantCaptureMouse) break;
     	const bool left = (event.motion.state & SDL_BUTTON_LMASK) != 0;
     	const bool right = (event.motion.state & SDL_BUTTON_RMASK) != 0;

      	m_camera->OnMouseMove(static_cast<float>(event.motion.xrel),
       static_cast<float>(event.motion.yrel), left, right);
       break;

    }
       case SDL_EVENT_MOUSE_WHEEL:
      m_camera->OnScroll(event.wheel.y);
      break;
    }
  }
}

void App::Update(float dt) {
  //----Update all subsystems-----
  m_simulation->Update(dt);
  m_camera->Update(dt);
  m_scene->Update(dt);
  //SceneManager owns the view toggles; mirror them into App each frame
  m_explodedView = m_scene->IsExploded();
  m_sectionView = m_scene->IsSection();
  m_showAirflow = m_scene->isShowingAirflow();
  m_particales->SetEnabled(m_showAirflow);

  m_particales->Update(dt, m_simulation->GetRPM(), m_simulation->GetThrottle());
}

void App::Render() {
  if(!m_renderer->BeginFrame()) return;

  //----Update Uniforms----
  UniformBufferObject ubo{};
  ubo.model = glm::mat4(1.0F); // Engine stays at origin
  ubo.view = m_camera->GetViewMatrix();
  ubo.proj = m_camera->GetProjectionMatrix();
  ubo.proj[1][1] *= -1.0F;

  ubo.engineState.x = m_simulation->GetRPM() / 15000.0F;
  ubo.engineState.y =
      m_simulation->GetStation(3).temperature / 2000.0F; // Normalized temp
  ubo.engineState.z =
      m_simulation->GetStation(2).pressure / 2000.0F; // Normalized pressure
  ubo.engineState.w = m_simulation->GetThrottle();

  ubo.viewFlags.x = m_scene->GetExplodedFactor();
  ubo.viewFlags.y = m_scene->GetSectionFactor();
  ubo.viewFlags.z = m_showAirflow ? 1.0F : 0.0f;
  ubo.viewFlags.w = static_cast<float>(SDL_GetTicks()) / 1000.0F;

  m_renderer->UpdateUniforms(ubo);

  //-----Build One vertex/index buffer : engine first, then particles----
  std::vector<Vertex> verts = m_engineModel->GetVertices();
  std::vector<uint32_t> idx = m_engineModel->GetIndices();
  const uint32_t engineIndexCount = static_cast<uint32_t>(idx.size());

  uint32_t particleIndexCount = 0;
  if(m_showAirflow and m_simulation->IsRunning()){
  		std::vector<Vertex> pVerts;
  		std::vector<uint32_t> pIdx;
  		m_particales->GenerateVertices(pVerts, pIdx);
    if(!pIdx.empty()){
    	//Particle indices start at 0, so shift them past the enigne vertices
     const uint32_t shift = static_cast<uint32_t>(verts.size());
     verts.insert(verts.end(), pVerts.begin(), pVerts.end());
     for(uint32_t i:pIdx) idx.push_back(i + shift);
     	particleIndexCount = static_cast<uint32_t>(pIdx.size());
    }
  }
  if (engineIndexCount > 0 || particleIndexCount) {
  	m_renderer->UploadMesh(verts, idx);
   //Pass 1 : opaue engine. Pass2: additive particles, drawn after
   m_renderer->DrawMesh(engineIndexCount, 0, false);
   m_renderer->DrawMesh(particleIndexCount, engineIndexCount, true);
  }


  // ///------------------Draw Engine Components -------
  // /// In a full implementation, we'd draw each component with its own transform.
  // /// Here we draw the full mesh once for simplicity.
  // const auto &indices = m_engineModel->GetIndices();
  // if (!indices.empty()) {
  //   m_renderer->DrawMesh(static_cast<uint32_t>(indices.size()));
  // }

  // //------Draw Airflow Particales ----
  // if (m_showAirflow && m_simulation->IsRunning()) {
  //   std::vector<Vertex> verts =  m_engineModel->GetVertices();
  //   std::vector<uint32_t> idx = m_engineModel->GetIndices();
  //   const uint32_t engineIndexCount = static_cast<uint32_t>(idx.size());

  //   if(m_showAirflow && m_simulation->IsRunning()){
  //     std::vector<Vertex> pVerts;
  //     std::vector<uint32_t> pIdx;

  //       m_particales->GenerateVertices(pVerts, pIdx);

  //       const uint32_t shift = static_cast<uint32_t>(verts.size());
  //       verts.insert(verts.end(), pVerts.begin(), pVerts.end() );
  //       for(uint32_t i : pIdx) idx.push_back(i + shift);
  //   }

  //   if(!idx.empty()){
  //     m_renderer->UploadMesh(verts, idx);
  //     m_renderer->DrawMesh(static_cast<uint32_t>(idx.size()));
  //   }


  // }

  // Draw UI Panels--
  m_renderer->BeginUI();
  m_ui->BeginFrame(); // Starts ImGui frame
  m_ui->DrawPanels(m_simulation.get(), m_scene.get());
  m_ui->EndFrame(); // calls Imgui::Rehnder()
  m_renderer->EndUI(); //Submits draw data to Vulkan command buffer

  m_renderer->EndFrame();
}

void App::Shutdown() {
  fmt::print("[App] Shutting down...\n");

  // Order matters: destroy in reverse of creation
  m_ui->Shutdown();
  m_ui.reset();
  m_particales.reset();
  m_scene.reset();
  m_simulation.reset();
  m_engineModel.reset();
  m_camera.reset();
  m_renderer->Shutdown();

  SDL_DestroyWindow(m_window);
  SDL_Quit();

  fmt::print("[App] Shutdown complete.\n");
}
