#pragma once
#include <stdexcept>
namespace summit::grid {

struct EmptyChipType : public std::runtime_error 
{
    EmptyChipType()
    : std::runtime_error("empty chip type")
    {}
};

}