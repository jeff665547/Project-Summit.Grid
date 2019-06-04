screw_extend_template()
if("${__screw_target}" STREQUAL "summit-config-path")
    target_link_libraries(${__screw_target} PUBLIC
        summit-utils
    )
else()
    target_link_libraries(${__screw_target} PUBLIC
        summit-config-path
    )
endif()