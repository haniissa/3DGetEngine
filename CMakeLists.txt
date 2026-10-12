cmake_minimum_required(VERSION 3.25)
set(PROJECT_NAME "3DJetEngineLab" CACHE STRING "project name")
project(${PROJECT_NAME}  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Beginner-friendly: Fetch all dependencies automatically ---
include(FetchContent)

#Find libraries
find_package(fmt CONFIG REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(sdl3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(imgui QUIET)



function(setup_lib LIB_NAME REPO_PATH GIT_TAG_VERSION)
  message(STATUS "Checking for  ${LIB_NAME}...")

  if(${LIB_NAME}_FOUND)
    list(APPEND ${LIB_NAME}_libraries ${LIB_NAME})
    list(APPEND ${LIB_NAME}_include_dirs "${CMAKE_PREFIX_PATH}/include")
    set(${LIB_NAME}_libraries ${LIB_NAME} PARENT_SCOPE)
    set(${LIB_NAME}_include_dirs "${CMAKE_PREFIX_PATH}/include" PARENT_SCOPE)
  else()
    message(STATUS "${LIB_NAME} not found locally. Installing via FetchContent")

    #Prevent the library from building its own examples/tests
    set(${LIB_NAME}_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(${LIB_NAME}_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
      ${LIB_NAME}
      GIT_REPOSITORY https://github.com/${REPO_PATH}.git
      GIT_TAG   ${GIT_TAG_VERSION}
    )
    FetchContent_MakeAvailable(${LIB_NAME})

    # set(${LIB_NAME}_SOURCE_DIR ${${LIB_NAME}_SOURCE_DIR} PARENT_SCOPE) # <-- ADD THIS
    set(${LIB_NAME}_libraries ${LIB_NAME} PARENT_SCOPE)
    set(${LIB_NAME}_include_dirs "${${LIB_NAME}_SOURCE_DIR}/include" PARENT_SCOPE)
  endif()
endfunction()
setup_lib("fmt" "fmtlib/fmt" "10.1.1")
setup_lib("sdl3" "libsdl-org/SDL" "prerelease-3.0.0")
setup_lib("vk-bootstrap" "charles-lunarg/vk-bootstrap" "v1.3.295")
setup_lib("glm" "g-truc/glm" "1.0.1")
# setup_lib("imgui" "ocornut/imgui" "v1.90.4")

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG   master # or docking
  )
  FetchContent_MakeAvailable(imgui)
  FetchContent_GetProperties(imgui)
# ImGui is not Cmake-native, so  we set it up manually
set(IMGUI_DIR "${imgui_SOURCE_DIR}")
add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3 Vulkan::Vulkan)


#-----Main Executable ----

 add_executable(${PROJECT_NAME}
    src/main.cpp
    src/App.cpp
    src/Renderer.cpp
    src/EngineModel.cpp
    src/EngineSimulation.cpp
    src/Camera.cpp
    src/UIManager.cpp
    src/BraytonCycle.cpp
    src/SceneManager.cpp
    src/ParticleSystem.cpp
)


target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  SDL3::SDL3
  Vulkan::Vulkan
  vk-bootstrap::vk-bootstrap
  fmt::fmt
  imgui
  glm::glm
)

# --- Shader Compilation ----
# Automatically compile .vert / .frag to SPIR-V at build time
find_program(GLSLC_EXECUTABLE
      glslangValidator HINTS $ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/bin
)
if(NOT GLSLC_EXCUTABLE)
  find_program(GLSLC_EXCUTABLE glslc
    HINTS ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/bin
  )
endif()

if(GLSLC_EXECUTABLE)
  file(GLOB SHADER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag")

  foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(FILENAME  ${SHADER} NAME)
    set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILENAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${CMAKE_COMMAND} -E make_directory
              "${CMKAE_CURRENT_BINARY_DIR}/shaders"
      COMMAND ${GLSLC_EXECUTABLE} -V ${SHADER} -o ${SPIRV}
      DEPENDS ${SHADER}
      COMMENT "Compiling shader ${FILENAME}"
    )
    list(APPEND SPIRV_BINARIES ${SPIRV})
  endforeach()
  add_custom_target(Shaders ALL DEPENDS ${SPIRV_BINARIES})
  add_dependencies(${PROJECT_NAME} Shaders)
else()
  message(WARNING "glslangValidator/glslc not found. Shaders must be compiled manually.")
endif()

#------Install /Package ---
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders DESTINATION bin)
