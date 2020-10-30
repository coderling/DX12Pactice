add_executable(C5_BOX3D Box3D.cpp)

target_sources(C5_BOX3D PRIVATE ${common_sources})
target_link_libraries(C5_BOX3D PRIVATE ${d3d12_libs})

install(TARGETS C5_BOX3D DESTINATION ${_Install_path})