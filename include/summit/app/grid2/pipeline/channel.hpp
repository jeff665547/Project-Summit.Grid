#pragma once
#include <summit/app/grid2/model.hpp>
namespace summit::app::grid2::pipeline {

constexpr struct Channel {
    decltype(auto) operator()(model::Channel&& channel) const {
        return 0;
    }
} channel;

}