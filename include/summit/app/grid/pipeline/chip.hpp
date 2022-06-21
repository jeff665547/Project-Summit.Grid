/**
 * @file chip.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::pipeline::Chip
 */
#pragma once
#include "channel.hpp"
#include <summit/app/grid/model.hpp>
#include <summit/app/grid/aruco_setter.hpp>
#include <summit/app/grid/custom_aruco_setter.hpp>
#include <summit/app/grid/probe_ch_proc_setter.hpp>
#include <summit/app/grid/general_marker_proc_setter.hpp>
#include <summit/app/grid/mk_detector_rescuer.hpp>
#include <summit/app/grid/model/marker_base.hpp>
#include <summit/app/grid/white_mk_append.hpp>
#include <summit/app/grid/denoise_mk_append.hpp>
#include <summit/app/grid/fov_mkid_rel.hpp>
#include <summit/grid/version.hpp>
#include <ChipImgProc/algo/um2px_auto_scale.hpp>
#include <ChipImgProc/rotation/marker_vec.hpp>
#include <ChipImgProc/rotation/iteration_cali.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/marker/reg_mat_um2px_r_det.hpp>
#include <ChipImgProc/marker/cell_layout.hpp>
#include <ChipImgProc/marker/detection/reg_mat_infer.hpp>
#include <Nucleona/range.hpp>
#include <Nucleona/util/remove_const.hpp>

#include <ChipImgProc/aruco/marker_map.hpp>
#include <ChipImgProc/marker/detection/aruco_random.hpp>
#include <ChipImgProc/warped_mat.hpp>
#include <ChipImgProc/rotation/from_warp_mat.hpp>
#include <ChipImgProc/warp_mat/from_warp_mat.hpp>
#include <ChipImgProc/warp_mat/to_warp_mat.hpp>
#include <ChipImgProc/warped_mat/estimate_transform_mat.hpp>

