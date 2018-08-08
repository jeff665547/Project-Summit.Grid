#pragma once
#include <stdexcept>
namespace summit::grid {

struct ChannelNotFound : public std::runtime_error {
    ChannelNotFound()
    : std::runtime_error("channel not found")
    {}
};

}