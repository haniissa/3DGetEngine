// #include "App.h"
#include "UIManager.h"
#include "SceneManager.h"
#include "EngineSimulation.h"
#include "Renderer.h"
#include "BraytonCycle.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include <fmt/format.h>
#include <cmath>
#include <vulkan/vulkan_core.h>

UIManager::UIManager() = default;
UIManager::~UIManager() = default;

bool UIManager::Initialize(SDL_Window* window, Renderer* renderer){
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	//Dark theme for professional look
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForVulkan(window);

	//ImGui Vulkan init requires render pass and other ditails from renderer
	//This is  simplified - full init would query renderere state
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.ApiVersion = VK_API_VERSION_1_3;
	initInfo.Instance = renderer->GetInstance(); // Would get from renderer
	initInfo.PhysicalDevice =  renderer->GetPhysicalDevice();
	initInfo.Device = renderer->GetDevice();
	initInfo.QueueFamily = renderer->GetGraphicsQueueFamily();
	initInfo.Queue = renderer->GetGraphicsQueue();
	initInfo.PipelineCache = VK_NULL_HANDLE;

	initInfo.DescriptorPool = VK_NULL_HANDLE;
	initInfo.DescriptorPoolSize = 64 ;
	initInfo.UseDynamicRendering = false;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = std::max(2u, renderer->GetSwapchainImageCount());

	//Main viewport pipeline confi g(ImgGui 1.90+)
	initInfo.PipelineInfoMain.RenderPass  = renderer->GetRenderPass();
	initInfo.PipelineInfoMain.Subpass = 0;
	initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.Allocator = nullptr;

	ImGui_ImplVulkan_Init(&initInfo);

	return true;
}
void UIManager::Shutdown(){
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void UIManager::BeginFrame(){
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void UIManager::EndFrame(){
	ImGui::Render();
}

void UIManager::DrawPanels(EngineSimulation* sim, SceneManager* scene){
	//-------Main Menu Bar--------
	if(ImGui::BeginMainMenuBar()){
		if(ImGui::BeginMenu("Panels")){
			if(ImGui::MenuItem("Controls"), nullptr, m_activePanel == PanelType::Controls)
				SetPanel(PanelType::Controls);
			if(ImGui::MenuItem("Cycle Diagram", nullptr, m_activePanel == PanelType::CycleDiagram))
				SetPanel(PanelType::CycleDiagram);
			if(ImGui::MenuItem("Stations", nullptr, m_activePanel ==  PanelType::Stations))
				SetPanel(PanelType::Stations);
			if(ImGui::MenuItem("Chapters", nullptr, m_activePanel == PanelType::Chapters))
				SetPanel(PanelType::Chapters);
			if(ImGui::MenuItem("Faults", nullptr, m_activePanel == PanelType::Faults))
				SetPanel(PanelType::Faults);
			ImGui::End();
		}
		ImGui::EndMainMenuBar();
	}
	//Draw Active Panel
	switch (m_activePanel) {
		case PanelType::Controls:
			DrawControlsPanel(sim, scene);
			break;
		case PanelType::CycleDiagram:
			DrawCycleDiagramPanel(sim);
			break;
		case PanelType::Stations:
			DrawStationPanel(sim);
			break;
		case PanelType::Chapters:
			DrawChaptersPanel(sim);
			break;
		case PanelType::Faults:
			DrawFaultsPanel(sim);
			break;
		default:
			break;
	}
	//----Overaly: Quick State---------
	ImGui::SetNextWindowPos(ImVec2(10, 40), ImGuiCond_Always);
	ImGui::Begin("Engine Stats", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

	ImGui::Text("RPM: %.0F", sim->GetRPM());
	ImGui::Text("RPM: %.1 kN", sim->GetThrust());
	ImGui::Text("RPM: %.1F%%", sim->GetEfficiency());
	ImGui::Text("Throttle: %.0F%%", sim->GetThrottle());

	if(sim->HasFault())
		ImGui::TextColored(ImVec4(1.0F, 0.2F, 0.2F, 1.0F), "FAULT: %s", sim->GetFaultName());
	ImGui::End();
}
void UIManager::DrawControlsPanel(EngineSimulation* sim, SceneManager* scene){
	ImGui::Begin("Contros", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	//Engine Start/stop
	if(sim->IsRunning()){
		if(ImGui::Button("STOP Engine", ImVec2(150, 40))){
			sim->SetRunning(false);
		}
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.2F, 1.0F, 0.2F, 1.0F), "ENGINE RUNNING");
	}else{
		if(ImGui::Button("START ENGINE", ImVec2(150, 40))){
			sim->SetRunning(true);
		}
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0F, 0.5F, 0.2F, 1.0F), "Engine Stopped");
	}
	ImGui::Separator();

	//Throttle slider
	float throttle = sim->GetThrottle() * 100.0F;
	if(ImGui::SliderFloat("Throtle", &throttle, 0.0F, 1.0F, "%.0F%%")){
		sim->SetThrottle(throttle / 100.0F);
	}
	ImGui::Separator();

	//View Toggles
	bool exploded = scene->IsExploded();
	if(ImGui::Checkbox("Exploded View", &exploded)){
		scene->SetExploded(exploded);
	}
	bool section = scene->IsSection();
	if(ImGui::Checkbox("Section View", &section)){
		scene->SetSection(section);
	}
	bool airflow = scene->isShowingAirflow();
	if(ImGui::Checkbox("Show Airflow", &airflow)){
		scene->SetShowAirflow(airflow);
	}
	ImGui::Separator();

	//Camera
	ImGui::Text("Camera Controls:");
	ImGui::BulletText("Left Mouse + Drag: orbit");
	ImGui::BulletText("Scroll: Zoom");
	ImGui::BulletText("Right Mouse + Drag: Pan");

	ImGui::End();

}

