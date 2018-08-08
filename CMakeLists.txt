cmake_minimum_required(VERSION 3.0.0)
include(cmake/HunterGate.cmake)
set(
    HUNTER_CACHE_SERVERS
    "https://github.com/elucideye/hunter-cache;https://github.com/ingenue/hunter-cache"
    CACHE
    STRING
    "Hunter cache servers"
)
HunterGate(
    URL "ssh://git@gitlab.centrilliontech.com.tw:10022/centrillion/hunter.git"
    SHA1 f4b65c22d782adddc3de9544660dc9a036335d34
    FILEPATH ${CMAKE_CURRENT_LIST_DIR}/cmake/packages.cmake
)
project(Summit LANGUAGES C CXX)

hunter_add_package(Screw)
find_package( Screw CONFIG REQUIRED )

include(cmake/version.cmake)
include(cmake/options.cmake)
include(cmake/dependencies.cmake)
include(cmake/compiler.cmake)
include(cmake/src.cmake)
include(cmake/unit_test.cmake)
include(cmake/ide.cmake)
include(cmake/install.cmake)
include(cmake/cpack.cmake)
