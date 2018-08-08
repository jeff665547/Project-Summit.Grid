#pragma once
#include <stdexcept>
namespace summit{ namespace exception{

struct ChipSpecNotFound
: public std::runtime_error
{
    ChipSpecNotFound()
    : std::runtime_error("chip spec not found")
    {}
};

}}