void UIManager::DrawCycleDiagramPanel(EngineSimulation* sim){
	ImGui::Begin("Brayton Cycle Diagrams", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	//Generate cycle points based on current engine state
	float pressureRatio = 0.0F + 15.0F * sim->GetThrottle();
	float T_max = 1200.0F + 800.0F * sim->GetThrottle();
	auto cycle = BraytonCycle::GenerateIdealCycle(pressureRatio,  T_max);

	ImGui::Text("Tempreature-Entropy (T-s) Diagram");
	ImVec2 diagramSize(400, 300);
	DrawTSDiagram(sim, diagramSize);

	ImGui::Separator();

	ImGui::Text("Pressure-Volume (P-v) Diagram");
	DrawPVDiagram(sim, diagramSize);

	ImGui::Separator();

	//Show formulas
	if(m_showFormulas){
		ImGui::Text("Key Formulas:");
		for(int i{}; i < 5; ++i){
			ImGui::BulletText("%s: %s", BraytonCycle::GetProcessname(i), BraytonCycle::GetFormula(i));
		}
	}
	ImGui::End();
}

void UIManager::DrawTSDiagram(EngineSimulation* sim, ImVec2 size){
	ImGui::BeginChild("TS_Diagram", size, true);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 origin = ImGui::GetCursorScreenPos();

	//Draw axes
	ImU32 axisColor = IM_COL32(200, 200,200, 255);
	dl->AddLine(origin, ImVec2(origin.x, origin.y + size.y), axisColor, 2.0F); // y axis(t)
	dl->AddLine(origin, ImVec2(origin.x + size.x, origin.y + size.y), axisColor, 2.0F);

	//labels
	dl->AddText(ImVec2(origin.x + 5, origin.y - 15), axisColor, "T (K)");
	dl->AddText(ImVec2(origin.x + size.x - 30, origin.y + size.y + 5), axisColor, "s");

	//Get cycle points
	float pressureRatio = 5.0F + 15.0F * sim->GetThrottle();
	float T_max = 1200.0F + 800.0F * sim->GetThrottle();
	auto points = BraytonCycle::GenerateIdealCycle(pressureRatio, T_max);

	if(points.size() >= 2){
		//Find ranges for scaling
		float minS = points[0].ts.x, maxS = points[0].ts.x;
		float minT = points[0].ts.y, maxT = points[0].ts.y;
		for(const auto& p : points){
			minS = std::min(minS, p.ts.x);
			maxS = std::max(maxS, p.ts.x);
			minT = std::min(minT, p.ts.y);
			maxT = std::max(maxT, p.ts.y);
		}
		float margin = 50.0F;
		auto scale = [&](const glm::vec2& pt) -> ImVec2{
			float x = origin.x + margin + (pt.x - minS) / (maxS - minS) * (size.x - 2 * margin);
			float y = origin.y + size.y - margin - (pt.y - minT) / (maxT - minT) * (size.y - 2 * margin);
			return ImVec2(x, y);
		};

		//Draw process  lines
		ImU32 lineColors[] = {
			IM_COL32(100, 200, 255, 255), //Intake - blue
			IM_COL32(255, 255, 100, 255), // Compression - yelloow
			IM_COL32(255, 150, 50, 255), // Combustion - origin
			IM_COL32(255, 100, 100, 255), // Expansion - red
			IM_COL32(200, 100, 255, 255) // Exhaust - purple
		};
		for(size_t i{}; i < points.size(); ++i){
			ImVec2 p1 = scale(points[i].ts);
			ImVec2 p2 = scale(points[(i + 1) % points.size()].ts);
			dl->AddLine(p1, p2, lineColors[i % 5], 3.0F);

			//Draw point
			dl->AddCircleFilled(p1, 6.0F, IM_COL32(255, 255, 255, 255));
			dl->AddCircle(p1, 6.0F, IM_COL32(0,0,0, 255), 12, 2.0F);

			//Label
			dl->AddText(ImVec2(p1.x + 8, p1.y - 8), IM_COL32(255, 255, 255, 255), points[i].label);
		}
		//Draw animated current positon
		glm::vec2 current = sim->GetCurrentPVPoint();
		ImVec2 curPos = scale(current);
		dl->AddCircleFilled(curPos, 8.0F, IM_COL32(255, 255, 255, 255), 12);
		dl->AddCircle(curPos, 8.0F, IM_COL32(255, 255, 255, 255), 12, 2.0F);
	}
	ImGui::EndChild();
}

void UIManager::DrawPVDiagram(EngineSimulation* sim, ImVec2 size){
	ImGui::BeginChild("PV_DIagram", size, true);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 origin = ImGui::GetCursorScreenPos();

	//Axes
	ImU32 axisColor = IM_COL32(200, 200, 200, 255);
	dl->AddLine(origin, ImVec2(origin.x, origin.y + size.y), axisColor, 2.0F);
	dl->AddLine(origin, ImVec2(origin.x + size.x, origin.y +  size.y), axisColor, 2.0F);
	dl->AddText(ImVec2(origin.x + 5, origin.y - 15), axisColor, "P (kPa)");
	dl->AddText(ImVec2(origin.x + size.x - 30, origin.y + size.y + 5), axisColor, "V (m³/kg");

	float pressureRatio = 5.0F + 15.0F * sim->GetThrottle();
	float T_max = 1200.0F + 800.F * sim->GetThrottle();
	auto points = BraytonCycle::GenerateIdealCycle(pressureRatio, T_max);

	if(points.size() >= 2){
		float minV = points[0].pv.x, maxV = points[0].pv.x;
		float minP = points[0].pv.y, maxP = points[0].pv.y;
		for(const auto& p : points){
			minV = std::min(minV, p.pv.x);
			maxV = std::max(maxV, p.pv.x);
			minV = std::min(minV, p.pv.y);
			maxV = std::max(maxV, p.pv.y);
		}
		float margin = 50.0F;

		auto scale = [&](const glm::vec2& pt) -> ImVec2{
			float x = origin.x + margin + (pt.x - minV) / (maxV - minV) * (size.x - 2 * margin);
			float y = origin.y + size.y - margin - (pt.y - minP) / (maxP - minP) * (size.y - 2 * margin);
			return ImVec2(x, y);
		};
		ImU32 lineColors[] = {
			IM_COL32(100, 200, 255, 255),
            IM_COL32(255, 255, 100, 255),
            IM_COL32(255, 150, 50, 255),
            IM_COL32(255, 100, 100, 255),
            IM_COL32(200, 100, 255, 255)
		};
		for(size_t i{}; i < points.size(); ++i){
			ImVec2 p1 = scale(points[i].pv);
			ImVec2 p2 = scale(points[(i + 1)% points.size()].pv);
			dl->AddLine(p1, p2, lineColors[i % 5], 3.0F);
			dl->AddCircleFilled(p1, 6.0F, IM_COL32(255, 255, 255, 255));
			dl->AddCircle(p1, 6.0F, IM_COL32(0, 0,0, 255), 12, 2.0F);
		}
		glm::vec2 current = sim->GetCurrentPVPoint();
		ImVec2 curPos = scale(current);
		dl->AddCircleFilled(curPos, 8.0F, IM_COL32(0, 255, 0, 255));
	}
	ImGui::EndChild();
}

void UIManager::DrawStationPanel(EngineSimulation* sim){
	ImGui::Begin("station Data", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	const char* stationNames[] = {
		"0: Freestream", "1: Intake Exit", "2: Compressor Exit",
		"3: Combustor Exit", "4: Tubine Exit", "5: Nozzle Exit"
	};

	ImGui::Text("Live Thermodynamic Data");
	ImGui::Separator();

	if(ImGui::BeginTable("Stations", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)){
		ImGui::TableSetupColumn("stations");
		ImGui::TableSetupColumn("P (Kpa)");
		ImGui::TableSetupColumn("T (k)");
		ImGui::TableSetupColumn("v (m/s)");
		ImGui::TableSetupColumn("p (kg/m³");
		ImGui::TableHeadersRow();

		for(int i{}; i < 6; ++i){
			const auto& s = sim->GetStation(i);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", stationNames[i]);
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%1f", s.pressure);
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.1f", s.temperature);
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.1f", s.velocity);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.3f", s.density);
		}
		ImGui::EndTable();
	}
	ImGui::Separator();
	ImGui::Text("Performance Metrics");
	ImGui::Text("Compressor Work (Wc) %.1f kJ/kg", sim->GetCompressorWork() / 1000);
	ImGui::Text("Turbine Work (Wt)    %.1f kJ/kg", sim->GetTurbineWork() / 1000);
	ImGui::Text("Heat Input (Qin)     %.1f kJ/kg", sim->GetHeatInput() / 1000);
	ImGui::Text("Thermal Efficiency:  %.1f kJ/kg", sim->GetHeatInput() / 1000);
	ImGui::Text("TSFC                 %.1f g/(kN-s)", sim->GetEfficiency() / 1000);

	ImGui::End();
}

void UIManager::DrawChaptersPanel(EngineSimulation* sim){
	ImGui::Begin("Guided Learning", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Chapter %d: %s", m_currentChapter + 1, m_chapterNames[m_currentChapter]);
	ImGui::Separator();

	const char* explanations[5] = {
		"Air enters the intake at high velocity. The diffuser slows it down\n"
		"isentropically, converting kietic energy into pressure.\n\n"
		"Key concept: Total pressure = Static pressure + Dynamic pressure",

		"The compressor does work on the air, raising its pressureand\n"
		"temperature  isentropically. This requires shaft power from the tubine.\n\n"
		"Key formula: T2/T1 = (P2/P1)^((y-1)/y)",

		"Fuel is injected and burned at constatn pressure. Temperature\n"
		"rises dramatically while pressure stays nearly constant.\n\n"
		"key formula: Qin = cp x (T3 - T2)",

		"Hot gas expands through the tubine, extracting work to drive\n"
		"the compressor. Pressure and temperature drop.\n\n"
		"key formula: Wt = cp x (T3 - T4)",

		"Remaining energy accelerates exhaust gas through the nozzle.\n"
		"High exit velocity produces thrust via Newton's 3rd Law\n\n"
		"Key formula: F= m_dot x (v_exit- v_inlet"
	};
	ImGui::TextWrapped("%s", explanations[m_currentChapter]);

	ImGui::Separator();

	if(ImGui::Button("Previous Chapter")){
		m_currentChapter = (m_currentChapter + 4) % 5;
	}
	ImGui::SameLine();
	if(ImGui::Button("Next Chapter"))
		m_currentChapter = (m_currentChapter + 1) % 5;

	//Highlight active stage
	int activeStage = static_cast<int>(sim->GetActiveStage());
	ImGui::Text("Currently viewing: %s", BraytonCycle::GetProcessname(activeStage));

	ImGui::End();
}

void UIManager::DrawFaultsPanel(EngineSimulation* sim){
	ImGui::Begin("Fault Challenegs", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Test your diagnostic skills!");
	ImGui::Separator();

	if(ImGui::Button("Inject Compressor Stall")){
		sim->InjectFault("Compressor Stall");
	}
	ImGui::SameLine();
	if(ImGui::Button("Inject Tubine Overheat")){
		sim->InjectFault("Tubine Overheat");
	}
	ImGui::SameLine();
	if(ImGui::Button("Inject Fuel Leak")){
		sim->InjectFault("Fuel Leak");
	}
	ImGui::Separator();
	if(sim->HasFault()){
		ImGui::TextColored(ImVec4(1.0F, 0.3F, 0.3F, 1.0F),
			"ACTIVE FAULT: %s", sim->GetFaultName());
		ImGui::Text("Observe the symptons in the Station Data panel.");
		ImGui::Text("Can you identify what's wrong?");

		if(ImGui::Button("Clear Fault")){
			sim->ClearFault();
		}
	}else{
		ImGui::TextColored(ImVec4(0.3F, 1.0F, 0.3F, 1.0F), "system Nominal");
	}
	ImGui::Separator();
	ImGui::Text("Common Symptoms Guide: ");
	ImGui::BulletText("Compressor Stall: Hight T2, low pressure ratio, vibration");
	ImGui::BulletText("Tubine Overheat: T4 exceeds limits, efficiency drops");
	ImGui::BulletText("Fuel Leak: Qin drops, thrust decreases, T3 unstable");

	ImGui::End();
}
void UIManager::SetPanel(PanelType type){
	m_activePanel = type;
}

bool UIManager::IsPanelOpen(PanelType type) const {
	return m_activePanel == type;
}
