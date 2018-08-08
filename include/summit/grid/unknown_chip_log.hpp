
#pragma once
#include <stdexcept>
namespace summit::grid {

struct UnknownChipLog : public std::runtime_error 
{
    UnknownChipLog()
    : std::runtime_error("unknown chip log format")
    {}
};

}