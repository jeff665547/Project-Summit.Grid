# Build from source code

## Tool chain
* [Visual Studio Community (>=2019)](https://visualstudio.microsoft.com/downloads/)
* [MinGW (7.2.0 or 7.3.0 ONLY)](https://sourceforge.net/projects/mingw-w64/)
* [CMake (>=3.10.0)](https://cmake.org/download/)
* [Git (>=2.28.0)](https://gitforwindows.org/)

## Installing and Environment

 * Windows 10 Pro with ENGLISH as default system language
 * Installing order: 
   1. Visual Studio
   2. Git
   3. MinGW
   4. CMake

 * Visual Studio, C++ pack with additional CLANG compiler
 * MinGW, use POSIX with SEH when installing
   * Add path to system environment
   * C:\Program Files\mingw-w64\x86_64-[DEPEND ON YOUR VERSION]\mingw64\bin
 * CMake, add path when installing

## Clone
```
git clone ssh://git@gitlab.centrilliontech.com.tw:10022/centrillion/Summit.Grid.git
cd Summit.Grid
git submodule init
git submodule update
```

## Create build directory
```
mkdir build stage
cd build
```

## Configure
```
cmake -G "MinGW Makefiles" ..\ -DCMAKE_INSTALL_PREFIX="..\stage" -DCMAKE_BUILD_TYPE="Release" -DENABLE_LOG=ON
```

## Build
```
cmake --build . --target install
```

then the build result will in Summit.grid/stage.

## Execute
```
./summit-app-grid -i <path to shared folder>/<path to image folder> -o <path to secure folder>/<path to image folder> -l all -r csv_probe_list,cen_file --shared_dir <path to shared folder> --secure_dir <path to secure folder> --no_bgp --marker_append -d 6
```
"-d 6" means logger level, so higher value will result in more log message, yet will slow the program.