/**
 * @file fov_ag.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::pipeline::FOVAG
 * 
 */
#pragma once
#include <summit/app/grid/model.hpp>
#include <summit/app/grid/model/type.hpp>
#include <summit/app/grid/model/fov.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/marker/detection/reg_mat.hpp>
#include <ChipImgProc/gridding/reg_mat.hpp>
#include <ChipImgProc/rotation/iteration_cali.hpp>
#include <ChipImgProc/rotation/calibrate.hpp>
#include <ChipImgProc/rotation/marker_vec.hpp>
#include <ChipImgProc/stitch/gridline_based.hpp>
#include <ChipImgProc/bgb/chunk_local_mean.hpp>
#include <ChipImgProc/bgb/bspline.hpp>
#include <ChipImgProc/margin.hpp>
#include <ChipImgProc/marker/detection/filter_low_score_marker.hpp>
#include <ChipImgProc/marker/detection/reg_mat_no_rot.hpp>
#include <ChipImgProc/marker/view.hpp>
#include <ChipImgProc/marker/roi_append.hpp>
#include <ChipImgProc/utils/mat_to_vec.hpp>
#include <limits>
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
 * @brief FOV level automatic gridding
 * @details Doing marker detection and use marker 
 *          position as anchor to grid FOV image.
 *          See @ref fov-level-process "FOV level process" for more details.
 * 
 */
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
    /**
     * @brief Set the background value to statistic matrix.
     * 
     * @tparam StatMats statistic matrix type
     * @tparam GLID grid line type
     * @param stat_mat statistic matrix, result storage
     * @param tiled_mat FOV image represented in tiled matrix
     * @param bg background value 
     */
    template<class StatMats, class GLID>
    void set_bg(
        StatMats&                   stat_mat, 
        const chipimgproc::TiledMat<GLID>& tiled_mat, 
        const cv::Mat_<double>&     bg
    ) const {
        auto x_org = tiled_mat.glx().at(0);
        auto y_org = tiled_mat.gly().at(0);
        const auto& tiles = tiled_mat.get_tiles();
        const auto& first_tile = tiles.at(0);
        const auto& last_tile = tiles.back();
        for(int i = 0; i < tiled_mat.rows(); i ++ ) {
            for(int j = 0; j < tiled_mat.cols(); j ++ ) {
                cv::Rect tile = tiled_mat.tile_at(i, j);
                tile.x -= x_org;
                tile.y -= y_org;
                cv::Mat_<double> sub_bg = bg(tile);
                stat_mat.bg(i, j) = Utils::mean(sub_bg);
            }
        }
    }
    /**
     * @brief Run FOV level gridding process.
     * 
     * @param fov_mod FOV parameter model.
     * @return true Gridding process is done.
     * @return false Gridding process is failed, the fail reason will store in grid log.
     */
    bool operator()(model::FOV& fov_mod) const {
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
            summit::grid::log.debug("{}: probe channel rotation: {}", log_prefix, task.rot_degree().value());
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
                mk.x = std::round(mk.x + (mk.width / 2.0) - (std_mk_px.cols / 2.0));
                mk.y = std::round(mk.y + (mk.width / 2.0) - (std_mk_px.rows / 2.0));
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
            auto x0 = std::round(grid_res.gl_x.front());
            auto y0 = std::round(grid_res.gl_y.front());
            auto x_d = std::round(grid_res.gl_x.back() - grid_res.gl_x.front());
            auto y_d = std::round(grid_res.gl_y.back() - grid_res.gl_y.front());
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
            // auto tiles = tiled_mat.get_tiles();

            // bgp
            auto inty_region = tiled_mat.get_image_roi();
            cv::Mat_<std::uint16_t> dmat = mat(inty_region);
            auto surf = cimp::bgb::bspline(dmat, {3, 6}, 0.3);
            if(!task.model().no_bgp()) {
                dmat.forEach([&surf](std::uint16_t& v, const int* pos){
                    const auto& r = pos[0];
                    const auto& c = pos[1];
                    v = std::round(std::max(0.0, v - surf(r, c))) + 1;
                });
            }
            auto margin_res = margin(
                task.model().method(),
                cimp::margin::Param<GridLineID> {
                    0.6,
                    &tiled_mat,
                    true,
                    fov_mod.pch_margin_view()
                }
            );
            set_bg(margin_res.stat_mats, tiled_mat, surf);
            fov_mod.set_tiled_mat(std::move(tiled_mat));
            fov_mod.set_stat_mats(std::move(margin_res.stat_mats));
            fov_mod.set_bg_means(cimp::utils::mat__to_vec(surf));

            f_grid_log["x0_px"] = std::round(grid_res.gl_x[0] * 100) / 100;
            f_grid_log["y0_px"] = std::round(grid_res.gl_y[0] * 100) / 100;
            f_grid_log["du_x"] = Utils::du_um(grid_res.gl_x, task.um2px_r());
            f_grid_log["du_y"] = Utils::du_um(grid_res.gl_y, task.um2px_r());
            f_grid_log["mks"]["pos_px"] = ranges::view::transform(mk_regs, [](const auto& mk_reg) {
                return std::vector<int>({mk_reg.x, mk_reg.y});
            });
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
    /**
     * @brief Functor doing image rotation.
     */
    __alias::crot::Calibrate                    rotate_calibrator       ;
    /**
     * @brief Functor doing probe channel marker detection.
     */
    __alias::cmk_det::RegMat                    probe_mk_detector       ;
    /**
     * @brief Functor doing marker detection based rotation degree detection.
     */
    __alias::crot::MarkerVec<float>             rotate_detector         ;
    /**
     * @brief Functor doing marker based gridding.
     */
    __alias::cimg::RegMat                       gridder                 ;
    /**
     * @brief Functor doing grid cell margin and cell statistics.
     */
    __alias::cimp::Margin<Float, GridLineID>    margin                  ;

} fov_ag;


}
