#pragma once
#include <summit/app/grid2/model.hpp>
namespace summit::app::grid2::pipeline {

constexpr struct Chip {
    decltype(auto) operator()(model::Task& task) const {
        auto white_img = Utils::read_white_channel(
            task.channels(),
            task.chip_dir(),
            task.fow_rows(),
            task.fov_cols(),
            task.is_img_enc(),
            task.model().paths()
        );
        for(auto&& ch : task.channels()) {
            std::string chname = ch.at("name");
        }
        // um2px rate compute
        // 
        return 0;
    }
} chip;

}