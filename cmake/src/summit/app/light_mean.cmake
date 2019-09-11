screw_path_to_target_name(__screw_rel_src_file __screw_target)
screw_add_executable(${__screw_target} ${__screw_src_file})
target_include_directories(${__screw_target} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
screw_add_launch_task(${__screw_target})
target_link_libraries(${__screw_target}
    Nucleona::Nucleona
    Boost::filesystem
    Boost::program_options
    ChipImgProc::ChipImgProc-utils
    fmt::fmt
    summit-utils
    summit-config-chip
)
if(MINGW)
    target_compile_options(${__screw_target} PUBLIC -Wa,-mbig-obj)
endif()
target_compile_definitions(${__screw_target} PRIVATE NUCLEONA_RANGE_USE_V3)
screw_show_var(__screw_target)