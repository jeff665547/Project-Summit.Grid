if( MINGW )
    set(ZLIB_BUILD_SHARED_LIBS OFF)
    set(OpenCV_ENABLE_PRECOMPILED_HEADERS OFF)
else()
    set(ZLIB_BUILD_SHARED_LIBS ON)
    set(OpenCV_ENABLE_PRECOMPILED_HEADERS ON)
endif()

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
hunter_config(
    ZLIB 
    VERSION ${HUNTER_ZLIB_VERSION}
    CMAKE_ARGS 
        BUILD_SHARED_LIBS=${ZLIB_BUILD_SHARED_LIBS}
)
hunter_config(
    Jpeg 
    VERSION ${HUNTER_Jpeg_VERSION}
    CMAKE_ARGS
        BUILD_SHARED_LIBS=ON
)
hunter_config(
    PNG
    VERSION ${HUNTER_PNG_VERSION}
    CMAKE_ARGS
        BUILD_SHARED_LIBS=ON
)
hunter_config(
    TIFF
    VERSION ${HUNTER_TIFF_VERSION}
    CMAKE_ARGS
        BUILD_SHARED_LIBS=ON
)
hunter_config(
    OpenCV
    VERSION "3.4.0-p0"
    CMAKE_ARGS 
        BUILD_SHARED_LIBS=ON
        ENABLE_PRECOMPILED_HEADERS=${OpenCV_ENABLE_PRECOMPILED_HEADERS}
)
hunter_config(
    Boost
    VERSION "1.64.0"
)