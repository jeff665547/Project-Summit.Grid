#pragma once
#include "macro.hpp"
#include "type.hpp"
#include "model.hpp"
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <summit/app/grid/utils.hpp>
#include <ChipImgProc/utils.h>
#include "single_img_proc_res.hpp"
#include <ChipImgProc/tiled_mat.hpp>
#include "channel.hpp"

namespace summit::app::grid::model {

struct FOV {
    using CimpMKRegion = chipimgproc::marker::detection::MKRegion;

    void init(
        Channel& ch, 
        const cv::Point& _fov_id, 
        chipimgproc::marker::Layout&& _mk_layout
    ) {
        channel_ = &ch;
        auto& task = channel_->task();
        auto& model = task.model();
        auto [img, img_path] = Utils::read_img<std::uint16_t>(
            task.chip_dir(),
            _fov_id.y, _fov_id.x,
            channel_->ch_name(),
            task.is_img_enc(),
            model
        );
        mk_layout_ = std::move(_mk_layout);
        src_path_ = std::move(img_path);
        src_ = img;
        fov_id_ = _fov_id;
        proc_bad_ = false;
        proc_done_ = false;
    }

    auto pch_rot_view() const {
        return channel_->pch_rot_view(fov_id_.y, fov_id_.x);
    }
    auto pch_mk_seg_view() const {
        return channel_->pch_mk_seg_view(fov_id_.y, fov_id_.x);
    }
    auto final_mk_seg_view() const {
        return channel_->final_mk_seg_view(fov_id_.y, fov_id_.x);
    }
    auto pch_grid_view() const {
        return channel_->pch_grid_view(fov_id_.y, fov_id_.x);
    }
    auto& take_tiled_mat() {
        return tiled_mat_;
    }
    auto& take_stat_mats() {
        return stat_mats_;
    }
    void set_in_grid_log(const nlohmann::json& _log ) {
        in_grid_log_ = &_log;
    }

    VAR_GET(boost::filesystem::path,            src_path                );
    VAR_GET(cv::Mat_<std::uint16_t>,            src                     );
    VAR_GET(cv::Point,                          fov_id                  );

    VAR_IO(chipimgproc::marker::Layout,         mk_layout               );
    VAR_IO(bool,                                proc_bad                );
    VAR_IO(bool,                                proc_done               );
    VAR_IO(chipimgproc::TiledMat<>,             tiled_mat               );
    VAR_IO(chipimgproc::stat::Mats<>,           stat_mats               );
    VAR_IO(std::vector<float>,                  bg_means                );
    VAR_IO(nlohmann::json,                      grid_log                );
    VAR_IO(std::vector<CimpMKRegion>,           mk_regs                 );
    VAR_IO(std::vector<cv::Point>,              low_score_marker_idx    );
    VAR_IO(std::string,                         mk_reg_src              ); // white channel, probe channel
    VAR_IO(cv::Mat,                             mk_append               );

    VAR_PTR_GET(Channel,                        channel                 );
    VAR_PTR_GET(nlohmann::json,                 in_grid_log             );

};

}