namespace summit::app::grid::pipeline {
namespace __alias {

namespace cimp      = chipimgproc;
namespace cmk       = chipimgproc::marker;
namespace crot      = chipimgproc::rotation;
namespace cmk_det   = chipimgproc::marker::detection;

}
/**
 * @brief Chip level process, evaluate chip-wide parameters
 *        (e.g. rotation degree and micron to pixel rate) and 
 *        collect chip-wide result.
 * @details The workflow diagram 
 *          @image html chip-level-process.png
 *          @image latex chip-level-process
 *          See @ref chip-level-process "Chip level process" for more details
 */
struct Chip {
    /**
     * @brief Construct a new Chip object
     */
    Chip() 
    : rotate_detector       () 
    , rotate_calibrator     () 
    , reg_mat_um2px_r_det   () 
    {}
    /**
     * @brief White channel image process, 
     *        evaluate rotation degree and micron to pixel rate.
     * @details The workflow diagram
     *          @image html bf-channel-image-process.png
     *          @image latex bf-channel-image-process
     * @param task chip parameter model.
     * @return true Process success
     * @return false Process failed, possible reason: 
     *  1. No white channel
     *  2. Chip type not support ArUco marker and no specific bright field marker found
     *  3. Bad image quality (process failed)
     */
    bool white_channel_proc(model::Task& task) const {
        namespace nr = nucleona::range;
        using namespace __alias;
        task.set_white_channel_imgs(Utils::read_white_channel(
            task.channels(),
            task.chip_dir(),
            task.fov_rows(),
            task.fov_cols(),
            task.is_img_enc(), 
            task.model()
        ));
        if(task.white_channel_imgs().size() == 0) return false;
        try {
            white_channel_proc_aruco(task);
            return true;
        } catch (...) {
            summit::grid::log.warn("aruco process failed");
            summit::grid::log.debug("start general marker process");
            return white_channel_proc_general(task);
        }
    }
    /**
     * @brief Bright-field ArUco marker process, a sub-part of white_channel_proc
     * 
     * @param task chip parameter model
     * @return true Proccess success
     * @return false Process failed
     *  1. Chip type not support ArUco marker
     *  2. Bad image quality
     */
    void white_channel_proc_aruco(model::Task& task) const {
        namespace nr = nucleona::range;
        using namespace __alias;
        using MKRegion = chipimgproc::marker::detection::MKRegion;

        bool rescue_proc = false;
        auto& executor   = task.model().executor();
        auto& model      = task.model();
        auto [id, wh_ch] = task.white_channel();
        auto wh_ch_name  = wh_ch["name"].get<std::string>();

        auto aruco_mk_detector = aruco_setter(task);
        
        auto& fov_marker_num = task.fov_marker_num();
        Utils::FOVMarkerRegionMap                 fov_marker_regs;
        Utils::FOVMap<int>                        fov_marker_unclear_ct;
        Utils::FOVMap<bool>                       fov_wh_warnings;
        Utils::FOVMap<bool>                       fov_wh_successes;
        Utils::FOVMap<float>                      fov_rot_degs;
        Utils::FOVMap<float>                      fov_mk_append_correlation;
        Utils::FOVMap<cv::Point2d>                fov_scales;
        Utils::FOVMap<cv::Mat>                    fov_mk_append;
        Utils::FOVMap<cv::Mat>                    fov_mk_append_dn;
        Utils::FOVMap<cv::Mat>                    fov_white_warp_mat;
        Utils::FOVMap<std::vector<cv::Point2d>>   fov_wh_mk_pos;
        Utils::FOVMap<std::vector<cv::Point2d>>   fov_mk_pos_spec;
        std::vector<cv::Mat>                      fov_rum_imgs(task.white_channel_imgs().size());

        chipimgproc::aruco::MarkerMap mk_map(task.id_map());

        auto [templ, mask] = chipimgproc::aruco::create_location_marker(
            task.tm_outer_width(), // task.tm_outer_width() - 2, // for erosion.
            task.tm_inner_width(), // task.tm_inner_width() + 2, // for erosion.
            task.tm_padding(),
            task.tm_margin(), 
            task.um2px_r()
        );
        
        auto fov_mk_rel = fov_mkid_rel(task.chipspec(), task.fov());
        for(auto&& [fov_id, mat] : task.white_channel_imgs()) {
            fov_marker_unclear_ct[fov_id] = {};
            fov_marker_regs[fov_id] = {};
            fov_wh_warnings[fov_id] = {};
            fov_wh_successes[fov_id] = {};
            fov_rot_degs[fov_id] = {};
            fov_mk_append_correlation[fov_id] = {};
            fov_scales[fov_id] = {};
            fov_mk_append[fov_id] = cv::Mat();
            fov_mk_append_dn[fov_id] = cv::Mat();
            fov_wh_mk_pos[fov_id] = {};
            fov_mk_pos_spec[fov_id] = {};
            fov_white_warp_mat[fov_id] = cv::Mat();
        }
        auto aruco_mk_det = [&, this](
            cv::Mat mat, 
            const cv::Point& fov_id
        ) {
            auto mk_regs = aruco_mk_detector(mat, mk_map.get_marker_indices());
            auto& mk_ids = fov_mk_rel.at(fov_id);
            std::vector<typename decltype(mk_regs)::value_type> tmp;
            for(auto mk : mk_regs) {
                auto& idx = std::get<0>(mk);
                if(idx < 0) continue;
                auto pos = mk_map.get_sub(idx);
                if(mk_ids.count(pos) > 0) {
                    tmp.push_back(mk);
                }
            }
            mk_regs.swap(tmp);
            summit::grid::log.debug("marker # = {}", mk_regs.size());
            return std::make_tuple(mk_regs, tmp);
        };

        auto& st_pts_cl = task.stitched_points_cl();

        summit::grid::log.debug("white channel image nums: {}", task.white_channel_imgs().size());

        task.white_channel_imgs()
        | nucleona::range::indexed()
        | ranges::view::transform([&](auto&& p){
            auto& i             = p.first;
            auto& fov_id_mat    = p.second;
            auto& fov_id        = fov_id_mat.first;
            auto& path          = std::get<0>(fov_id_mat.second);
            cv::Mat mat         = std::get<1>(fov_id_mat.second);
            summit::grid::log.debug("white channel, fov id:({}, {}) start process", fov_id.x, fov_id.y);
            try {
                auto& mk_num = fov_marker_num.at(fov_id);
                /* detect marker regions */
                auto [mk_pos_des, raw_findings] = aruco_mk_det(mat, fov_id);
                std::vector<cv::Point2d>    mk_pos_px       ;
                std::vector<cv::Point2d>    mk_pos_rum      ;
                auto& st_p_cl = st_pts_cl.at(fov_id);
                cv::Point2d st_p(st_p_cl.x * task.cell_wd_rum(), st_p_cl.y * task.cell_hd_rum());
                for(auto& [aid, score, pos] : mk_pos_des) {
                    if(aid == -1) continue;
                    auto mkpid   = mk_map.get_sub(aid);
                    auto mk_lt_x = (mkpid.x * task.mk_wd_rum()) + task.xi_rum() - st_p.x;
                    auto mk_lt_y = (mkpid.y * task.mk_hd_rum()) + task.yi_rum() - st_p.y;
                    auto mk_x    = mk_lt_x + (task.mk_w_rum() / 2);
                    auto mk_y    = mk_lt_y + (task.mk_h_rum() / 2);
                    // summit::grid::log.debug("mk_pos_rum : ({}, {})", mk_x,  mk_y );
                    // summit::grid::log.debug("mk_pos_px  : ({}, {})", pos.x, pos.y);
                    mk_pos_px.push_back(pos);
                    mk_pos_rum.emplace_back(mk_x, mk_y);
                }
                auto mk_num_match_spec = mk_pos_px.size() == mk_num.x * mk_num.y;
                if(!mk_num_match_spec) {
                    auto recognized_count = mk_pos_px.size();
                    int remaining_marker_ct = mk_num.x * mk_num.y - recognized_count;
                    std::vector<cv::Point2d>  init_mk_pos_px;
                    for(auto mk : mk_pos_des) {
                        auto& pos_px = std::get<2>(mk);
                        init_mk_pos_px.push_back(pos_px);
                    }
                    summit::grid::log.debug("Start second-round recognition.");
                    // std::cout << "New recognized process" << std::endl;
                    auto remaining_mk_detector = custom_aruco_setter(task, remaining_marker_ct);
                    auto img = custom_aruco_setter.draw_mk_pos(mat, templ, init_mk_pos_px);
                    try {
                        mk_pos_des = remaining_mk_detector(img, mk_map.get_marker_indices());
                        size_t ct = 0;
                        for(auto& [aid, score, pos] : mk_pos_des) {
                            if(aid < 0) continue;
                            auto mkpid   = mk_map.get_sub(aid);
                            auto mk_lt_x = (mkpid.x * task.mk_wd_rum()) + task.xi_rum() - st_p.x;
                            auto mk_lt_y = (mkpid.y * task.mk_hd_rum()) + task.yi_rum() - st_p.y;
                            auto mk_x    = mk_lt_x + (task.mk_w_rum() / 2);
                            auto mk_y    = mk_lt_y + (task.mk_h_rum() / 2);
                            summit::grid::log.debug("Second-round mk_pos_rum : ({}, {})", mk_x,  mk_y );
                            // summit::grid::log.debug("Second-round mk_pos_px  : ({}, {})", pos.x, pos.y);
                            mk_pos_px.push_back(pos);
                            mk_pos_rum.emplace_back(mk_x, mk_y);
                            ct++;
                        }
                        summit::grid::log.debug("Second-round recognized marker # = {}", ct);
                        summit::grid::log.debug("Second-round recognition finish.");
                    } catch (...) {
                        summit::grid::log.debug("Second-round recognition failed.");
                    }
                    mk_num_match_spec = mk_pos_px.size() == mk_num.x * mk_num.y;
                    if(!mk_num_match_spec){
                        recognized_count = mk_pos_px.size();
                        remaining_marker_ct = mk_num.x * mk_num.y - recognized_count;
                        fov_wh_warnings.at(fov_id) = true;
                        fov_marker_unclear_ct.at(fov_id) = remaining_marker_ct;
                        if(recognized_count <= 1) {
                            throw std::runtime_error("Recognized markers are not enough!");
                        }
                    }
                }
                auto aruco_ch_mk_seg_view = task.aruco_ch_mk_seg_view(fov_id.y, fov_id.x);
                if(aruco_ch_mk_seg_view) {
                    std::vector<cv::Point> tmp(mk_pos_px.begin(), mk_pos_px.end());
                    aruco_ch_mk_seg_view(chipimgproc::marker::view(mat, tmp));
                } 
                auto warp_mat = chipimgproc::warped_mat::estimate_transform_mat(mk_pos_rum, mk_pos_px);

                // auto warped_mat = chipimgproc::make_basic_warped_mat(
                //     warp_mat, {mat}, {task.xi_rum(), task.yi_rum()}, 
                //     task.cell_wd_rum(), task.cell_hd_rum(), 
                //     task.fov_w_rum(), task.fov_h_rum() 
                // );
                
                /* count theta */
                fov_rot_degs.at(fov_id) = chipimgproc::warp_mat::rot_deg_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), theta: {}", fov_id.x, fov_id.y, fov_rot_degs.at(fov_id));
                auto slope = chipimgproc::warp_mat::slope_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), slope: {}", fov_id.x, fov_id.y, slope);
                fov_scales.at(fov_id) = chipimgproc::warp_mat::scale_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), x scale: {}, y scale: {}", 
                fov_id.x, fov_id.y, fov_scales.at(fov_id).x, fov_scales.at(fov_id).y);

