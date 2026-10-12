/**
    @file main.cpp 
        @brief Entry point -- the "front door" of out appliation

        Every c++ program starts at main(). Ours is simple:
            1. Create the App
            2. Initialize everything
            3. Run the Loop
            4. Clean up
 */ 

 #include "App.h"
#include <cstdio>
#include <exception>
#include <fmt/base.h>
 #include <fmt/format.h>

 int main(int argc, char* argv[]){
    fmt::print("======================================");
    fmt::print("       3D Jet Engine Lab v1.0.0\n");
    fmt::print("       Portfolio Project for Upwork\n");
    fmt::print("======================================");
    fmt::print("Controls:\n");
    fmt::print("    [Space]     Start/Stop Engine\n");
    fmt::print("    [Up/Down]   Throttle Contrln\n");
    fmt::print("    [E]         Toggle Exploded View\n");
    fmt::print("    [S]         Toggle Section View\n");
    fmt::print("    [A]         Toggle Airflow Particles\n");
    fmt::print("    [C]         Next Guided Chapter\n");
    fmt::print("    [F]         Inject Fault Challenge\n");
    fmt::print("    [Mouse]     Orbit / Zoom / Pan\n");
    fmt::print("=======================================");

    try {
        App app;
        if(!app.Initialize()){
            fmt::print(stderr, "Failed to initialize application.\n");
            return -1;
        }

        app.Run();
        app.Shutdown();
    }catch (const std::exception& e){
        fmt::print(stderr, "Fatal error: {}\n", e.what());
        return -1;
    }
    fmt::print("\nGoodbye!\n");
    return 0;
 }