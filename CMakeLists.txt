cmake_minimum_required(VERSION 3.5)
project(DX12Pactice LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(_Install_path ${PROJECT_SOURCE_DIR}/bin)
if(NOT EXISTS "${_Install_path}")
    execute_process(
        COMMAND New-Item ${_Install_path} -type directory
    )
endif()

include_directories(DX12Pactice PUBLIC ${PROJECT_SOURCE_DIR}/DirectxMath/Inc)

add_subdirectory(c1)


