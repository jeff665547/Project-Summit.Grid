
#pragma once
#include <stdexcept>
namespace summit::app::grid {

struct UnknownChipLog : public std::runtime_error 
{
    UnknownChipLog()
    : std::runtime_error("unknown chip log format")
    {}
};

}