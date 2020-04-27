/**
 * @file fov_nag.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::pipeline::FOVNAG
 */
#pragma once
#include <summit/app/grid/model.hpp>
#include <limits>
#include <summit/app/grid/model/type.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/marker/detection/reg_mat.hpp>
#include <ChipImgProc/gridding/pseudo.hpp>
#include <ChipImgProc/rotation/iteration_cali.hpp>
#include <ChipImgProc/rotation/calibrate.hpp>
#include <ChipImgProc/rotation/marker_vec.hpp>
#include <ChipImgProc/stitch/gridline_based.hpp>
#include <summit/app/grid/model/fov.hpp>
#include <ChipImgProc/marker/detection/layout_lookup.hpp>
#include <fmt/format.h>
namespace summit::app::grid::pipeline {

namespace __alias {

namespace cimp      = chipimgproc;
namespace cmk       = chipimgproc::marker;
namespace crot      = chipimgproc::rotation;
namespace cmk_det   = chipimgproc::marker::detection;
namespace cimg      = chipimgproc::gridding;
namespace cm_st     = chipimgproc::stitch;

}

/**
 * @brief FOV manual gridding data generate
 * @details Re-generate grid data by user modified grid log
 */
constexpr struct FOVNAG {
    using Float = summit::app::grid::model::Float;
    using GridLineID = summit::app::grid::model::GridLineID;
    /**
     * @brief Run the manual gridding.
     * 
     * @param fov_mod FOV parameter model.
     * @return true  grid done
     * @return false grid failed
     */
    bool operator()(model::FOV& fov_mod) const {
        using namespace __alias;
        auto& channel     = fov_mod.channel();
        auto& fov_id      = fov_mod.fov_id();
        auto& path        = fov_mod.src_path();
        auto  mat         = fov_mod.src().clone();
        auto& mk_layout   = fov_mod.mk_layout();
        auto& f_grid_log  = fov_mod.grid_log();
        auto& grid_bad    = fov_mod.proc_bad();
        auto& grid_done   = fov_mod.proc_done();
        auto& in_grid_log = fov_mod.in_grid_log();
        auto& task        = fov_mod.channel().task();
        auto log_prefix   = fmt::format("[{}-{}-({},{})]", 
            task.id().chip_id(), channel.ch_name(), fov_id.x, fov_id.y
        );
        summit::grid::log.trace(
            "{}: start non-auto gridding process", 
            log_prefix
        );
        try {
            // * determine the no rotation marker region
            rotate_calibrator(
                mat, 
                task.rot_degree().value(),
                fov_mod.pch_rot_view()
            );
            // gridding
            auto grid_res = gridder(
                Utils::abs_px(
                    in_grid_log.at("x0_px").get<double>(),
                    in_grid_log.at("du_x").get<std::vector<double>>(),
                    task.um2px_r()
                ),
                Utils::abs_px(
                    in_grid_log.at("y0_px").get<double>(),
                    in_grid_log.at("du_y").get<std::vector<double>>(),
                    task.um2px_r()
                ),
                fov_mod.pch_grid_view(),
                &mat
            );
            grid_bad = false;
            // marker location
            auto ru32 = [](auto&& v){ 
                return (std::uint32_t)std::round(v);
            };
            auto mk_regs = cmk_det::layout_lookup(
                grid_res.gl_x | ranges::view::transform(ru32), 
                grid_res.gl_y | ranges::view::transform(ru32), 
                mk_layout
            );
            fov_mod.set_mk_reg_src("gridline_infer");

            auto final_mk_seg_view = fov_mod.final_mk_seg_view();
            if(final_mk_seg_view) {
                final_mk_seg_view(cmk::view(mat, mk_regs));
            }

            if(task.model().marker_append()) {
                auto mk_append_res = cmk::roi_append(
                    mat, mk_layout, mk_regs
                );
                fov_mod.set_mk_append(
                    std::move(mk_append_res)
                );
            }
            auto tiled_mat = cimp::TiledMat<>::make_from_grid_res(
                grid_res, mat, mk_layout
            );
            cimp::GridRawImg<> grid_raw_img(
                mat, grid_res.gl_x, grid_res.gl_y
            );

            auto tiles = tiled_mat.get_tiles();
            auto margin_res = margin(
                "auto_min_cv",
                cimp::margin::Param<GridLineID> {
                    0.6,
                    &tiled_mat,
                    task.model().no_bgp(),
                    std::function<void(const cv::Mat&)>(nullptr)
                }
            );
            auto bg_value = cimp::bgb::chunk_local_mean(
                margin_res.stat_mats.mean,
                grid_raw_img,
                3, 3, 0.05,
                mk_layout,
                !task.model().no_bgp(),
                nucleona::stream::null_out
            );
            if(task.model().no_bgp()) {
                tiled_mat.get_cali_img() = grid_raw_img.mat();
                tiled_mat.get_tiles() = tiles;
                margin_res = margin(
                    "auto_min_cv",
                    cimp::margin::Param<GridLineID> {
                        0.6,
                        &tiled_mat,
                        true,
                        std::function<void(const cv::Mat&)>(nullptr)
                    }
                );
            }
            fov_mod.set_tiled_mat(std::move(tiled_mat));
            fov_mod.set_stat_mats(std::move(margin_res.stat_mats));
            // fov_mod.set_bg_means(std::move(bg_value));
            f_grid_log["x0_px"] = in_grid_log.at("x0_px");
            f_grid_log["y0_px"] = in_grid_log.at("y0_px");
            f_grid_log["du_x"] = in_grid_log.at("du_x");
            f_grid_log["du_y"] = in_grid_log.at("du_y");
            f_grid_log["grid_bad"] = grid_bad;
            f_grid_log["marker_region_source"] = fov_mod.mk_reg_src();
            f_grid_log["id"] = fov_mod.in_grid_log().at("id");
            grid_done = true;
        } catch(const std::exception& e) {
            f_grid_log["grid_fail_reason"] = e.what();
            grid_done = false;
            summit::grid::log.error(
                "channel: {}, FOV: ({},{}) process failed, reason: {}", 
                channel.ch_name(), fov_id.x, fov_id.y, e.what()
            );
        }
        f_grid_log["grid_done"] = grid_done;
        return grid_done;
    }
    __alias::crot::Calibrate                    rotate_calibrator       ;
    __alias::cmk_det::RegMat                    probe_mk_detector       ;
    __alias::crot::MarkerVec<float>             rotate_detector         ;
    __alias::cimg::Pseudo                       gridder                 ;
    __alias::cimp::Margin<Float, GridLineID>    margin                  ;

} fov_nag;


}