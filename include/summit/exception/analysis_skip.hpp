#pragma once
#include <stdexcept>
namespace summit{ namespace exception{

struct AnalysisSkip
: public std::runtime_error
{
    AnalysisSkip(const std::string& reason)
    : std::runtime_error("analysis skipped, reason: " + reason)
    {}
};

}}