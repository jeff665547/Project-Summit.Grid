# gtest
if(BUILD_TESTS)
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
endif()

# json
hunter_add_package(nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)

# nucleona 
hunter_add_package(NucleonaM)
find_package(Nucleona CONFIG REQUIRED)

# CFU
hunter_add_package(CFU)
find_package(CFU CONFIG REQUIRED)

# ChipImgProc 
hunter_add_package(ChipImgProc)
find_package(ChipImgProc CONFIG REQUIRED)

# OpenCV
screw_get_bits(BITS)
list(APPEND BUNDLE_RT_DIRS ${OpenCV_DIR}/x${BITS}/${OpenCV_RUNTIME}/bin)
list(APPEND BUNDLE_RT_DIRS ${OpenCV_DIR}/x${BITS}/${OpenCV_RUNTIME}/lib)

include(${SCREW_DIR}/hunter_root.cmake)