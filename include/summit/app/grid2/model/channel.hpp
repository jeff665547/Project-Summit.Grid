#pragma once
#include "macro.hpp"
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <summit/app/grid2/utils.hpp>
#include "single_img_proc_res.hpp"
#include "task.hpp"
#include "make_multi_tiled_mat.hpp"
#include "type.hpp"
#include "model.hpp"

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
    template<class T>
    auto make_fov_map(const T& v = {}) {
        Utils::FOVMap<T> m;
        for(auto&& fov_id : fov_imgs_ | ranges::view::keys) {
            m.emplace(fov_id, v);
        }
        return m;
    }
    void set_fov_good(Utils::FOVMap<bool>&& _fov_good) {
        fov_good_ = std::move(_fov_good);
    }
    // void set_fov_imp_res(Utils::FOVMap<OptSingleImgProcRes>&& _fov_res) {
    //     fov_imp_res_ = std::move(_fov_res);
    // }
    // auto& trans_to_multi_tiled_mat() {
    //     multi_tiled_mat_ = make_multi_tiled_mat(std::move(fov_imp_res_), task());
    //     return multi_tiled_mat_.value();
    // }
    void set_multi_tiled_mat(MTMat&& mt_mat) {
        multi_tiled_mat_ = std::move(mt_mat);
    }
    auto heatmap_writer() {
        return [this](MTMat& mt_mat){
            task_->model().heatmap_writer().write(
                task_->id().string(),
                ch_name_,
                task_->model().filter(),
                mt_mat
            );
        };
    }
    auto heatmap_writer() const {
        return nucleona::remove_const(*this).heatmap_writer();
    }
    auto background_writer() {
        return [this](const Utils::FOVMap<float>& bg_vs){
            task_->model().background_writer().write(
                task_->id().string(),
                ch_name_,
                bg_vs
            );
        };
    }
    auto background_writer() const {
        return nucleona::remove_const(*this).background_writer();
    }
    void write_output() {
        heatmap_writer()(multi_tiled_mat_.value());
    }
    VAR_GET(std::string, ch_name)
    VAR_GET(Utils::FOVImages<std::uint16_t>,        fov_imgs        )
    VAR_GET(Utils::FOVMap<bool>,                    fov_good        )
    // VAR_GET(Utils::FOVMap<OptSingleImgProcRes>,     fov_imp_res     )
    VAR_GET(OptMTMat,                               multi_tiled_mat )

    VAR_PTR_GET(Task,                               task)
    VAR_PTR_GET(nlohmann::json,                     json)
};

}