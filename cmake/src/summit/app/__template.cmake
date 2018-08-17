screw_path_to_target_name(__screw_rel_src_file __screw_target)
screw_add_executable(${__screw_target} ${__screw_src_file})
target_include_directories(${__screw_target} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
screw_add_launch_task(${__screw_target})
target_link_libraries(${__screw_target}
    ChipImgProc::ChipImgProc-hough_transform
    Nucleona::Nucleona
    Boost::filesystem
    ChipImgProc::ChipImgProc-utils
    Boost::program_options
    summit-utils
    summit-config-path
    summit-config-cell_fov
    summit-config-chip
    ChipImgProc::ChipImgProc-stitch
)
screw_show_var(__screw_target)