screw_path_to_target_name(__screw_rel_src_file __screw_target)
screw_add_executable(${__screw_target} ${__screw_src_file})
target_link_libraries(
    ${__screw_target}
    GTest::gtest 
    GTest::main
    ${OpenCV_LIBS}
    Boost::system
)
target_include_directories(${__screw_target} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)