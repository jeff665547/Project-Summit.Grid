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

# # boost 
# hunter_add_package(Boost COMPONENTS 
#     thread 
#     system 
#     filesystem
#     # more boost module goes here
# )
# find_package(Boost CONFIG COMPONENTS 
#     thread 
#     system 
#     filesystem 
#     # more boost module goes here
#     REQUIRED
# )

# ChipImgProc 
hunter_add_package(ChipImgProc)
find_package(ChipImgProc CONFIG REQUIRED)

include(${SCREW_DIR}/hunter_root.cmake)