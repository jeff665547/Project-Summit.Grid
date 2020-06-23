#pragma once
// #include "logger/impl.hpp"
#ifdef SUMMITGRID_ENABLE_LOG
#   pragma message "build with log"
#   include "logger/impl.hpp"
#else
#   pragma message "build without log"
#   include "logger/nolog_impl.hpp"
#endif