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
#include <summit/app/grid/denoise_mk_append.hpp>
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
    auto draw_grid_line(
        const cv::Mat& std_mat, 
        const model::Task& task
    ) const {
        cv::Mat_<std::uint16_t> res = chipimgproc::viewable(std_mat);

        for(double x = task.xi_rum(); x <= std_mat.cols; x += task.cell_wd_rum()) {
            int _x = std::round(x);
            cv::line(
                res, 
                cv::Point(_x, 0),
                cv::Point(_x, std_mat.rows),
                cv::Scalar(65536/2)
            );
        } 
        for(double y = task.yi_rum(); y <= std_mat.rows; y += task.cell_hd_rum()) {
            int _y = std::round(y);
            cv::line(
                res, 
                cv::Point(0, _y),
                cv::Point(std_mat.rows, _y),
                cv::Scalar(65536/2)
            );
        } 
        return res;
    }
    // auto draw_margin(
    //     cv::Mat& std_mat, 
    //     const chipimgproc::WarpedMat<true, float>& warped_mat, 
    //     int clw, int clh, 
    //     int clwn, int clhn, 
    //     double seg_rate, 
    //     double um2px_r
    // ) const {
    //     cv::Mat_<std::uint16_t> margin_mat(chipimgproc::viewable(std_mat));
    //     cv::Mat iwarp_mat;
    //     int swin_w(std::round(clw * seg_rate * um2px_r));
    //     int swin_h(std::round(clh * seg_rate * um2px_r));
    //     clw = std::round(clw * um2px_r);
    //     clh = std::round(clh * um2px_r);
    //     cv::invertAffineTransform(warped_mat.warp_mat(), iwarp_mat);

    //     for (int i(0); i < clhn; ++i) {
    //         for (int j(0); j < clwn; ++j) {
    //             auto patch(warped_mat.make_at_result());
    //             warped_mat.at_cell(patch, i, j);
    //             cv::Point2d center(patch.img_p);
    //             // cv::Point2d pos(warped_mat.stat_mats_.min_cv_pos(i, j));
    //             // std::vector<cv::Point2d> src{pos}, dst(1);

    //             // src[0].x -= 0.5;
    //             // src[0].y -= 0.5;
    //             // cv::transform(src, dst, iwarp_mat);
    //             // pos = dst[0];
    //             // chipimgproc::warped_mat::Patch res;
    //             // warped_mat.at_cell(res, i, j);

    //             int swin_w_px(swin_w * um2px_r), swin_h_px(swin_h * um2px_r);
    //             cv::rectangle(
    //                 margin_mat, center, cv::Point2d(center.x + swin_w_px, center.y + swin_h_px), 65536 / 2
    //             );
    //         }
    //     }
    //     return margin_mat;

    //     // const int swin_w(std::round(cell_w * seg_rate)), swin_h(std::round(cell_h * seg_rate));
    //     // const int swin_size(swin_w * swin_h);
    //     // double mean, sd;
    //     // cv::Mat mean_mat, sd_mat;
    //     // for (int x(task.xi_rum() + 1); x <= grid_view.cols; x += cell_wd) {
    //     //     for (int y(task.xi_rum() + 1); y <= grid_view.rows; y += cell_hd) {
    //     //         //plus 1 on width and height to prevent cut rectangle border off
    //     //         cv::Mat cell(grid_view, cv::Rect2d(x, y, cell_w + 1, cell_h + 1));
    //     //         cv::Point2i min_pos(-1, -1);
    //     //         double min_cv(std::numeric_limits<double>::max());
    //     //         //minus 1 on width and height to get correct value
    //     //         for (int w(0); w < cell.cols - 1 - swin_w + 1; ++w) {
    //     //             for (int h(0); h < cell.rows - 1 - swin_h + 1 ; ++h) {
    //     //                 cv::Mat swin(cell, cv::Rect2d(w, h, swin_w, swin_h));
    //     //                 cv::meanStdDev(swin, mean_mat, sd_mat);

    //     //                 mean = mean_mat.at<double>(0, 0);
    //     //                 // mean = cv::sum(swin)[0] / swin_size;
    //     //                 sd = sd_mat.at<double>(0, 0);
    //     //                 if (min_cv > sd / mean) {
    //     //                     min_cv = sd / mean;
    //     //                     min_pos = {w, h};
    //     //                 }
    //     //             }
    //     //         }

    //     //         cv::rectangle(
    //     //             cell, cv::Point2d(min_pos.x, min_pos.y), 
    //     //             cv::Point2d(min_pos.x + swin_w, min_pos.y + swin_h), 65536/2
    //     //         );
    //     //     }
    //     // }
    // }
    /**
     * @brief Run FOV level gridding process.
     * 
     * @param fov_mod FOV parameter model.
     * @return true Gridding process is done.
     * @return false Gridding process is failed, the fail reason will store in grid log.
     */
    bool operator()(model::FOV& fov_mod) const {
        // std::cout << "start fov_ag\n";
        // auto tmp_timer(std::chrono::steady_clock::now());
        std::chrono::duration<double, std::milli> d;
        using namespace __alias;
        auto& channel       = fov_mod.channel();
        auto& fov_id        = fov_mod.fov_id();
        auto& path          = fov_mod.src_path();
        auto  mat           = fov_mod.src().clone();
        auto& f_grid_log    = fov_mod.grid_log();
        auto& grid_bad      = fov_mod.proc_bad();
        auto& grid_done     = fov_mod.proc_done();
        auto& task          = fov_mod.channel().task();
        // auto& wh_mk_pos    = task.fov_wh_mk_pos().at(fov_id);
        // auto& wh_warp_mat  = task.white_warp_mat().at(fov_id);
        auto& ref_from_pb   = task.ref_from_probe_ch(); 
        auto& ref_success   = task.fov_ref_ch_successes().at(fov_id);
        auto& ref_mk_pos    = task.fov_ref_ch_mk_pos().at(fov_id);
        auto& ref_warp_mat  = task.ref_ch_warp_mat().at(fov_id);
        auto& mk_pos_spec   = task.fov_mk_pos_spec().at(fov_id);
        auto log_prefix  = fmt::format("[{}-{}-({},{})]", 
            task.id().chip_id(), channel.ch_name(), fov_id.x, fov_id.y
        );
        summit::grid::log.info(
            "{}: start auto gridding process", 
            log_prefix
        );
        // d = std::chrono::steady_clock::now() - tmp_timer;
        // std::cout << "fov init: " << d.count() << " ms\n";
        try {
            // tmp_timer = std::chrono::steady_clock::now();
            summit::grid::log.info("marker pattern number: {}", channel.sh_mk_pats().size());
            // TODO: case for no white marker
            double res_score = 0.0;
            cv::Point2d res_bias;
            for(auto&& p_mk_pat : channel.sh_mk_pats()) {
                auto& mk_pat = *p_mk_pat;
                auto [templ, mask] = cmk::txt_to_img(
                    mk_pat.marker,
                    mk_pat.mask,
                    task.cell_h_um(),
                    task.cell_w_um(),
                    task.space_um(),
                    task.um2px_r()
                );
                
                if(!ref_success /* !ref_from_wh */){

                    // fluorescent process: Dealing with ref_warp_mat for first-staged finding marker failed.
                    
                }else{
                    if(task.est_bias()){
                        auto [bias, score] = cmk_det::estimate_bias(
                            mat, templ, mask, mk_pos_spec, ref_warp_mat, 
                            task.global_search(),
                            task.basic_cover_size(),
                            task.highP_cover_extend_r(),
                            task.est_bias_regulation(),
                            task.regu_cover_extend_r()
                        );
                        if(res_score < score) {
                            res_score = score;
                            res_bias = bias;
                        }
                    }else{
                        res_bias.x = 0;
                        res_bias.y = 0;
                    }
                }
            }
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "estimate_bias: " << d.count() << " ms\n";
            summit::grid::log.info("bias: ({}, {})", res_bias.x, res_bias.y);
            auto warp_mat = ref_warp_mat.clone();
            {
                auto _bias = res_bias;
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

            // write raw result
            auto fov_raw_path = channel.fov_image("raw", fov_id.y, fov_id.x);
            auto std2raw = task.std2raw_warp();
            cv::Mat raw_mat;
            cv::warpAffine(std_mat, raw_mat, std2raw, cv::Size(
                std::round(task.fov_w_rum()*task.rum2px_r()), 
                std::round(task.fov_h_rum()*task.rum2px_r())
            ));
            cv::imwrite(fov_raw_path.string(), raw_mat);
            
            std::vector<cmk_det::MKRegion> mk_regs;
            cv::Mat_<std::int16_t> mk_map(
                fov_mod.mk_num().y, 
                fov_mod.mk_num().x
            );
            for(int i = 0; i < fov_mod.mk_num().y; i ++) {
                for(int j = 0; j < fov_mod.mk_num().x; j ++) {
                    cmk_det::MKRegion mkr;
                    mkr.x = (j * task.mk_wd_rum()) + task.xi_rum();
                    mkr.y = (i * task.mk_hd_rum()) + task.yi_rum();
                    mkr.width  = task.mk_w_rum();
                    mkr.height = task.mk_h_rum();
                    mkr.x_i = j;
                    mkr.y_i = i;
                    mk_regs.emplace_back(std::move(mkr));
                    mk_map(i, j) = i * fov_mod.mk_num().x + j;
                }
            }

            if (task.model().debug() >= 5) {
                // rescale domain images
                auto fov_rescale_path = channel.fov_image("rescale", fov_id.y, fov_id.x);
                cv::imwrite(fov_rescale_path.string(), std_mat);
                // debug mk_seg image
                auto final_mk_seg_view = fov_mod.final_mk_seg_view();
                if(final_mk_seg_view) {
                    final_mk_seg_view(cmk::view(std_mat, mk_regs));
                }
                // debug grid image
                auto grid_view = draw_grid_line(std_mat, task);
                if(fov_mod.pch_grid_view()) {
                    fov_mod.pch_grid_view()(grid_view);
                }
            }

            if(task.model().marker_append()) {                
                auto mk_append_res = cmk::roi_append(
                    std_mat, mk_map, mk_regs
                );
                if(ref_from_pb) {
                    auto mk_append_denoised = denoise_mk_append(
                        mk_append_res,
                        task.bit_ms_wd_rum(), task.bit_ms_hd_rum(),
                        task.mk_w_rum(),      task.mk_h_rum(),
                        fov_mod.mk_num().y,   fov_mod.mk_num().x
                    );
                    fov_mod.set_mk_append_denoised(
                        std::move(mk_append_denoised)
                    );
                }
                fov_mod.set_mk_append(
                    std::move(mk_append_res)
                );
            }

            summit::grid::log.info("stats window size -- width: {} pxs, height: {} pxs", 
                                    std::round(task.cell_w_rum() * task.stat_window_size_r()), 
                                    std::round(task.cell_h_rum() * task.stat_window_size_r()));
            chipimgproc::ip_convert(mat, CV_32F);
            // tmp_timer = std::chrono::steady_clock::now();
            auto warped_mat = cimp::make_warped_mat(
                warp_mat, mat, {task.xi_rum(), task.yi_rum()},
                task.cell_w_rum(), task.cell_h_rum(),
                task.cell_wd_rum(), task.cell_hd_rum(),
                task.fov_w_rum(), task.fov_h_rum(),
                task.stat_window_size_r(), task.rum2px_r(),
                task.fov_w(), task.fov_h()
            );
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "make_warped_mat: " << d.count() << " ms\n";

            // if (task.model().debug() >= 5) {
            //     // draw min-cv margin
            //     auto mat_clone(fov_mod.src().clone());
            //     auto margin_mat = draw_margin(
            //         mat_clone, warped_mat, task.cell_w_rum(), task.cell_h_rum(), 
            //         task.fov_w(), task.fov_h(), 0.6, task.rum2px_r()
            //     );
            //     cv::warpAffine(margin_mat, margin_mat, iwarp_mat, cv::Size(
            //         std::round(task.fov_w_rum()),
            //         std::round(task.fov_h_rum())
            //     ));
            //     fov_mod.pch_margin_view()(margin_mat);
            // }

            // // TODO: bgp
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
            //         0.17, 
            //         &tiled_mat,
            //         true,
            //         fov_mod.pch_margin_view()
            //     }
            // );
            // set_bg(margin_res.stat_mats, tiled_mat, surf);
            // tmp_timer = std::chrono::steady_clock::now();
            summit::grid::log.warn("currently not handle the background");

            // fov_mod.set_tiled_mat(std::move(tiled_mat));
            // fov_mod.set_stat_mats(std::move(margin_res.stat_mats));
            // fov_mod.set_bg_means(cimp::utils::mat__to_vec(surf));
            fov_mod.set_warped_mat(std::move(warped_mat));
            fov_mod.set_std_img(std::move(std_mat));
            fov_mod.set_warp_mat(std::move(warp_mat));

            // f_grid_log["x0_px"] = std::round(grid_res.gl_x[0] * 100) / 100;
            // f_grid_log["y0_px"] = std::round(grid_res.gl_y[0] * 100) / 100;
            // f_grid_log["du_x"] = Utils::du_um(grid_res.gl_x, task.um2px_r());
            // f_grid_log["du_y"] = Utils::du_um(grid_res.gl_y, task.um2px_r());
            // f_grid_log["mks"]["pos_px"] = ranges::view::transform(mk_regs, [](const auto& mk_reg) {
            //     return std::vector<int>({mk_reg.x, mk_reg.y});
            // });
            // f_grid_log["grid_bad"] = grid_bad;
            // f_grid_log["marker_region_source"] = fov_mod.mk_reg_src();
            f_grid_log["warp_mat"] = fov_mod.warp_mat_vec();
            auto& fov_log_id = f_grid_log["id"];
            fov_log_id = nlohmann::json::array();
            fov_log_id[0] = fov_id.x;
            fov_log_id[1] = fov_id.y;

            grid_done = true;
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "fov statistic: " << d.count() << " ms\n";
        } catch(const std::exception& e) {
            f_grid_log["grid_fail_reason"] = e.what();
            grid_done = false;
            grid_bad = true;
            summit::grid::log.error(
                "channel: {}, FOV: ({},{}) process failed, reason: {}", 
                channel.ch_name(), fov_id.x, fov_id.y, e.what()
            );
            if (task.model().debug() >= 6) {
                std::cerr << "An exception thrown while processing fluor channel, and debug level is the highest, program exit.\n";
                std::exit(-1);
            }
        }
        f_grid_log["grid_done"] = grid_done;
        f_grid_log["grid_bad"] = grid_bad;

        // std::cout << "end fov_ag\n";
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
