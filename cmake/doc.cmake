find_package(Doxygen)
if(DOXYGEN_FOUND AND BUILD_DOC)
    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} doc.cfg
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    if(WIN32)
        set(MAKE_PDF "make.bat")
    else()
        set(MAKE_PDF "make")
    endif()

    add_custom_command(TARGET doc_doxygen POST_BUILD 
        COMMAND ${MAKE_PDF} 
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc/output/latex
    )
    install( 
        DIRECTORY doc/output/html 
        DESTINATION doc/
        COMPONENT Document 
    )
    # install( 
    #     FILES doc/output/latex/refman.pdf
    #     DESTINATION doc/
    #     COMPONENT Document 
    # )
else()
endif()