set( _GLIBCXX_USE_CXX11_ABI 0 )
set( CMAKE_CXX_STANDARD 17 )
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAG} -Wno-deprecated-declarations")
    # list(APPEND cxx_debug_flag -O0)
    # list(APPEND cxx_flag -Wno-deprecated-declarations)
else()
    # set(cxx_debug_flag)
endif()

# screw_set_debugger()

get_filename_component(
    __compiler_bin_dir
    ${CMAKE_CXX_COMPILER}
    DIRECTORY
)
set(SCREW_DEBUGGER ${__compiler_bin_dir}/gdb.exe)
unset(__compiler_bin_dir)

screw_show_var(SCREW_DEBUGGER)