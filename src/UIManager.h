#pragma once
/**
    @file UIManager.h
    @brief All UI panels using Dear ImGui
    Panels (mutually exclusive, collapsible):
        - Controls:   Start/Stop Throttle, View toggles
        - Cycle Diagram: Animated T-s and P-v plots
        - Stations:   Live P, T, v data per station
        - Chapters: Guided lerning tours
        - Faults: Challenege mode
    */



// #include "App.h"
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <vector>
#include <string>

class EngineSimulation;
class SceneManager;
class Renderer;

enum class PanelType{
    None,
    Controls,
    CycleDiagram,
    Stations,
    Chapters,
    Faults
};

class UIManager{
    private:
        void DrawControlsPanel(EngineSimulation* sim, SceneManager* scene);
        void DrawCycleDiagramPanel(EngineSimulation* sim);
        void DrawStationPanel(EngineSimulation* sim);
        void DrawChaptersPanel(EngineSimulation* sim);
        void DrawFaultsPanel(EngineSimulation* sim);

        //Diagram drawing helpers
        void DrawTSDiagram(EngineSimulation*, ImVec2 size);
        void DrawPVDiagram(EngineSimulation* sim, ImVec2 size);
        void DrawArrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 color);

        PanelType m_activePanel = PanelType::Controls;
        int m_currentChapter = 0;
        bool m_showFormulas = true;

        //Chapter descriptions
        const char* m_chapterNames[5] = {
            "1. Intake & Compersion",
            "2. Combustion Heat Addition",
            "3. Turbine Work Extraction",
            "4. Mozzle Expansion & Thrust",
            "5. Full Cycle Efficiency"
        };

    public:
        UIManager();
        ~UIManager();

        bool Initialize(SDL_Window* window, Renderer* renderer);
        void Shutdown();

        void BeginFrame();
        void EndFrame();

        //Draw all a ctive panels
        void DrawPanels(EngineSimulation* aim, SceneManager* scene);

        //Panel state queries (for App Logic)
        bool IsPanelOpen(PanelType type) const;
        void TogglePanel(PanelType type);
        void SetPanel(PanelType type); // Mutually exlusive

        //Chapter system
        void NextChapter(){m_currentChapter = (m_currentChapter + 1) % 5;}
        int GetCurrentChapter() const {return m_currentChapter;}
};
