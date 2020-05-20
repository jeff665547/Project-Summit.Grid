if( MINGW )
    set(ZLIB_BUILD_SHARED_LIBS OFF)
    set(OpenCV_ENABLE_PRECOMPILED_HEADERS OFF)
else()
    set(ZLIB_BUILD_SHARED_LIBS ON)
    set(OpenCV_ENABLE_PRECOMPILED_HEADERS ON)
endif()

hunter_config(Nucleona
    VERSION ${HUNTER_Nucleona_VERSION}
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
    OpenCV-Extra
    VERSION "3.4.0"
)
hunter_config(SummitCrypto GIT_SUBMODULE "lib/Summit.Crypto"
    CMAKE_ARGS
        BUILD_TESTS=OFF
)
hunter_config(range-v3
    VERSION "0.5.0"
)
hunter_config(cryptopp
    VERSION "5.6.5-p0"
)
hunter_config(ChipImgProc
    VERSION ${HUNTER_ChipImgProc_VERSION}
    CMAKE_ARGS
        ENABLE_LOG=ON
)
# hunter_config(ChipImgProc GIT_SUBMODULE "lib/ChipImgProc"
#     CMAKE_ARGS
#         ENABLE_LOG=ON
# )