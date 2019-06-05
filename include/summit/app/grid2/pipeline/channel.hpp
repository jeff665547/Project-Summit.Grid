#pragma once
#include <summit/app/grid2/model.hpp>
namespace summit::app::grid2::pipeline {

constexpr struct Channel {
    decltype(auto) operator()(model::Channel&& channel) const {
        // for each fov,
        // detect dark marker
        // compare theta
        // run white channel support algorithm
        return 0;
    }
} channel;

}