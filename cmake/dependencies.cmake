# gtest
if(BUILD_TESTS)
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
endif()

# json
hunter_add_package(nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)

# nucleona 
hunter_add_package(Nucleona)
find_package(Nucleona CONFIG REQUIRED)

# range-v3 workaround
find_package(range-v3 CONFIG REQUIRED)
if(MSVC)
    target_compile_options(range-v3 INTERFACE /permissive-)
endif()

# CFU
hunter_add_package(CFU)
find_package(CFU CONFIG REQUIRED)

# ChipImgProc 
hunter_add_package(ChipImgProc)
find_package(ChipImgProc CONFIG REQUIRED)

# OpenCV
screw_get_bits(BITS)
if(WIN32)
    list(APPEND BUNDLE_RT_DIRS ${OpenCV_DIR}/x${BITS}/${OpenCV_RUNTIME}/bin)
    list(APPEND BUNDLE_RT_DIRS ${OpenCV_DIR}/x${BITS}/${OpenCV_RUNTIME}/lib)
else()
endif()
include(${SCREW_DIR}/hunter_root.cmake)

# SummitCrypto
hunter_add_package(SummitCrypto)
find_package(SummitCrypto CONFIG REQUIRED)

# spdlog
hunter_add_package(spdlog)
find_package(spdlog)

# fmt
hunter_add_package(fmt)
find_package(fmt)

