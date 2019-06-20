screw_extend_template()
if(NOT "${__screw_target}" STREQUAL "summit-config-path")
    target_link_libraries(${__screw_target} PUBLIC
        summit-config-path
    )
endif()