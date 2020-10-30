cmake_minimum_required(VERSION 3.5)
project(DX12Pactice LANGUAGES CXX)

add_definitions(-DUNICODE -D_UNICODE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(_Install_path ${PROJECT_SOURCE_DIR}/bin)
if(NOT EXISTS "${_Install_path}")
    execute_process(
        COMMAND New-Item ${_Install_path} -type directory
    )
endif()

include_directories(DX12Pactice PUBLIC ${PROJECT_SOURCE_DIR}/DirectxMath/Inc)

set(common_sources
${CMAKE_CURRENT_SOURCE_DIR}/Common/D3DApp.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Common/Util.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Common/GameTimer.cpp)

set(d3d12_libs
"d3d12.lib"
"dxgi.lib"
"d3dcompiler.lib")


add_subdirectory(c1)
add_subdirectory(c4)

