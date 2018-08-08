#pragma once
#include <stdexcept>
namespace summit::grid {

struct ImageQCFail : public std::runtime_error 
{
    ImageQCFail()
    : std::runtime_error("image quality control fail")
    {}
};

}