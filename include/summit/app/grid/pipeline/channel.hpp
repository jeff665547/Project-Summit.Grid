#pragma once
#include <summit/app/grid/model.hpp>
#include <limits>
#include <summit/app/grid/model/type.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/marker/detection/reg_mat.hpp>
#include <ChipImgProc/gridding/reg_mat.hpp>
#include <ChipImgProc/rotation/iteration_cali.hpp>
#include <ChipImgProc/rotation/calibrate.hpp>
#include <ChipImgProc/rotation/marker_vec.hpp>
#include <ChipImgProc/stitch/gridline_based.hpp>
#include "fov_ag.hpp"
#include "fov_nag.hpp"

namespace summit::app::grid::pipeline {

namespace __alias {

namespace cimp      = chipimgproc;
namespace cmk       = chipimgproc::marker;
namespace crot      = chipimgproc::rotation;
namespace cmk_det   = chipimgproc::marker::detection;
namespace cimg      = chipimgproc::gridding;
namespace cm_st     = chipimgproc::stitch;

}

constexpr struct Channel {
    using Float = summit::app::grid::model::Float;
    using GridLineID = summit::app::grid::model::GridLineID;
    auto gridding(model::Channel& channel) const {
        using namespace __alias;
        auto& task = channel.task();
        auto& model = task.model();
        if(!task.rot_degree_done())
            throw std::runtime_error("[BUG]: no rotation degree, surrender gridding");
        if(!task.um_to_px_r_done())
            throw std::runtime_error("[BUG]: no um to pixel rate, surrender gridding");
        auto& executor = task.model().executor();

        Utils::FOVMap<model::FOV> fov_mods;
        auto fov_ids = Utils::fov_ids(task.fov_rows(), task.fov_cols());
        for(auto&& fov_id : fov_ids) { fov_mods[fov_id] = {}; }

        if(!model.auto_gridding()) {
            for(auto&& fov_in_grid_log : channel.in_grid_log().at("fovs")) {
                auto& fov_id = fov_in_grid_log.at("id");
                auto& mod = fov_mods.at(cv::Point(
                    fov_id[0].get<int>(), fov_id[1].get<int>()
                ));
                mod.set_in_grid_log(
                    fov_in_grid_log
                );
            }
        }

        // TODO: use cache
        auto tmp_fov_mk_layouts = Utils::generate_sgl_pat_reg_mat_marker_layout(
            task.um2px_r(),
            task.chipspec(),
            task.fov(),
            channel.json()
        );
        fov_mods
        | ranges::view::transform([&](auto&& fov_id_mod){
            auto& fov_id = fov_id_mod.first;
            auto& fov_mod = fov_id_mod.second;
            fov_mod.init(
                channel,
                fov_id,
                std::move(tmp_fov_mk_layouts.at(fov_id))
            );
            if(model.auto_gridding()) {
                fov_ag(fov_mod);
            } else {
                fov_nag(fov_mod);
            }
            return 0;
        })
        | nucleona::range::p_endp(executor);
        channel.collect_fovs(fov_mods);

        // write heatmap
        // FIXME: remove_const -> multi tiled mat immutable workaround
        channel.heatmap_writer()(
            nucleona::remove_const(channel.multi_tiled_mat().value())
        );

        // stitch image
        auto grid_image = image_stitcher(channel.multi_tiled_mat().value());
        auto v_st_img = cimp::viewable(grid_image.mat());
        auto st_img_path = channel.stitch_image("norm");
        cv::imwrite(st_img_path.string(), v_st_img);

        // gridline
        std::ofstream gl_file(channel.gridline().string());
        Utils::write_gl(gl_file, grid_image);
        channel.set_gridline(grid_image.gl_x(), grid_image.gl_y());

        // background
        Utils::FOVMap<float> bg_value;
        for(auto&& p : fov_mods) {
            auto& fov_id = p.first;
            auto& fov_mod = p.second;
            bg_value[fov_id] = Utils::mean(fov_mod.bg_means());
        }
        channel.background_writer()(bg_value);
        
    }
    decltype(auto) operator()(model::Channel& channel) const {
        try{
            gridding(channel);
            channel.update_grid_done();
        } catch (const std::exception& e) {
            channel.set_grid_failed(e.what());
            summit::grid::log.error(
                "channel: {} process failed, reason: {}", channel.ch_name(), e.what()
            );
        }
        // channel.write_log();
        return channel.grid_log().at("grid_done").get<bool>();
    }
private:
    __alias::cm_st::GridlineBased               image_stitcher          ;
} channel;

}