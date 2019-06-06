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
    void set_json(const nlohmann::json& jch) {
        json_ = &jch;
        ch_name_ = json_->at("name");
    }
    void set_fov_imgs(Utils::FOVImages<std::uint16_t>&& _fov_imgs) {
        fov_imgs_ = std::move(_fov_imgs);
        for(auto&& fov_id : fov_imgs_ | ranges::view::keys) {
            fov_good_[fov_id] = false;
        }
    }
    void set_fov_good(Utils::FOVMap<bool>&& _fov_good) {
        fov_good_ = std::move(_fov_good);
    }
    VAR_GET(std::string, ch_name)
    VAR_GET(Utils::FOVImages<std::uint16_t>, fov_imgs)
    VAR_GET(Utils::FOVMap<bool>,             fov_good)

    VAR_PTR_GET(Task, task)
    VAR_PTR_GET(nlohmann::json, json)
};

}