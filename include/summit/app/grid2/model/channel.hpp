#pragma once
#include "macro.hpp"
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <summit/app/grid2/utils.hpp>
namespace summit::app::grid2 {
struct Task;
}
namespace summit::app::grid2::model {

struct Channel {
    void set_task(const Task& _task) {
        task_ = &_task;
    }
    void set_channel(const nlohmann::json& jch) {
        channel_ = &jch;
        ch_name_ = channel_->at("name");
    }
    void set_fov_imgs(Utils::FOVImages<std::uint16_t>&& _fov_imgs) {
        fov_imgs_ = std::move(_fov_imgs);
    }
    VAR_GET(std::string, ch_name)
    VAR_GET(Utils::FOVImages<std::uint16_t>, fov_imgs)
    VAR_PTR_GET(Task, task)
    VAR_PTR_GET(nlohmann::json, channel)
};

}