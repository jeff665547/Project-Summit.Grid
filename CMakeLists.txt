cmake_minimum_required(VERSION 3.10.0)
set(
    HUNTER_CACHE_SERVERS
    "https://github.com/CHChang810716/hunter-binary-cache"
    CACHE
    STRING
    "Hunter cache servers"
)
include(cmake/HunterGate.cmake)
HunterGate(
    URL "ssh://git@gitlab.centrilliontech.com.tw:10022/centrillion/hunter.git"
    SHA1 9eb716ecf7a01e1480085c01d748f45617c4a301
    FILEPATH ${CMAKE_CURRENT_LIST_DIR}/cmake/packages.cmake
)
project(SummitGrid LANGUAGES C CXX)

hunter_add_package(Screw)
find_package( Screw CONFIG REQUIRED )

include(cmake/version.cmake)
include(cmake/options.cmake)
include(cmake/dependencies.cmake)
include(cmake/compiler.cmake)
include(cmake/src.cmake)
include(cmake/unit_test.cmake)
include(cmake/ide.cmake)
include(cmake/licenses.cmake)
include(cmake/install.cmake)
include(cmake/cpack.cmake)
