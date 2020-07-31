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
#include <ChipImgProc/marker/detection/estimate_bias.hpp>
#include <ChipImgProc/utils/mat_to_vec.hpp>
#include <ChipImgProc/warped_mat.hpp>
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
        auto& channel     = fov_mod.channel();
        auto& fov_id      = fov_mod.fov_id();
        auto& path        = fov_mod.src_path();
        auto  mat         = fov_mod.src().clone();
        auto& f_grid_log  = fov_mod.grid_log();
        auto& grid_bad    = fov_mod.proc_bad();
        auto& grid_done   = fov_mod.proc_done();
        auto& task        = fov_mod.channel().task();
        auto& wh_mk_pos   = task.fov_wh_mk_pos().at(fov_id);
        auto& wh_warp_mat = task.white_warp_mat().at(fov_id);
        auto log_prefix  = fmt::format("[{}-{}-({},{})]", 
            task.id().chip_id(), channel.ch_name(), fov_id.x, fov_id.y
        );
        summit::grid::log.trace(
            "{}: start auto gridding process", 
            log_prefix
        );
        try {
            // TODO: search best marker, currently use first as standard marker
            auto& std_mk = *(channel.sh_mk_pats()[0]);
            auto [templ, mask] = cmk::txt_to_img(
                std_mk.marker,
                std_mk.mask,
                task.cell_h_um() * task.um2px_r(),
                task.cell_w_um() * task.um2px_r(),
                task.space_um()  * task.um2px_r()
            );
            // TODO: case for no white marker
            auto [bias, score] = cmk_det::estimate_bias(mat, templ, mask, wh_mk_pos, wh_warp_mat);
            summit::grid::log.info("bias: ({}, {})", bias.x, bias.y);
            auto warp_mat = wh_warp_mat.clone();
            {
                auto _bias = bias;
                cimp::typed_mat(warp_mat, [&_bias](auto& mat){
                    mat(0, 2) += _bias.x;
                    mat(1, 2) += _bias.y;
                });
            }
            cv::Mat iwarp_mat;
            cv::Mat std_mat;
            cv::invertAffineTransform(warp_mat, iwarp_mat);
            cv::warpAffine(mat, std_mat, iwarp_mat, cv::Size(
                std::round(task.fov_w_rum()),
                std::round(task.fov_h_rum())
            ));
            std::vector<cmk_det::MKRegion> mk_regs;
            cv::Mat_<std::int16_t> mk_map(
                fov_mod.mk_num().y, 
                fov_mod.mk_num().x
            );
            for(int i = 0; i < fov_mod.mk_num().y; i ++) {
                for(int j = 0; j < fov_mod.mk_num().x; j ++) {
                    cmk_det::MKRegion mkr;
                    mkr.x = (j * task.mk_wd_rum()) + task.xi_rum() + 0.5;
                    mkr.y = (i * task.mk_hd_rum()) + task.yi_rum() + 0.5;
                    mkr.width  = task.mk_w_rum();
                    mkr.height = task.mk_h_rum();
                    mkr.x_i = j;
                    mkr.y_i = i;
                    mk_regs.emplace_back(std::move(mkr));
                    mk_map(i, j) = i * fov_mod.mk_num().x + j;
                }
            }
            auto final_mk_seg_view = fov_mod.final_mk_seg_view();
            if(final_mk_seg_view) {
                final_mk_seg_view(cmk::view(std_mat, mk_regs));
            }
            // gridding
            // TODO: pch_grid_view
            // auto grid_res = gridder(mat, mk_layout, mk_regs,
            //     nucleona::stream::null_out,
            //     fov_mod.pch_grid_view()
            // );

            // write raw result
            auto fov_raw_path = channel.fov_image("raw", fov_id.y, fov_id.x);
            cv::imwrite(fov_raw_path.string(), std_mat);

            if(task.model().marker_append()) {
                auto mk_append_res = cmk::roi_append(
                    std_mat, mk_map, mk_regs
                );
                fov_mod.set_mk_append(
                    std::move(mk_append_res)
                );
            }

            auto warped_mat = cimp::make_warped_mat(
                warp_mat, mat, {task.xi_rum(), task.yi_rum()},
                task.cell_w_rum(), task.cell_h_rum(),
                task.cell_wd_rum(), task.cell_hd_rum(),
                task.fov_w_rum(), task.fov_h_rum(),
                0.6, task.rum2px_r(),
                task.fov_w(), task.fov_h()
            );

            // // bgp
            // auto inty_region = tiled_mat.get_image_roi();
            // cv::Mat_<std::uint16_t> dmat = mat(inty_region);
            // auto surf = cimp::bgb::bspline(dmat, {3, 6}, 0.3);
            // if(!task.model().no_bgp()) {
            //     dmat.forEach([&surf](std::uint16_t& v, const int* pos){
            //         const auto& r = pos[0];
            //         const auto& c = pos[1];
            //         v = std::round(std::max(0.0, v - surf(r, c))) + 1;
            //     });
            // }
            // auto margin_res = margin(
            //     task.model().method(),
            //     cimp::margin::Param<GridLineID> {
            //         0.6,
            //         &tiled_mat,
            //         true,
            //         fov_mod.pch_margin_view()
            //     }
            // );
            // set_bg(margin_res.stat_mats, tiled_mat, surf);
            // fov_mod.set_tiled_mat(std::move(tiled_mat));
            // fov_mod.set_stat_mats(std::move(margin_res.stat_mats));
            // fov_mod.set_bg_means(cimp::utils::mat__to_vec(surf));

            // f_grid_log["x0_px"] = std::round(grid_res.gl_x[0] * 100) / 100;
            // f_grid_log["y0_px"] = std::round(grid_res.gl_y[0] * 100) / 100;
            // f_grid_log["du_x"] = Utils::du_um(grid_res.gl_x, task.um2px_r());
            // f_grid_log["du_y"] = Utils::du_um(grid_res.gl_y, task.um2px_r());
            // f_grid_log["mks"]["pos_px"] = ranges::view::transform(mk_regs, [](const auto& mk_reg) {
            //     return std::vector<int>({mk_reg.x, mk_reg.y});
            // });
            // f_grid_log["grid_bad"] = grid_bad;
            // f_grid_log["marker_region_source"] = fov_mod.mk_reg_src();
            // auto& fov_log_id = f_grid_log["id"];
            // fov_log_id = nlohmann::json::array();
            // fov_log_id[0] = fov_id.x;
            // fov_log_id[1] = fov_id.y;

            // grid_done = true;
        } catch(const std::exception& e) {
            // f_grid_log["grid_fail_reason"] = e.what();
            // grid_done = false;
            // grid_bad = true;
            // summit::grid::log.error(
            //     "channel: {}, FOV: ({},{}) process failed, reason: {}", 
            //     channel.ch_name(), fov_id.x, fov_id.y, e.what()
            // );
        }
        // f_grid_log["grid_done"] = grid_done;
        // f_grid_log["grid_bad"] = grid_bad;
        return grid_done;
    }
    /**
     * @brief Functor, image rotation
     */
    __alias::crot::Calibrate                    rotate_calibrator       ;
    /**
     * @brief Functor, probe channel marker detection
     */
    __alias::cmk_det::RegMat                    probe_mk_detector       ;
    /**
     * @brief Functor, marker detection based rotation degree detection
     */
    __alias::crot::MarkerVec<float>             rotate_detector         ;
    /**
     * @brief Functor, the marker based gridding
     */
    __alias::cimg::RegMat                       gridder                 ;
    /**
     * @brief Functor, the grid cell margin and cell statistics
     */
    __alias::cimp::Margin<Float, GridLineID>    margin                  ;

} fov_ag;


}
