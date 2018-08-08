#pragma once
#include <stdexcept>
namespace summit::app::grid {

struct ImageQCFail : public std::runtime_error 
{
    ImageQCFail()
    : std::runtime_error("image quality control fail")
    {}
};

}