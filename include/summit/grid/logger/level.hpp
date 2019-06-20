#pragma once
#include <spdlog/spdlog.h>
namespace summit::grid::logger {
constexpr struct LevelTrans {
    int operator()(spdlog::level::level_enum spd_lev) const {
        return 6 - static_cast<int>(spd_lev);
    }
    spdlog::level::level_enum operator()(int n) const {
        return static_cast<spdlog::level::level_enum>(6 - n);
    }
} level_trans;

}