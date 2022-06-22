/**
 * @file summit/app/grid/pipeline/channel.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::pipeline::Channel
 * 
 */
#pragma once
#include <summit/app/grid/model.hpp>
#include <summit/app/grid/model/type.hpp>
#include <cstdlib>
#include <future>
#include <iostream>
#include <limits>
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
/**
 * @brief Channel level process, 
 *        create marker layout for all FOV and distrbute the FOV 
 *        parameters to FOV level pipeline.
 * @details See @ref channel-level-process "Channel level process" for more details.
 */
constexpr struct Channel {
    using Float = summit::app::grid::model::Float;
    using GridLineID = summit::app::grid::model::GridLineID;
    /**
     * @brief Run the channel level process.
     * 
     * @param channel channel parameter model.
     * @return decltype(auto) exit code. 
     */
    decltype(auto) operator()(model::Channel& channel) const {
        using namespace __alias;
        auto& task = channel.task();
        try{
            auto& model = task.model();
            if(!task.rot_degree_done())
                throw std::runtime_error("[BUG]: no rotation degree, surrender gridding");
            // if(!task.um_to_px_r_done())
            //     throw std::runtime_error("[BUG]: no um to pixel rate, surrender gridding");
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
            // auto tmp_fov_mk_layouts = Utils::generate_sgl_pat_reg_mat_marker_layout(
            //     task.um2px_r(),
            //     task.chipspec(),
            //     task.fov(),
            //     channel.json()
            // );

            // auto tmp_timer(std::chrono::steady_clock::now());
            // std::chrono::duration<double, std::milli> d, d1;
            // read all fov image by using std::async to prevent subsequent I/O blockage
            // using FutureMap = std::map<
            //         decltype(fov_mods)::key_type, std::future<void>, chipimgproc::PointLess
            //     >;
            // FutureMap read_futures;
            // for (auto& fov_id_mod : fov_mods) {
            //     read_futures.emplace(
            //         fov_id_mod.first, 
            //         std::async(
            //             std::launch::async, 
            //             [&channel, &fov_id = fov_id_mod.first, &fov_mod = fov_id_mod.second](){
            //                 fov_mod.init(channel, fov_id);
            //             }
            //         )
            //     );
            // }
            // for (auto& fov_id_mod : fov_mods)
            //     read_futures.at(fov_id_mod.first).wait();
            // std::future<void> write_future(
            //     Utils::get_future_write_imgs(task, channel.ch_name(), fov_mods)
            // );
            fov_mods
            | ranges::view::transform([&](auto&& fov_id_mod){
                auto& fov_id = fov_id_mod.first;
                auto& fov_mod = fov_id_mod.second;
                // auto tmp_timer1(std::chrono::steady_clock::now());
                fov_mod.init(channel, fov_id);
                
                // d1 += std::chrono::steady_clock::now() - tmp_timer1;
                // fov_mod.set_mk_layout(std::move(tmp_fov_mk_layouts.at(fov_id)));
                if(model.auto_gridding()) {
                    fov_ag(fov_mod);
                } else {
                    fov_nag(fov_mod);
                }
                return 0;
            })
            | nucleona::range::p_endp(executor);
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "fov_mod init: " << d1.count() << " ms\n";
            // std::cout << "fov: " << d.count() << " ms\n";

            channel.summary_fov_log(fov_mods);
            if(channel.grid_log().at("grid_bad").get<bool>()) {
                debug_throw(std::runtime_error("bad process FOV exist, channel process stop collecting result"));
            }
            // tmp_timer = std::chrono::steady_clock::now();
            channel.collect_fovs(fov_mods);
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "collect_fovs: " << d.count() << " ms\n";

            // write heatmap
            // tmp_timer = std::chrono::steady_clock::now();
            channel.heatmap_writer()(channel.multi_warped_mat());
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "heatmap_writer: " << d.count() << " ms\n";

            // stitch image
            // tmp_timer = std::chrono::steady_clock::now();
            model::GLRawImg stitched_grid_img;
            auto& std_rum_st_pts = task.stitched_points_rum();
            std::vector<cv::Mat> std_rum_imgs;
            for(auto [fov_id, st_pts_cl] : task.stitched_points_cl()) {
                std_rum_imgs.push_back(
                    fov_mods.at(fov_id).std_img()
                );
            }
            auto stitched_img = chipimgproc::stitch::add(
                std_rum_imgs, std_rum_st_pts
            );
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "stitched: " << d.count() << " ms\n";

            // tmp_timer = std::chrono::steady_clock::now();
            {
                // stitch-[channel].png @ viewable_norm folder
                auto v_st_img = cimp::viewable(stitched_img);
                auto st_img_path = channel.stitch_image("norm");
                cv::imwrite(st_img_path.string(), v_st_img);
            }
            {
                // stitch-[channel].tiff @ viewable_raw folder
                auto raw_st_img_path = channel.stitch_image("raw");
                auto std2raw = task.std2raw_warp();
                cv::Mat raw_stitched_img;
                cv::warpAffine(stitched_img, raw_stitched_img, std2raw, cv::Size(
                    std::round(task.chip_w_rum()*task.rum2px_r()), 
                    std::round(task.chip_h_rum()*task.rum2px_r())
                ));
                cv::imwrite(raw_st_img_path.string(), raw_stitched_img);
                
                // stitch_img_gridline.csv @ viewable_raw folder
                model::GLRawImgFlt raw_stitched_grid_img;
                raw_stitched_grid_img.mat() = std::move(raw_stitched_img);
                raw_stitched_grid_img.gl_x() = task.gl_x_raw();
                raw_stitched_grid_img.gl_y() = task.gl_y_raw();
                std::ofstream gl_raw_file(channel.stitch_gridline("raw").string());
                Utils::write_gl(gl_raw_file, raw_stitched_grid_img);
            }
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "write_viewable: " << d.count() << " ms\n";

            if (task.model().debug() >= 5) {
                auto std_st_img_path = channel.stitch_image("rescale");
                cv::imwrite(std_st_img_path.string(), stitched_img);
            }

            std::vector<model::GLID> x_gl(task.spec_w_cl() + 1);
            std::vector<model::GLID> y_gl(task.spec_h_cl() + 1);
            for(model::GLID i = 0; i <= task.spec_w_cl(); i ++) {
                x_gl[i] = i * task.cell_wd_rum();
            }
            for(model::GLID i = 0; i <= task.spec_h_cl(); i ++) {
                y_gl[i] = i * task.cell_hd_rum();
            }
            stitched_grid_img.mat() = std::move(stitched_img);
            stitched_grid_img.gl_x() = task.gl_x_rum();
            stitched_grid_img.gl_y() = task.gl_y_rum();

            // tmp_timer = std::chrono::steady_clock::now();
            // gridline
            std::ofstream gl_file(channel.gridline().string());
            Utils::write_gl(gl_file, stitched_grid_img);
            channel.set_gridline(
                stitched_grid_img.gl_x(), 
                stitched_grid_img.gl_y()
            );
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "gridline: " << d.count() << " ms\n";

            channel.set_stitched_img(std::move(stitched_grid_img));

            // // TODO: background
            // Utils::FOVMap<float> bg_value;
            // for(auto&& p : fov_mods) {
            //     auto& fov_id = p.first;
            //     auto& fov_mod = p.second;
            //     bg_value[fov_id] = Utils::mean(fov_mod.bg_means());
            // }
            // channel.background_writer()(bg_value);

            // marker append
            // tmp_timer = std::chrono::steady_clock::now();
            task.collect_fovs_zero_value_ct(
                channel.fov_zero_val_ct(),
                channel.ch_name(),
                channel.grid_log(),
                channel.warn()
            );
            if(model.marker_append()) {
                channel.mk_append_view()(channel.mk_append_mat());
                if(task.ref_from_probe_ch()) {
                    cv::Point center_id(task.fov_rows()/2, task.fov_cols()/2);
                    task.check_fovs_mk_append(
                        channel.fov_mk_append_dn(),
                        channel.fov_mk_append_correlation(),
                        center_id,
                        task.pb_mk_append_eval(),
                        channel.warn()
                    );
                    task.collect_fovs_mk_append_correlation(
                        channel.fov_mk_append_correlation(), 
                        center_id,
                        channel.ch_name(),
                        channel.grid_log()
                    );
                }
                if(task.compute_sharpness()) {
                    channel.compute_mk_append_statistics();
                }
            }
            // d = std::chrono::steady_clock::now() - tmp_timer;
            // std::cout << "white marker_append: " << d.count() << " ms\n";
            channel.set_warning();
        } catch (const std::exception& e) {
            channel.set_grid_failed(e.what());
            summit::grid::log.error(
                "channel: {} process failed, reason: {}", channel.ch_name(), e.what()
            );
            if (task.model().debug() >= 6) {
                std::cerr << "An exception thrown while processing fluor channel, and debug level is the highest, program exit.\n";
                std::exit(-1);
            }
        }
        return !channel.grid_log().at("grid_bad").get<bool>();
    }
private:
    /**
     * @brief Functor, channel FOVs stitch, grid line based algorithm.
     * 
     */
    __alias::cm_st::GridlineBased               image_stitcher          ;
} channel;

}