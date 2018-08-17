hunter_config(Screw GIT_SUBMODULE "cmake/screw")
hunter_config(ChipImgProc GIT_SUBMODULE "lib/ChipImgProc"
    CMAKE_ARGS
        BUILD_TESTS=OFF
)
hunter_config(NucleonaM GIT_SUBMODULE "lib/Nucleona"
    CMAKE_ARGS 
        BUILD_TESTS=OFF 
        ENABLE_HDF5=ON
)
hunter_config(CFU GIT_SUBMODULE "lib/CFU"
    CMAKE_ARGS
        BUILD_TESTS=OFF
)
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
if( MINGW )
    hunter_config(
        OpenCV
        VERSION "3.4.0-p0"
        CMAKE_ARGS 
            BUILD_SHARED_LIBS=ON
            ENABLE_PRECOMPILED_HEADERS=OFF
    )
else()
    hunter_config(
        OpenCV
        VERSION "3.4.0-p0"
        CMAKE_ARGS BUILD_SHARED_LIBS=ON
    )
endif()