                cv::Mat iwarp_mat;
                cv::Mat std_mat;
                cv::invertAffineTransform(warp_mat, iwarp_mat);
                cv::warpAffine(mat, std_mat, iwarp_mat, cv::Size(
                    std::round(task.fov_w_rum()), 
                    std::round(task.fov_h_rum())
                ));
                fov_rum_imgs[fov_id.x + (fov_id.y * task.fov_cols())] = std_mat;
                auto [fov_wh_mk_append, mk_regs] = white_mk_append(
                    std_mat, 
                    task.xi_rum(), task.yi_rum(), 
                    task.mk_w_rum(), task.mk_h_rum(),
                    task.mk_wd_rum(), task.mk_hd_rum(), 
                    mk_num.y, mk_num.x
                );
                summit::grid::log.debug("white channel, fov id:({}, {}) marker append done", fov_id.x, fov_id.y);
                auto fov_mk_append_denoised = denoise_mk_append(
                    fov_wh_mk_append,
                    task.bit_ms_wd_rum(), task.bit_ms_hd_rum(),
                    task.mk_w_rum(),      task.mk_h_rum(),
                    mk_num.y,             mk_num.x
                );
                fov_white_warp_mat.at(fov_id) = warp_mat;
                if(model.marker_append()) {
                    fov_mk_append.at(fov_id) = fov_wh_mk_append;
                    fov_mk_append_dn.at(fov_id) = fov_mk_append_denoised;
                }
                fov_marker_regs.at(fov_id)  = std::move(mk_regs);
                fov_wh_mk_pos.at(fov_id)    = std::move(mk_pos_px);
                fov_mk_pos_spec.at(fov_id)  = std::move(mk_pos_rum);
                fov_wh_successes.at(fov_id) = true;
            } catch (...) {
                summit::grid::log.error(
                    "white channel, fov id:({}, {}) process failed", 
                    fov_id.x, fov_id.y
                );
                fov_wh_successes.at(fov_id) = false;
                rescue_proc = true;
            }
            summit::grid::log.debug("white channel, fov id:({}, {}) end process", fov_id.x, fov_id.y);
            return 0;
        })
        | nucleona::range::p_endp(executor)
        ;

        summit::grid::log.debug("white channel FOVs process done");

        std::size_t success_num = 0;
        for(auto f = fov_wh_successes.begin(); f != fov_wh_successes.end(); f++) {
            if(f->second) success_num ++;
        }
        if(success_num == 0) {
            debug_throw(std::runtime_error("white marker process failed"));
        } 


        /* consensus */
        auto rot_degs = fov_wh_successes 
            | ranges::view::filter([&](auto&& p ){ return p.second; })
            | nr::transformed([&](auto&& p){ return fov_rot_degs.at(p.first); })
            | ranges::to_vector
        ;
        auto rot_deg = Utils::mean(rot_degs);
        task.set_rot_degree(rot_deg);        

        /* Rescuing Process (Activated only when some FOV images unsuccessfully estimate the warp matrix from ArUco markers.) */
        if (rescue_proc && success_num) {
            summit::grid::log.debug("activating the rescuing process...");
            summit::grid::log.debug("[Rescuing Process] collecting parameter information from successful FOV images.");

            // Collect parameters from successful FOV images.
            cv::Point2d scale, sum_fov_scale{0, 0}, tmp, translate, translate_max, translate_min, sum_fov_translate{0, 0};
            std::vector<cv::Point2d> ref_mk_pos_spec;
            double ct = success_num;
            double max_dev;
            bool initial = true;
            for(auto f = fov_wh_successes.begin(); f != fov_wh_successes.end(); f++) {
                // std::cout << f->first << std::endl;
                if (f->second) {
                    ref_mk_pos_spec    = fov_mk_pos_spec.at(f->first);
                    sum_fov_scale     += fov_scales.at(f->first);
                    translate.x        = fov_white_warp_mat.at(f->first).at<double>(0, 2);
                    translate.y        = fov_white_warp_mat.at(f->first).at<double>(1, 2);
                    sum_fov_translate += translate;
                    
                    if(initial){
                        translate_max = translate_min = translate;
                        initial = false;
                    }else{
                        // Pick the maximum deviation translation
                        if (translate.x > translate_max.x) translate_max.x = translate.x;
                        if (translate.y > translate_max.y) translate_max.y = translate.y;
                        // Pick the minimum deviation translation
                        if (translate.x < translate_min.x) translate_min.x = translate.x;
                        if (translate.y < translate_min.y) translate_min.y = translate.y;
                    }
                }
            }
            scale     = sum_fov_scale / ct;
            translate = sum_fov_translate / ct;
            max_dev   = std::max(std::max((translate_max - translate).x, (translate_max - translate).y), 
                                 std::max((translate - translate_min).x, (translate - translate_min).y));
            
            auto  ref_warp_mat = chipimgproc::warp_mat::create_warp(rot_deg, scale.x, scale.y, translate.x, translate.y);

            double cover      = std::max(3.0 * max_dev, 350.0); // pixels
            double cover_r    = cover / templ.cols; // ratio
            
            // Show collected parameters from successful FOV images.
            summit::grid::log.debug("[Rescuing Process] poor image nums: {}", ct);
            summit::grid::log.debug("[Rescuing Process] rotation degs: {}", rot_deg);
            summit::grid::log.debug("[Rescuing Process] scale x: {}, y: {}", scale.x, scale.y);
            summit::grid::log.debug("[Rescuing Process] translation: x: {}, y: {}", translate.x, translate.y);
            summit::grid::log.debug("[Rescuing Process] cover ratio: {}", cover_r);

            // std::cout << ct << std::endl;
            // std::cout << rot_deg << std::endl;
            // std::cout << scale << std::endl;
            // std::cout << translate << std::endl;
            // std::cout << max_dev << std::endl;
            // std::cout << ref_warp_mat << std::endl;

            fov_wh_successes
            | nucleona::range::indexed()
            | ranges::view::filter([&](auto&& p ){ return !p.second.second; })
            | ranges::view::transform([&](auto&& p){
                auto& i           = p.first;
                auto& fov_id_succ = p.second;
                auto& fov_id      = fov_id_succ.first;
                auto& path_mat    = task.white_channel_imgs().at(fov_id);
                auto& path        = std::get<0>(path_mat);
                cv::Mat mat       = std::get<1>(path_mat);

                summit::grid::log.debug("[Rescuing Process] fov id:({}, {}), start the white channel rescuing process.", fov_id.x, fov_id.y);

                auto& mk_num = fov_marker_num.at(fov_id);

                std::vector<cv::Point2d> mk_pos_rum = ref_mk_pos_spec;
                // Estimate the warp matrix from the bad FOV image with the use of the referenced information (from the above Info).
                auto [warp_mat, score] = mk_detector_rescuer(mat, templ, mask, ref_mk_pos_spec, ref_warp_mat, cover_r);

                /* count theta */
                // fov_rot_degs.at(fov_id) = chipimgproc::warp_mat::rot_deg_from_warp(warp_mat);
                // summit::grid::log.debug("[Rescuing Process] white channel, fov id:({}, {}) theta: {}.", fov_id.x, fov_id.y, fov_rot_degs.at(fov_id));

                // The following procedure is the same as the above one.
                cv::Mat iwarp_mat;
                cv::Mat std_mat;
                cv::invertAffineTransform(warp_mat, iwarp_mat);
                cv::warpAffine(mat, std_mat, iwarp_mat, cv::Size(
                    std::round(task.fov_w_rum()), 
                    std::round(task.fov_h_rum())
                ));
                fov_rum_imgs[fov_id.x + (fov_id.y * task.fov_cols())] = std_mat;
                auto [fov_wh_mk_append, mk_regs] = white_mk_append(
                    std_mat, 
                    task.xi_rum(), task.yi_rum(), 
                    task.mk_w_rum(), task.mk_h_rum(),
                    task.mk_wd_rum(), task.mk_hd_rum(), 
                    mk_num.y, mk_num.x
                );
                summit::grid::log.debug("[Rescuing Process] white channel, fov id:({}, {}) marker append done.", fov_id.x, fov_id.y);
                auto fov_mk_append_denoised = denoise_mk_append(
                    fov_wh_mk_append,
                    task.bit_ms_wd_rum(), task.bit_ms_hd_rum(),
                    task.mk_w_rum(),      task.mk_h_rum(),
                    mk_num.y,             mk_num.x
                );
                fov_white_warp_mat.at(fov_id) = warp_mat;
                if(model.marker_append()) {
                    fov_mk_append.at(fov_id) = fov_wh_mk_append;
                    fov_mk_append_dn.at(fov_id) = fov_mk_append_denoised;
                }
                fov_marker_regs.at(fov_id)  = std::move(mk_regs);
                // fov_wh_mk_pos.at(fov_id)    = std::move(mk_pos_px);
                fov_mk_pos_spec.at(fov_id)  = std::move(mk_pos_rum);

                if(score > 0) fov_wh_successes.at(fov_id) = true;
                summit::grid::log.debug("[Rescuing Process] white channel, fov id:({}, {}), rescue matching score: {}.", fov_id.x, fov_id.y, score);

                return 0;
            })
            | nucleona::range::p_endp(executor)
            ;
            summit::grid::log.debug("[Rescuing Process] set the rescue parameter for other channels' image gridding.");
            task.set_ref_ch_rescue_r(std::move(cover_r));
            summit::grid::log.debug("[Rescuing Process] the white channel rescuing process done.");
        }


        task.set_ref_from_white_ch(true);
        task.set_ref_ch_warp_mat(std::move(fov_white_warp_mat));
        task.set_fov_ref_ch_mk_pos(std::move(fov_wh_mk_pos));
        task.set_fov_ref_ch_successes(std::move(fov_wh_successes));
        // task.set_white_warp_mat(std::move(fov_white_warp_mat));
        // task.set_fov_wh_mk_pos(std::move(fov_wh_mk_pos));
        task.set_fov_mk_pos_spec(std::move(fov_mk_pos_spec));
        task.set_fov_mk_regs(std::move(fov_marker_regs));
        task.collect_fovs_warnings(fov_wh_warnings, task.warn());
        task.collect_fovs_marker_unclear_ct(fov_marker_unclear_ct);
        if(model.marker_append()) {
            task.collect_fovs_mk_append(fov_mk_append);
            cv::Point center_id(task.fov_rows()/2, task.fov_cols()/2);
            task.check_fovs_mk_append(
                fov_mk_append_dn,
                fov_mk_append_correlation,
                center_id,
                task.wh_mk_append_eval(),
                task.warn()
            );
            task.collect_fovs_mk_append_correlation(
                fov_mk_append_correlation, 
                center_id,
                wh_ch_name,
                task.qc_log()
            );
            if(task.compute_sharpness()) {
                task.get_fovs_mk_append_statistics(fov_mk_append);
            }
        }
        auto stitched_img = chipimgproc::stitch::add(
            fov_rum_imgs, task.stitched_points_rum()
        );
        model::GLRawImg stitched_grid_img(
            stitched_img, task.gl_x_rum(), task.gl_y_rum()
        );
        task.set_stitched_img(wh_ch_name, std::move(stitched_grid_img));
    }
    /**
     * @brief Bright-field general marker process, a sub-part of white_channel_proc
     * 
     * @param task chip parameter model
     * @return true Proccess success
     * @return false Process failed
     *  1. Chip type not support general marker
     *  2. Bad image quality
     */
    bool white_channel_proc_general(model::Task& task) const {
        
        namespace nr = nucleona::range;
        using namespace __alias;

        auto& executor        = task.model().executor();
        auto& model           = task.model();
        auto [id, wh_ch]      = task.white_channel();
        auto wh_ch_name       = wh_ch["name"].get<std::string>();

        auto gen_mk_detector  = gen_mk_proc_setter(task);

        auto& fov_marker_num  = task.fov_marker_num();
        Utils::FOVMarkerRegionMap                fov_marker_regs;
        Utils::FOVMap<bool>                      fov_wh_warnings;
        Utils::FOVMap<bool>                      fov_wh_successes;
        Utils::FOVMap<float>                     fov_rot_degs;
        Utils::FOVMap<float>                     fov_mk_append_correlation;
        Utils::FOVMap<cv::Point2d>               fov_scales;
        Utils::FOVMap<cv::Mat>                   fov_mk_append;
        Utils::FOVMap<cv::Mat>                   fov_mk_append_dn;
        Utils::FOVMap<cv::Mat>                   fov_white_warp_mat;
        Utils::FOVMap<std::vector<cv::Point2d>>  fov_wh_mk_pos;
        Utils::FOVMap<std::vector<cv::Point2d>>  fov_mk_pos_spec;
        std::vector<cv::Mat>                     fov_rum_imgs(task.white_channel_imgs().size());
        
        // Initialization
        for(auto&& [fov_id, mat] : task.white_channel_imgs()) {
            fov_marker_regs[fov_id] = {};
            fov_wh_warnings[fov_id] = {};
            fov_wh_successes[fov_id] = {};
            fov_rot_degs[fov_id] = {};
            fov_mk_append_correlation[fov_id] = {};
            fov_scales[fov_id] = {};
            fov_mk_append[fov_id] = cv::Mat();
            fov_mk_append_dn[fov_id] = cv::Mat();
            fov_wh_mk_pos[fov_id] = {};
            fov_mk_pos_spec[fov_id] = {};
            fov_white_warp_mat[fov_id] = cv::Mat();
        }
        auto gen_mk_det = [&, this](
            cv::Mat mat
        ) {
            auto mk_regs = gen_mk_detector(mat);
            summit::grid::log.debug("general marker # = {}", mk_regs.size());
            return mk_regs;
        };

        summit::grid::log.debug("white channel images nums: {}", task.white_channel_imgs().size());

        /* Start to find markers */
        task.white_channel_imgs()
        | nucleona::range::indexed()
        | ranges::view::transform([&](auto&& p){
            auto& i          = p.first;
            auto& fov_id_mat = p.second;
            auto& fov_id     = fov_id_mat.first;
            auto& path       = std::get<0>(fov_id_mat.second);
            cv::Mat mat      = std::get<1>(fov_id_mat.second);
            summit::grid::log.debug("white channel, fov id:({}, {}) start process", fov_id.x, fov_id.y);
            try {
                auto& mk_num = fov_marker_num.at(fov_id);
                /* Detect general markers position */
                auto mk_pos_des = gen_mk_det(mat);
                std::vector<cv::Point2d>   mk_pos_px  ;
                std::vector<cv::Point2d>   mk_pos_rum ;

                for(auto& [fov_mk_id, score, pos] : mk_pos_des) {
                    auto mk_lt_x  = (fov_mk_id.x * task.mk_wd_rum()) + task.xi_rum();
                    auto mk_lt_y  = (fov_mk_id.y * task.mk_hd_rum()) + task.yi_rum();
                    auto mk_cen_x = mk_lt_x + (task.mk_w_rum() / 2);
                    auto mk_cen_y = mk_lt_y + (task.mk_h_rum() / 2);
                    mk_pos_rum.emplace_back(mk_cen_x, mk_cen_y);
                    mk_pos_px.push_back(pos);
                    // summit::grid::log.debug("mk_cen : ({}, {})", mk_cen_x, mk_cen_y);
                    // summit::grid::log.debug("pos    : ({}, {})", pos.x, pos.y);
                }
                auto mk_num_match_spec = mk_pos_px.size() == mk_num.x * mk_num.y;
                if(!mk_num_match_spec) {
                    fov_wh_warnings.at(fov_id) = true;
                    // task.create_warning_file();
                }
                auto warp_mat = chipimgproc::warped_mat::estimate_transform_mat(mk_pos_rum, mk_pos_px);

                /* Summary */
                fov_rot_degs.at(fov_id) = chipimgproc::warp_mat::rot_deg_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), theta: {}", fov_id.x, fov_id.y, fov_rot_degs.at(fov_id));
                auto slope = chipimgproc::warp_mat::slope_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), slope: {}", fov_id.x, fov_id.y, slope);
                fov_scales.at(fov_id) = chipimgproc::warp_mat::scale_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), x scale: {}, y scale: {}", 
                fov_id.x, fov_id.y, fov_scales.at(fov_id).x, fov_scales.at(fov_id).y);

                cv::Mat iwarp_mat;
                cv::Mat std_mat;
                cv::invertAffineTransform(warp_mat, iwarp_mat);
                cv::warpAffine(mat, std_mat, iwarp_mat, cv::Size(
                    std::round(task.fov_w_rum()),
                    std::round(task.fov_h_rum())
                ));
                fov_rum_imgs[fov_id.x + (fov_id.y * task.fov_cols())] = std_mat;
                auto [fov_wh_mk_append, mk_regs] = white_mk_append(
                    std_mat,
                    task.xi_rum(), task.yi_rum(),
                    task.mk_w_rum(), task.mk_h_rum(),
                    task.mk_wd_rum(), task.mk_hd_rum(),
                    mk_num.y, mk_num.x
                );
                summit::grid::log.debug("white channel, fov id:({}, {}) marker append done", fov_id.x, fov_id.y);
                auto fov_mk_append_denoised = denoise_mk_append(
                    fov_wh_mk_append,
                    task.bit_ms_wd_rum(), task.bit_ms_hd_rum(),
                    task.mk_w_rum(),      task.mk_h_rum(),
                    mk_num.y,             mk_num.x
                );
                fov_white_warp_mat.at(fov_id) = warp_mat;
                if(model.marker_append()) {
                    fov_mk_append.at(fov_id) = fov_wh_mk_append;
                    fov_mk_append_dn.at(fov_id) = fov_mk_append_denoised;
                }
                fov_marker_regs.at(fov_id)  = std::move(mk_regs);
                fov_wh_mk_pos.at(fov_id)    = std::move(mk_pos_px);
                fov_mk_pos_spec.at(fov_id)  = std::move(mk_pos_rum);
                fov_wh_successes.at(fov_id) = true;
            } catch (...) {
                summit::grid::log.error(
                    "white channel, fov id:({}, {}) general markers detection process failed",
                    fov_id.x, fov_id.y
                );
                fov_wh_successes.at(fov_id) = false;
            }
            summit::grid::log.debug("white channel, fov id:({}, {}) end process", fov_id.x, fov_id.y);
            return 0;
        })
        | nucleona::range::p_endp(executor)
        ;

        summit::grid::log.debug("white channel general markers FOVs process done");

        std::size_t success_num = 0;
        for(auto f = fov_wh_successes.begin(); f != fov_wh_successes.end(); f++) {
            if(f->second) success_num++;
        }
        if(success_num == 0) {
            debug_throw(std::runtime_error("white channel general markers detection process failed"));
        }

        auto stitched_img = chipimgproc::stitch::add(
            fov_rum_imgs, task.stitched_points_rum()
        );

        /* consensus */
        auto rot_degs = fov_wh_successes
            | ranges::view::filter([&](auto&& p ){ return p.second; })
            | nr::transformed([&](auto&& p){ return fov_rot_degs.at(p.first); })
            | ranges::to_vector
        ;
        auto rot_deg = Utils::mean(rot_degs);
        task.set_ref_from_white_ch(true);
        task.set_ref_ch_warp_mat(std::move(fov_white_warp_mat));
        task.set_fov_ref_ch_mk_pos(std::move(fov_wh_mk_pos));
        task.set_fov_ref_ch_successes(std::move(fov_wh_successes));
        task.set_fov_mk_pos_spec(std::move(fov_mk_pos_spec));
        task.set_rot_degree(rot_deg);
        task.set_fov_mk_regs(std::move(fov_marker_regs));
        task.collect_fovs_warnings(fov_wh_warnings, task.warn());
        if(model.marker_append()) {
            task.collect_fovs_mk_append(fov_mk_append);
            cv::Point center_id(task.fov_rows()/2, task.fov_cols()/2);
            task.check_fovs_mk_append(
                fov_mk_append_dn,
                fov_mk_append_correlation,
                center_id,
                task.wh_mk_append_eval(),
                task.warn()
            );
            task.collect_fovs_mk_append_correlation(
                fov_mk_append_correlation, 
                center_id,
                wh_ch_name,
                task.qc_log()
            );
            if(task.compute_sharpness()) {
                task.get_fovs_mk_append_statistics(fov_mk_append);
            }            
        }
        model::GLRawImg stitched_grid_img(
            stitched_img, task.gl_x_rum(), task.gl_y_rum()
        );
        task.set_stitched_img(wh_ch_name, std::move(stitched_grid_img));

    // =================================================================================================

    //     cmk_det::RegMat mk_detector;
    //     int  sel_fov_row         = task.fov_rows() / 2;
    //     int  sel_fov_col         = task.fov_cols() / 2;
    //     auto [ch_i, ch]          = task.white_channel();
    //     auto [img_path, mat]     = task.white_channel_imgs().at(cv::Point(sel_fov_col, sel_fov_row));
    //     auto  mks                = task.get_marker_patterns(
    //                                 "filter", 0
    //                                );
    //     if(mks.empty()) return false;
    //     auto& mk                 = mks.at(0);
    //     auto& marker             = mk->marker;
    //     auto& mask               = mk->mask;
    //     auto& fov_marker_num     = task.get_fov_marker_num(sel_fov_row, sel_fov_col);
    //     auto mk_layout           = make_marker_layout_from_raw_img(
    //         marker, mask,
    //         mk->meta.at("w_um").get<int>(),
    //         mk->meta.at("h_um").get<int>(),
    //         task.cell_h_um(), task.cell_w_um(),
    //         task.space_um(),
    //         fov_marker_num.y,
    //         fov_marker_num.x,
    //         task.mk_wd_cl(),
    //         task.mk_hd_cl(),
    //         task.um2px_r()
    //     );
    //     std::vector<cv::Point>   low_score_marker_idx;
    //     auto mk_rot_cali = crot::make_iteration_cali(
    //         [&, this](const cv::Mat& mat) {
    //             auto mk_regs = mk_detector(
    //                 static_cast<const cv::Mat_<std::uint8_t>&>(mat), 
    //                 mk_layout, 
    //                 __alias::cimp::MatUnit::PX, 0,
    //                 nucleona::stream::null_out
    //             );
    //             low_score_marker_idx = cmk_det::filter_low_score_marker(mk_regs);
    //             auto theta      = rotate_detector(mk_regs, nucleona::stream::null_out);
    //             return theta;
    //         },
    //         [&, this](cv::Mat& mat, auto theta) {
    //             rotate_calibrator(mat, theta /*debug viewer*/);
    //         }
    //     );
    //     // * marker detection and rotate
    //     auto theta      = mk_rot_cali(mat);
    //     // std::cout << "probe channel detect theta: " << theta << std::endl;
    //     cv::Mat mat_loc = mat.clone();
    //     rotate_calibrator(mat_loc, theta);

    //     // * um2px_r auto scaler
    //     auto [best_um2px_r, score_mat, best_mk_layout] = um_to_px_autoscale(mat_loc, marker, mask,
    //         mk->meta.at("w_um").get<int>(),
    //         mk->meta.at("h_um").get<int>(),
    //         task.cell_h_um(), task.cell_w_um(),
    //         task.space_um(),
    //         fov_marker_num.y,
    //         fov_marker_num.x,
    //         task.mk_wd_cl(),
    //         task.mk_hd_cl(),
    //         task.um2px_r(), 0.002, 7, low_score_marker_idx
    //     );
    //     task.set_rot_degree(theta);
    //     task.set_um2px_r(best_um2px_r);
        return true;
    }
    /**
     * @brief Probe channel image process, 
     *        similar to white_channel_proc but use probe marker detection.
     * @param task chip parameter model.
     * @return true Process finished normally.
     * @return false Current implementation won't return false.
     */
    bool probe_channel_proc(model::Task& task) const {

        namespace nr = nucleona::range;
        using namespace __alias;

        // * Prepare parameters for reading fluorescent images.
        // TODO: Sample some FOVs to get the rough information of general marker position and warp matrix.
        //       Find the actual warp matrix and marker position for each FOV at second stage (fov_ag.hpp).
        // int sampled_fov_row   = task.fov_rows() / 2; 
        // int sampled_fov_col   = task.fov_cols() / 2;
        // auto [ch_i, ch]       = task.first_probe_channel();
        // auto pb_ch_name       = ch["name"].get<std::string>();
        // std::vector<cv::Point> fov_ids{{sampled_fov_col, sampled_fov_row}, {0, 0}};
        // for-loop  Utils::read_img;

        // * Choose the first probe channel and read all fluorescent images.
        task.set_probe_channel_imgs(Utils::read_first_probe_channel(
            task.channels(),
            task.chip_dir(),
            task.fov_rows(),
            task.fov_cols(),
            task.is_img_enc(), 
            task.model()
        ));

        if(task.probe_channel_imgs().size() == 0) {
            summit::grid::log.warn("There are no available probe channel (fluorescent) images.");
            return false;
        }

        auto&  executor        = task.model().executor();
        auto&  model           = task.model();
        auto   [ch_i, ch]      = task.first_probe_channel();
        
        auto probe_mk_detector = pb_ch_proc_setter(task, ch);

        auto& fov_marker_num   = task.fov_marker_num();
        Utils::FOVMarkerRegionMap                   fov_marker_regs;
        Utils::FOVMap<bool>                         fov_pb_warnings;
        Utils::FOVMap<bool>                         fov_pb_successes;
        Utils::FOVMap<float>                        fov_rot_degs;
        Utils::FOVMap<cv::Point2d>                  fov_scales;
        // Utils::FOVMap<cv::Mat>                      fov_mk_append;
        Utils::FOVMap<cv::Mat>                      fov_probe_warp_mat;
        Utils::FOVMap<std::vector<cv::Point2d>>     fov_pb_mk_pos;
        Utils::FOVMap<std::vector<cv::Point2d>>     fov_mk_pos_spec;
        std::vector<cv::Mat>                        fov_rum_imgs(task.probe_channel_imgs().size());

        // cv::Mat probe_img_template = Utils::get_fov_mat_from(task.probe_channel_imgs(), 0, 0);
        // cv::imwrite("C:/grid_res/img_template.tiff", probe_img_template);
    

        // Initialization
        for(auto&& [fov_id, mat] : task.probe_channel_imgs()) {
            fov_marker_regs[fov_id] = {};
            fov_pb_warnings[fov_id] = {};
            fov_pb_successes[fov_id] = {};
            fov_rot_degs[fov_id] = {};
            fov_scales[fov_id] = {};
            // fov_mk_append[fov_id] = cv::Mat();
            fov_pb_mk_pos[fov_id] = {};
            fov_mk_pos_spec[fov_id] = {};
            fov_probe_warp_mat[fov_id] = cv::Mat();
        }
        auto probe_mk_det = [&, this](
            cv::Mat mat
        ) {
            auto mk_regs = probe_mk_detector(mat);
            summit::grid::log.debug("Probe marker # = {}", mk_regs.size());
            return mk_regs;
        };

        summit::grid::log.debug("Probe channel image nums: {}", task.probe_channel_imgs().size());
        // auto example1 = Utils::get_fov_mat_from(task.probe_channel_imgs(), 0, 0);
        // auto mk_pos_des = probe_mk_det(example1);
        // std::vector<cv::Point2d>    mk_pos_px       ;
        // std::vector<cv::Point2d>    mk_pos_rum      ;
        // for(auto& [fov_mk_id, score, pos] : mk_pos_des) {
        //     auto mk_lt_x  = (fov_mk_id.x * task.mk_wd_rum()) + task.xi_rum();
        //     auto mk_lt_y  = (fov_mk_id.y * task.mk_hd_rum()) + task.yi_rum();
        //     auto mk_cen_x = mk_lt_x + (task.mk_w_rum() / 2);
        //     auto mk_cen_y = mk_lt_y + (task.mk_h_rum() / 2);
        //     mk_pos_rum.emplace_back(mk_cen_x, mk_cen_y);
        //     mk_pos_px.push_back(pos);
        //     summit::grid::log.debug("mk_cen : ({}, {})", mk_cen_x, mk_cen_y);
        //     summit::grid::log.debug("pos    : ({}, {})", pos.x, pos.y);
        // }
        // auto warp_mat = chipimgproc::warped_mat::estimate_transform_mat(mk_pos_rum, mk_pos_px);


        task.probe_channel_imgs()
        | nucleona::range::indexed()
        | ranges::view::transform([&](auto&& p){
            auto& i          = p.first;
            auto& fov_id_mat = p.second;
            auto& fov_id     = fov_id_mat.first;
            auto& path       = std::get<0>(fov_id_mat.second);
            cv::Mat mat      = std::get<1>(fov_id_mat.second);
            summit::grid::log.debug("Probe channel, fov id:({}, {}) start process", fov_id.x, fov_id.y);
            try {
                auto& mk_num = fov_marker_num.at(fov_id);
                /* Detect probe markers position */
                auto mk_pos_des = probe_mk_det(mat);
                std::vector<cv::Point2d>    mk_pos_px       ;
                std::vector<cv::Point2d>    mk_pos_rum      ;

                for(auto& [fov_mk_id, score, pos] : mk_pos_des) {
                    auto mk_lt_x  = (fov_mk_id.x * task.mk_wd_rum()) + task.xi_rum();
                    auto mk_lt_y  = (fov_mk_id.y * task.mk_hd_rum()) + task.yi_rum();
                    auto mk_cen_x = mk_lt_x + (task.mk_w_rum() / 2);
                    auto mk_cen_y = mk_lt_y + (task.mk_h_rum() / 2);
                    mk_pos_rum.emplace_back(mk_cen_x, mk_cen_y);
                    mk_pos_px.push_back(pos);
                    // summit::grid::log.debug("mk_cen : ({}, {})", mk_cen_x, mk_cen_y);
                    // summit::grid::log.debug("pos    : ({}, {})", pos.x, pos.y);
                }
                auto mk_num_match_spec = mk_pos_px.size() == mk_num.x * mk_num.y;
                if(!mk_num_match_spec) {
                    fov_pb_warnings.at(fov_id) = true;
                    // task.create_warning_file();
                }
                auto warp_mat = chipimgproc::warped_mat::estimate_transform_mat(mk_pos_rum, mk_pos_px);

                /* Summary */
                // 1. Warp matrix estimated from the finally filtered marker pos.
                // 2. Rotation degree computed from the warp matrix.
                // 3. Marker position inferred from GDS spec (scaled GDS domain).
                // 4. Marker position detected from image (real image domain).
                // 5. Deducing flag for success of probe channel FOVs image process for current FOV.
                fov_probe_warp_mat.at(fov_id) = warp_mat;
                fov_rot_degs.at(fov_id)       = chipimgproc::rotation::rot_deg_from_warp(warp_mat);
                fov_mk_pos_spec.at(fov_id)    = std::move(mk_pos_rum);
                fov_pb_mk_pos.at(fov_id)      = std::move(mk_pos_px);
                fov_pb_successes.at(fov_id)   = true;

                fov_rot_degs.at(fov_id) = chipimgproc::warp_mat::rot_deg_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), theta: {}", fov_id.x, fov_id.y, fov_rot_degs.at(fov_id));
                auto slope = chipimgproc::warp_mat::slope_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), slope: {}", fov_id.x, fov_id.y, slope);
                fov_scales.at(fov_id) = chipimgproc::warp_mat::scale_from_warp(warp_mat);
                summit::grid::log.debug("white channel, fov id:({}, {}), x scale: {}, y scale: {}", 
                fov_id.x, fov_id.y, fov_scales.at(fov_id).x, fov_scales.at(fov_id).y);
            } catch (...) {
                summit::grid::log.error(
                    "Probe channel, fov id:({}, {}) process fialed",
                    fov_id.x, fov_id.y
                );
                fov_pb_successes.at(fov_id) = false;
            }
            summit::grid::log.debug("Probe channel, fov id:({}, {}) end process", fov_id.x, fov_id.y);
            return 0;
        })
        | nucleona::range::p_endp(executor)
        ;

        summit::grid::log.debug("The process for the first probe channel FOVs is finished.");

        /* Some statistics */
        cv::Mat backup_warp_mat = cv::Mat();
        std::size_t success_num = 0;
        for(auto f = fov_pb_successes.begin(); f != fov_pb_successes.end(); f++) {
            if(f->second) success_num ++;
            // TODO: Remedy for failed FOVs.
        }
        if(success_num == 0) {
            debug_throw(std::runtime_error("The process for the first probe channel FOVs is failed!"));
        }

        /* Consensus */
        auto rot_degs = fov_pb_successes
            | ranges::view::filter([&](auto&& p ){ return p.second; })
            | nr::transformed([&](auto&& p){ return fov_rot_degs.at(p.first); })
            | ranges::to_vector
        ;
        auto rot_deg = Utils::mean(rot_degs);
        task.set_ref_from_probe_ch(true);
        task.set_ref_ch_warp_mat(std::move(fov_probe_warp_mat));
        task.set_fov_ref_ch_mk_pos(std::move(fov_pb_mk_pos));
        task.set_fov_ref_ch_successes(std::move(fov_pb_successes));
        task.set_fov_mk_pos_spec(std::move(fov_mk_pos_spec));
        task.set_rot_degree(rot_deg);
        task.collect_fovs_warnings(fov_pb_warnings, task.warn());

    /* 
     * The following code is for grid one.
     */

    //     using namespace __alias;
    //     cmk_det::RegMat probe_mk_detector       ;
    //     int  sel_fov_row = task.fov_rows() / 2;
    //     int  sel_fov_col = task.fov_cols() / 2;
    //     auto [ch_i, ch]  = task.first_probe_channel();
    //     auto [mat, img_path] = Utils::read_img<std::uint16_t>(
    //         task.chip_dir(),
    //         sel_fov_row, sel_fov_col,
    //         ch["name"].get<std::string>(),
    //         task.is_img_enc(),
    //         task.model()
    //     );
    //     // should add Utils::imwrite manully
    //     auto mks                 = task.get_marker_patterns_by_marker_type(ch.at("marker_type"));
    //     auto& marker             = mks.at(0)->marker;
    //     auto& mask               = mks.at(0)->mask;
    //     auto& fov_marker_num     = task.get_fov_marker_num(sel_fov_row, sel_fov_col);
    //     auto  probe_mk_layout    = cmk::make_single_pattern_reg_mat_layout(
    //         marker, mask,
    //         task.cell_h_um(), task.cell_w_um(),
    //         task.space_um(),
    //         fov_marker_num.y,
    //         fov_marker_num.x,
    //         task.mk_wd_cl(),
    //         task.mk_hd_cl(),
    //         task.um2px_r()
    //     );
    //     std::vector<cv::Point>   low_score_marker_idx;
    //     auto  pbmk_iter_rot_cali = crot::make_iteration_cali(
    //         [&, this](const cv::Mat& mat) {
    //             auto mk_regs    = probe_mk_detector(
    //                 static_cast<const cv::Mat_<std::int16_t>&>(mat), 
    //                 probe_mk_layout, 
    //                 __alias::cimp::MatUnit::PX, 0,
    //                 nucleona::stream::null_out
    //             );
    //             low_score_marker_idx = cmk_det::filter_low_score_marker(mk_regs);
    //             auto theta      = rotate_detector(mk_regs, nucleona::stream::null_out);
    //             return theta;
    //         },
    //         [&, this](cv::Mat& mat, auto theta) {
    //             rotate_calibrator(mat, theta /*debug viewer*/);
    //         }
    //     );

    //     // * marker detection and rotate
    //     auto theta      = pbmk_iter_rot_cali(mat);
    //     // std::cout << "probe channel detect theta: " << theta << std::endl;
    //     cv::Mat mat_loc = mat.clone();
    //     rotate_calibrator(mat_loc, theta);

    //     // * um2px_r auto scaler
    //     cimp::algo::Um2PxAutoScale auto_scaler(
    //         mat_loc, 
    //         task.cell_w_um(), task.cell_h_um(),
    //         task.space_um()
    //     );
    //     auto [best_um2px_r, score_mat] = auto_scaler.linear_steps(
    //         probe_mk_layout, task.um2px_r(), 0.002, 7,
    //         low_score_marker_idx, nucleona::stream::null_out
    //     );
    //     task.set_rot_degree(theta);
    //     task.set_um2px_r(best_um2px_r);
        return true;
    }
    /**
     * @brief Draw grid line on image
     * 
     * @tparam T Grid line position type
     * @param mat Image
     * @param gl_x Grid line position along x dimension
     * @param gl_y Grid line position along y dimension
     * @param line_value Line gray color value
     */
    template<class T>
    void draw_grid(
        cv::Mat& mat, 
        const std::vector<T>& gl_x, 
        const std::vector<T>& gl_y, 
        int line_value
    ) const {
        for(auto&& xl : gl_x) {
            cv::line(mat,
                cv::Point(xl, 0), 
                cv::Point(xl, mat.rows),
                line_value
            );
        }
        for(auto&& yl : gl_y) {
            cv::line(mat,
                cv::Point(0, yl), 
                cv::Point(mat.cols, yl),
                line_value
            );
        }
    }
    /**
     * @brief Create debug grid line stitched images and write to filesystem.
     * 
     * @param task chip parameter model
     */
    void gridline_debug_image_proc(model::Task& task) const { 
        chipimgproc::stitch::GridlineBased gl_stitcher;
        std::map<std::string, const nlohmann::json*> channel_params;
        /*
         * Find white channel name
         */
        std::string wh_name = "";
        for(auto&& ch : task.channels()) {
            channel_params[ch.at("name").get<std::string>()] = &ch;
            if(ch.at("filter").get<int>() == 0) {
                wh_name = ch.at("name").get<std::string>();
            }
        }
        /*
         * Filter id to color mapping, 0 = blue, 1 = green, 2 = red
         */
        std::vector<int> filter_to_color({0, 0, 1, 0, 2, 0});
        /*
         * Create white channel stitched image.
         */
        // if(!wh_name.empty() && channel_params.size() > 1) {
        //     auto tpl_mtm = task.multi_tiled_mat().begin()->second.value();
        //     for(auto&& [fov_id, img_data] : task.white_channel_imgs()) {
        //         auto& [path, img] = img_data;
        //         cv::Mat img_loc = img.clone();
        //         rotate_calibrator(img_loc, task.rot_degree().value());
        //         auto& tpl_gri = tpl_mtm.get_fov_img(fov_id.x, fov_id.y);
        //         tpl_gri.mat() = img_loc;
        //     }
        //     auto gl_wh_stitch = gl_stitcher(tpl_mtm);
        //     task.set_stitched_img(wh_name, std::move(gl_wh_stitch));
        // }
        if(channel_params.size() <= (wh_name.empty() ? 0 : 1)) {
            summit::grid::log.warn("no probe channel images provided, unable to generate stitched grid images");
            return ;
        }

        /*
         * Stitch each probe channel image and write to files.
         * White channel convert 8 bit to 16 bit.
         */
        std::vector<cv::Mat> channels;
        channels.resize(3);
        for(auto& [chid, st_img] : task.stitched_img()) {
            auto loc_st_img = st_img.mat().clone();
            if(chid == wh_name) {
                cv::Mat tmp;
                loc_st_img.convertTo(tmp, CV_16UC1, 256);
                loc_st_img = tmp;
            } else {
                loc_st_img = chipimgproc::viewable(loc_st_img, 5000);
            }
            draw_grid(loc_st_img, st_img.gl_x(), st_img.gl_y(), 32767);
            auto ch_filter = channel_params.at(chid)->at("filter").get<int>();
            channels.at(filter_to_color.at(ch_filter)) = loc_st_img;
            cv::imwrite(task.debug_stitch(chid).string(), loc_st_img);
        }
        /* channel may empty, because user may only scan one channel, 
         we need to fill empty channel for merge */
        cv::Mat* first_exist_ch = nullptr;
        for(auto& ch : channels) {
            if(ch.empty()) continue;
            first_exist_ch = &ch;
            break;
        }
        for(auto& ch : channels) {
            if(ch.empty()) {
                ch = cv::Mat::zeros(
                    cv::Size(first_exist_ch->cols, first_exist_ch->rows),
                    first_exist_ch->type()
                );
            }
            summit::grid::log.info("{},{},{}", ch.rows, ch.cols, ch.type());
        }
        try {
            /*
             * Merge all channel and write to file.
             */
            cv::Mat stitched_grid_all;
            cv::merge(channels, stitched_grid_all);
            cv::imwrite(task.debug_stitch("merged").string(), stitched_grid_all);
        } catch(...) {
            summit::grid::log.error("Stitched and channel merged image generate failed\n"
                "This is because the each channel may not use the same gridding source and when the source is different,\n"
                "the image size will not exactly same.\n"
                "In this case, each channel image is logically unmerge able.\n"
                "This is current algorithm limitation."
            );
        }
    }
    /**
     * @brief Run chip level process.
     * @details See @ref chip-level-process "Chip level process" for more details.
     * @param task chip parameter model
     * @return decltype(auto) exit code
     */
    decltype(auto) operator()(model::Task& task) const {
        using namespace __alias;
        // auto tmp_timer(std::chrono::steady_clock::now());
        // std::chrono::duration<double, std::milli> d;
        auto& model = task.model();
        auto& executor = task.model().executor();
        try {
            auto timer = nucleona::proftool::make_timer([&](auto&& du){
                auto du_ms = std::chrono::duration_cast<std::chrono::milliseconds>(du).count();
                task.set_proc_time(du_ms / 1000.0);
            });
            // auto tmp_timer(std::chrono::steady_clock::now());
            // std::chrono::duration<double, std::milli> dd;
            task.grid_log()["date"] = summit::utils::datetime("%Y/%m/%d %H:%M:%S", std::chrono::system_clock::now());
            auto& wh_ch_log = task.grid_log()["white_channel_proc"];
            auto& wh_ch_warning = task.grid_log()["white_channel_warning"];
            if(task.model().auto_gridding()) {
                if(!white_channel_proc(task)) {
                    task.set_white_ch_proc_failed(true);
                    wh_ch_log = false;
                    auto& probe_ch_log = task.grid_log()["probe_channel_proc"];
                    if(!task.um_to_px_r_done()) {
                        throw std::runtime_error("no on spec um to pixel rate provided");
                    } 
                    if(!probe_channel_proc(task)) {
                        probe_ch_log = false;
                        throw std::runtime_error("both white/probe channel are failed");
                    } else {
                        probe_ch_log = true;
                    }
                } else {
                    wh_ch_log = true;
                    wh_ch_warning = task.warn();
                }
            } else {
                wh_ch_log = false;
                auto& in_grid_log = task.model().in_grid_log();
                task.set_rot_degree(in_grid_log.at("rotate_degree"));
                task.set_um2px_r(in_grid_log.at("um_to_pixel_rate"));
            }
            // dd = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "white: " << dd.count() << " ms\n";

            // tmp_timer = std::chrono::steady_clock::now();
            task.grid_log()["rotate_degree"] = task.rot_degree().value();
            // task.grid_log()["um_to_pixel_rate"] = task.um2px_r();
            task.probe_channels()
            | nucleona::range::indexed()
            | ranges::view::transform([&task](auto&& i_jch){
                auto& i = std::get<0>(i_jch);
                auto& jch = std::get<1>(i_jch);
                model::Channel ch_mod;
                ch_mod.init(task, jch);
                channel(ch_mod);
                task.channel_log().at(i) = ch_mod.grid_log();
                return 0;
            })
            | nucleona::range::p_endp(executor)
            ;
            // dd = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "fluro: " << dd.count() << " ms\n";

            task.grid_log()["input"] = task.model().input().string();
            task.grid_log()["chip_dir"] = task.chip_dir().string();
            task.grid_log()["output_formats"] = task.model().FormatDecoder::to_string();
            task.grid_log()["output"] = task.model().output().string();
            // task.grid_log()["no_bgp"] = task.model().no_bgp();
            // task.grid_log()["method"] = task.model().method();
            task.grid_log()["shared_dir"] = task.model().shared_dir_path().string();
            task.grid_log()["secure_dir"] = task.model().secure_dir_path().string();
            task.grid_log()["marker_append"] = task.model().marker_append();
            task.grid_log()["auto_gridding"] = task.model().auto_gridding();
            task.grid_log()["version"] = summit::grid::version().to_string();
            task.summary_channel_log();
            task.model().heatmap_writer().flush();
            if(task.warn()) {
                task.create_warning_file();
            }
            if(task.grid_bad()) {
                debug_throw(
                    std::runtime_error(
                        "several channel gridding failed, stop process(gridline image not generated)"
                    )
                );
                task.create_warning_file();
            }
            if(task.model().debug() >= 5) {
                gridline_debug_image_proc(task);
            }

        } catch( const std::exception& e ) {
            summit::grid::log.error("grid failed with reason: {}", e.what());
            task.set_grid_done(false);
            task.grid_log()["grid_fail_reason"] = e.what();
        }
        if(task.model().debug() >= 4) {
            task.write_gridding_results_log();
        }
        task.write_log();
        task.copy_chip_log();
        // d = std::chrono::steady_clock::now() - tmp_timer;
        // std::cout << "chip: " << d.count() << " ms\n";
        if(task.grid_done()) {
            return 0;
        } else {
            return 1;
        }
    }
private:
    /**
     * @brief Functor, marker detection based rotation degree detection.
     * 
     */
    __alias::crot::MarkerVec<float>             rotate_detector         ;
    /**
     * @brief Functor, rotation calibration
     * 
     */
    __alias::crot::Calibrate                    rotate_calibrator       ;
    /**
     * @brief Functor, marker based micron-to-pixel rate estimiation
     * 
     */
    __alias::cmk::RegMatUm2PxRDet               reg_mat_um2px_r_det     ;
    
};

}
