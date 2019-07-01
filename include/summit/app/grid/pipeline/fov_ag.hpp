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
#include <summit/app/grid/model/fov.hpp>
namespace summit::app::grid::pipeline {

namespace __alias {

namespace cimp      = chipimgproc;
namespace cmk       = chipimgproc::marker;
namespace crot      = chipimgproc::rotation;
namespace cmk_det   = chipimgproc::marker::detection;
namespace cimg      = chipimgproc::gridding;
namespace cm_st     = chipimgproc::stitch;

}

constexpr struct FOVAG {
    using Float = summit::app::grid::model::Float;
    using GridLineID = summit::app::grid::model::GridLineID;
    auto find_and_set_best_marker(
        const cv::Mat_<std::uint16_t>& mat,
        __alias::cmk::Layout& mk_layout,
        const float& rot_deg
    ) const {
        using namespace __alias;
        std::vector<cv::Point>      low_score_marker_idx    ;
        std::size_t best_i = 0;
        float min_theta_diff = std::numeric_limits<float>::max();
        for(auto i : nucleona::range::irange_0(
            mk_layout.get_single_pat_candi_num()
        )) {
            auto mk_regs = probe_mk_detector(
                mat, mk_layout,
                cimp::MatUnit::PX, i
            );
            auto tmp_ls_mk_idx = cmk_det::filter_low_score_marker(mk_regs);
            auto test_theta = rotate_detector(mk_regs);
            auto theta_diff = std::abs(test_theta - rot_deg);
            if(theta_diff < min_theta_diff) {
                min_theta_diff = theta_diff;
                best_i = i;
                low_score_marker_idx = tmp_ls_mk_idx;
            }
        }
        mk_layout.set_single_pat_best_mk(best_i);
        return nucleona::make_tuple(
            std::move(min_theta_diff), 
            std::move(low_score_marker_idx)
        );
    }
    auto operator()(model::FOV& fov_mod) const {
        using namespace __alias;
        auto& channel    = fov_mod.channel();
        auto& fov_id     = fov_mod.fov_id();
        auto& path       = fov_mod.src_path();
        auto  mat        = fov_mod.src().clone();
        auto& mk_layout  = fov_mod.mk_layout();
        auto& f_grid_log = fov_mod.grid_log();
        auto& grid_bad   = fov_mod.proc_bad();
        auto& grid_done  = fov_mod.proc_done();
        auto& task       = fov_mod.channel().task();
        auto log_prefix  = fmt::format("[{}-{}-({},{})]", 
            task.id().chip_id(), channel.ch_name(), fov_id.x, fov_id.y
        );
        summit::grid::log.trace(
            "{}: start auto gridding process", 
            log_prefix
        );
        try {
            auto wh_mk_regs  = task.fov_mk_regs().at(fov_id);
            // * determine the no rotation marker region
            auto td_lsmk     = find_and_set_best_marker(
                mat, mk_layout, task.rot_degree().value()
            );
            auto& theta_diff = std::get<0>(td_lsmk);
            auto& ls_mk_idx  = std::get<1>(td_lsmk);
            rotate_calibrator(
                mat, 
                task.rot_degree().value(),
                fov_mod.pch_rot_view()
            );
            auto  mk_regs    = cmk_det::reg_mat_no_rot(
                mat, mk_layout,
                cimp::MatUnit::PX, ls_mk_idx,
                nucleona::stream::null_out,
                fov_mod.pch_mk_seg_view()
            );
            auto& mk_des     = mk_layout.get_single_pat_marker_des();
            auto& std_mk_px  = mk_des.get_std_mk(cimp::MatUnit::PX);
            for(auto&& mk : wh_mk_regs) {
                mk.x = mk.x - (std_mk_px.cols / 2);
                mk.y = mk.y - (std_mk_px.rows / 2);
                mk.width  = std_mk_px.cols;
                mk.height = std_mk_px.rows;
            }
            if(theta_diff > 0.5)  {
                if(wh_mk_regs.empty()) {
                    grid_bad = true;
                } else {
                    mk_regs = wh_mk_regs;
                    fov_mod.mk_reg_src() = "white_channel";
                }
            } else {
                if(!wh_mk_regs.empty()) {
                    auto& std_mk_cl = mk_des.get_std_mk(cimp::MatUnit::CELL);
                    const auto px_per_cl_w = std_mk_px.cols / (float)std_mk_cl.cols;
                    const auto shift_thd_w = px_per_cl_w / 2;

                    const auto px_per_cl_h = std_mk_px.rows / (float)std_mk_cl.rows;
                    const auto shift_thd_h = px_per_cl_h / 2;

                    auto& d_mk_reg = mk_regs.at(0);
                    auto& w_mk_reg = wh_mk_regs.at(0);
                    
                    if( std::abs(d_mk_reg.x - w_mk_reg.x) > shift_thd_w ||
                        std::abs(d_mk_reg.y - w_mk_reg.y) > shift_thd_h
                    ) {
                        mk_regs = wh_mk_regs;
                        fov_mod.mk_reg_src() = "white_channel";
                    } else {
                        grid_bad = false;
                        fov_mod.mk_reg_src() = "probe_channel";
                    }
                }
            }
            auto final_mk_seg_view = fov_mod.final_mk_seg_view();
            if(final_mk_seg_view) {
                final_mk_seg_view(cmk::view(mat, mk_regs));
            }
            // gridding
            auto grid_res = gridder(mat, mk_layout, mk_regs,
                nucleona::stream::null_out,
                fov_mod.pch_grid_view()
            );
            // write norm result
            // auto fov_norm_path = channel.fov_image("norm", fov_id.y, fov_id.x);
            // cv::imwrite(fov_norm_path.string(), chipimgproc::viewable(mat));

            // write raw result
            auto fov_norm_path = channel.fov_image("raw", fov_id.y, fov_id.x);
            auto x0 = grid_res.gl_x.at(0);
            auto y0 = grid_res.gl_y.at(0);
            auto x_d = grid_res.gl_x.back() - x0;
            auto y_d = grid_res.gl_y.back() - y0;
            cv::imwrite(fov_norm_path.string(), mat(
                cv::Rect(x0, y0, x_d, y_d)
            ));

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
            fov_mod.set_bg_means(std::move(bg_value));
            f_grid_log["gl_x"] = grid_res.gl_x;
            f_grid_log["gl_y"] = grid_res.gl_y;
            f_grid_log["grid_bad"] = grid_bad;
            f_grid_log["marker_region_source"] = fov_mod.mk_reg_src();
            auto& fov_log_id = f_grid_log["id"];
            fov_log_id = nlohmann::json::array();
            fov_log_id[0] = fov_id.x;
            fov_log_id[1] = fov_id.y;

            grid_done = true;
        } catch(const std::exception& e) {
            f_grid_log["grid_fail_reason"] = e.what();
            grid_done = false;
            grid_bad = true;
            summit::grid::log.error(
                "channel: {}, FOV: ({},{}) process failed, reason: {}", 
                channel.ch_name(), fov_id.x, fov_id.y, e.what()
            );
        }
        f_grid_log["grid_done"] = grid_done;
        f_grid_log["grid_bad"] = grid_bad;
        return grid_done;
    }
    __alias::crot::Calibrate                    rotate_calibrator       ;
    __alias::cmk_det::RegMat                    probe_mk_detector       ;
    __alias::crot::MarkerVec<float>             rotate_detector         ;
    __alias::cimg::RegMat                       gridder                 ;
    __alias::cimp::Margin<Float, GridLineID>    margin                  ;

} fov_ag;


}