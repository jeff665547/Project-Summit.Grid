#pragma once
#include <summit/app/grid2/model.hpp>
#include <Nucleona/range.hpp>
#include "channel.hpp"
namespace summit::app::grid2::pipeline {

constexpr struct Chip {
    decltype(auto) operator()(model::Task& task) const {
        auto& executor = task.get_model().executor();

        task.set_white_channel_imgs(Utils::read_white_channel(
            task.channels(),
            task.chip_dir(),
            task.fov_rows(),
            task.fov_cols(),
            task.is_img_enc(),
            task.model()
        ));
        task.channels()
        | ranges::view::transform([&task](auto& jch){
            model::Channel ch_mod;
            ch_mod.set_task(task);
            ch_mod.set_channel(jch);
            return ch_mod;
        })
        | ranges::view::transform(channel)
        | nucleona::range::p_endp(executor)
        ;
        return 0;
    }
} chip;

}