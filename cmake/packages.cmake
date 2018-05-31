if( MSVC )
    hunter_config(
        GTest
        VERSION ${HUNTER_GTest_VERSION}
        CMAKE_ARGS 
            CMAKE_CXX_FLAGS=/D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
    )
else()
    hunter_config(
        GTest
        VERSION ${HUNTER_GTest_VERSION}
    )
endif()
hunter_config(Screw GIT_SUBMODULE "cmake/screw")
if( MINGW )
    hunter_config(
        OpenCV
        VERSION ${HUNTER_OpenCV_VERSION}
        CMAKE_ARGS 
            BUILD_SHARED_LIBS=ON
            ENABLE_PRECOMPILED_HEADERS=OFF
    )
else()
    hunter_config(
        OpenCV
        VERSION ${HUNTER_OpenCV_VERSION}
        CMAKE_ARGS BUILD_SHARED_LIBS=ON
    )
endif()