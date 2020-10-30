add_executable(C4_InitializeApp InitialzeApp.cpp)

target_sources(C4_InitializeApp PRIVATE ${common_sources})
target_link_libraries(C4_InitializeApp PRIVATE ${d3d12_libs})

install(TARGETS C4_InitializeApp DESTINATION ${_Install_path})