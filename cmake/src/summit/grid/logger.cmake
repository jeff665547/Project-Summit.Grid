screw_extend_template()
if(ENABLE_LOG)
    target_compile_definitions(${__screw_target} PUBLIC 
        SUMMITGRID_ENABLE_LOG
    )
    target_link_libraries(${__screw_target} PUBLIC 
        spdlog::spdlog
    )
endif()