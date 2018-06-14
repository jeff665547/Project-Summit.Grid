# if( MSVC )
#     hunter_config(
#         GTest
#         VERSION ${HUNTER_GTest_VERSION}
#         CMAKE_ARGS 
#             CMAKE_CXX_FLAGS=/D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
#     )
# endif()
hunter_config(Screw GIT_SUBMODULE "cmake/screw")
hunter_config(NucleonaM GIT_SUBMODULE "lib/Nucleona")
hunter_config(Affy GIT_SUBMODULE "lib/Affy"
    CMAKE_ARGS 
        BUILD_TESTS=OFF 
)
# set(str "TARGET=CORTEXA57;BINARY=64;HOSTCC=gcc")
# hunter_config(
#     OpenBLAS
#     VERSION "0.2.19-p0"
#     CMAKE_ARGS 
#         target_arg=${str}
# )
# if( MINGW )
#     hunter_config(
#         OpenCV
#         VERSION ${HUNTER_OpenCV_VERSION}
#         CMAKE_ARGS 
# 	    BUILD_SHARED_LIBS=OFF
#             ENABLE_PRECOMPILED_HEADERS=OFF
# 	    WITH_PROTOBUF=OFF
#     )
# else()
#     hunter_config(
#         OpenCV
#         VERSION ${HUNTER_OpenCV_VERSION}
# 	    CMAKE_ARGS 
#             BUILD_SHARED_LIBS=OFF
#             WITH_PROTOBUF=OFF
#     )
# endif()
# hunter_config(
#     Boost
#     VERSION "1.64.0"
# )
