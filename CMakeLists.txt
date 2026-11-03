cmake_minimum_required(VERSION 3.21)
project(EarthworksPlanning VERSION 0.13.6 LANGUAGES CXX)
option(EARTHWORKS_TRACE_FAULT "Enable temporary native fault tracing during investigation" OFF)
option(EARTHWORKS_DEVELOPER_UI "Include technical inspector and terrain-probe UI" OFF)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Use the Microsoft x64 C++ toolchain on Windows.")
endif()

set(TESMIO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/TesmioLoader" CACHE PATH "Pinned TesmioLoader checkout")
if(NOT EXISTS "${TESMIO_SDK_DIR}/src/tesmio_api.h")
    message(FATAL_ERROR "SDK missing. Run scripts/Fetch-Dependencies.ps1 first.")
endif()

file(SHA256 "${TESMIO_SDK_DIR}/src/tesmio_api.h" sdk_header_hash)
if(NOT sdk_header_hash STREQUAL "5daaf51ec1f6a5f279868bb039f01719931dfba0be69312a75863073ceac04a6")
    message(FATAL_ERROR "SDK header differs from the pinned API 4 header. Review dependencies.lock.json.")
endif()

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/RepublicMP/third_party/imgui")
if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(FATAL_ERROR "Pinned RepublicMP/ImGui checkout missing. Run scripts/Fetch-Dependencies.ps1.")
endif()
add_library(earthworks_imgui STATIC
    "${IMGUI_DIR}/imgui.cpp" "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp" "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp" "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp")
target_include_directories(earthworks_imgui SYSTEM PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
target_compile_definitions(earthworks_imgui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
set_property(TARGET earthworks_imgui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_library(EarthworksPlanning SHARED src/plugin/plugin.cpp src/core/planning.cpp src/core/productivity.cpp src/core/state_store.cpp src/runtime/runtime.cpp src/runtime/save_store.cpp src/runtime/terrain_probe.cpp src/runtime/native_terrain_probe.cpp src/runtime/workforce_probe.cpp src/runtime/vehicle_filter.cpp src/ui/overlay.cpp)
target_link_libraries(EarthworksPlanning PRIVATE earthworks_imgui d3d11 dxgi d3dcompiler dwmapi bcrypt windowscodecs ole32)
target_include_directories(EarthworksPlanning SYSTEM PRIVATE "${TESMIO_SDK_DIR}/src")
target_compile_features(EarthworksPlanning PRIVATE cxx_std_17)
target_compile_definitions(EarthworksPlanning PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX
    EARTHWORKS_GAME_PE_TIMESTAMP=0x6A3EB6ADu
)
if(EARTHWORKS_DEVELOPER_UI)
    target_compile_definitions(EarthworksPlanning PRIVATE EARTHWORKS_DEVELOPER_UI=1)
endif()
if(EARTHWORKS_TRACE_FAULT)
    target_compile_definitions(EarthworksPlanning PRIVATE EARTHWORKS_TRACE_FAULT)
endif()
target_compile_options(EarthworksPlanning PRIVATE /W4 /WX /permissive- /utf-8)
target_link_options(EarthworksPlanning PRIVATE "/MAP:${CMAKE_BINARY_DIR}/bin/EarthworksPlanning.map")
set_property(TARGET EarthworksPlanning PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_target_properties(EarthworksPlanning PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

enable_testing()
add_executable(planning_checks tests/planning_checks.cpp src/core/planning.cpp)
target_compile_features(planning_checks PRIVATE cxx_std_17)
target_compile_options(planning_checks PRIVATE /W4 /WX /utf-8)
set_property(TARGET planning_checks PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_test(NAME geometry_and_draft_history COMMAND planning_checks)
add_executable(terrain_save_checks tests/terrain_save_checks.cpp)
target_compile_features(terrain_save_checks PRIVATE cxx_std_17)
target_compile_options(terrain_save_checks PRIVATE /W4 /WX /utf-8)
set_property(TARGET terrain_save_checks PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_executable(workforce_checks tests/workforce_checks.cpp src/runtime/workforce_probe.cpp src/runtime/vehicle_filter.cpp)
target_include_directories(workforce_checks SYSTEM PRIVATE "${TESMIO_SDK_DIR}/src")
target_compile_features(workforce_checks PRIVATE cxx_std_17)
target_compile_options(workforce_checks PRIVATE /W4 /WX /utf-8)
target_compile_definitions(workforce_checks PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
set_property(TARGET workforce_checks PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_test(NAME synthetic_workforce_readings COMMAND workforce_checks)
