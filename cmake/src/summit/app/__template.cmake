screw_path_to_target_name(__screw_rel_src_file __screw_target)
screw_add_executable(${__screw_target} ${__screw_src_file})
target_include_directories(${__screw_target} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
screw_add_launch_task(${__screw_target})
target_link_libraries(${__screw_target}
    summit-utils
    summit-config-path
    summit-config-cell_fov
    summit-config-chip
    summit-config-aruco_db
    summit-format
    summit-grid-logger
    ChipImgProc::ChipImgProc
    SummitCrypto::summit-crypto-scan_image
    Nucleona::Nucleona
    Boost::filesystem
    Boost::program_options

)
if(PROFILER)
    target_compile_definitions(${__screw_target} PUBLIC PROFILER)
    target_link_libraries(${__screw_target} profiler)
    if(GNU)
        target_link_options(${__screw_target} PRIVATE -Wl,--whole-archive)
    endif()
endif()
if(MINGW)
    target_compile_options(${__screw_target} PUBLIC -Wa,-mbig-obj)
endif()
if(MSVC)
  target_compile_options(${__screw_target} PUBLIC /bigobj)
endif()
target_compile_definitions(${__screw_target} PRIVATE NUCLEONA_RANGE_USE_V3)
screw_show_var(__screw_target)
