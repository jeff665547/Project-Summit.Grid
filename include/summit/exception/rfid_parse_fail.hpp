#pragma once
#include <stdexcept>
namespace summit{ namespace exception{

struct RFIDParseFail
: public std::runtime_error
{
    RFIDParseFail()
    : std::runtime_error("RFID parse fail")
    {}
};

}}