screw_path_to_target_name(__screw_rel_src_file __screw_target)
screw_add_library(${__screw_target} ${__screw_src_file})
set(include_target 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
list(APPEND include_target 
    $<BUILD_INTERFACE:${HUNTER_INSTALL_PREFIX}/include/opencv4>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(${__screw_target} PUBLIC ${include_target})
target_link_libraries(${__screw_target} PUBLIC
    Boost::filesystem
    Nucleona::Nucleona
)
screw_show_var(__screw_target)