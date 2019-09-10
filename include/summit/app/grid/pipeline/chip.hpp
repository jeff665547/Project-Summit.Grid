#pragma once
#include <summit/app/grid/model.hpp>
#include <Nucleona/range.hpp>
#include "channel.hpp"
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/rotation/marker_vec.hpp>
#include <ChipImgProc/rotation/iteration_cali.hpp>
#include <ChipImgProc/marker/reg_mat_um2px_r_det.hpp>
#include <summit/app/grid/aruco_setter.hpp>
#include <Nucleona/util/remove_const.hpp>
#include <summit/app/grid/model/marker_base.hpp>
#include <ChipImgProc/algo/um2px_auto_scale.hpp>
#include <summit/app/grid/white_mk_append.hpp>
#include <ChipImgProc/marker/cell_layout.hpp>

namespace summit::app::grid::pipeline {
namespace __alias {

namespace cimp      = chipimgproc;
namespace cmk       = chipimgproc::marker;
namespace crot      = chipimgproc::rotation;
namespace cmk_det   = chipimgproc::marker::detection;

}
struct Chip {
    Chip() 
    : rotate_detector       () 
    , rotate_calibrator     () 
    , reg_mat_um2px_r_det   () 
    {}

    bool white_channel_proc(model::Task& task) const {
        namespace nr = nucleona::range;
        using namespace __alias;
        cmk::CellLayout         cell_layout             ;
        cmk_det::ArucoRegMat    aruco_mk_detector       ;
        auto& executor  = task.model().executor();
        auto& model     = task.model();
        
        task.set_white_channel_imgs(Utils::read_white_channel(
            task.channels(),
            task.chip_dir(),
            task.fov_rows(),
            task.fov_cols(),
            task.is_img_enc(),
            task.model()
        ));
        if(task.white_channel_imgs().size() == 0) return false;
        if(!aruco_setter(aruco_mk_detector, task)) return false;

        cell_layout.reset(
            task.fov_rows(),    task.fov_cols(),
            task.fov_w(),       task.fov_h(),
            task.fov_wd(),      task.fov_hd(),
            task.mk_row_cl(),   task.mk_col_cl(),
            task.mk_xi_cl(),    task.mk_yi_cl(),
            task.mk_w_cl(),     task.mk_h_cl(),
            task.mk_wd_cl(),    task.mk_hd_cl()
        );

        auto& fov_marker_num = task.fov_marker_num();
        std::vector<float> rot_degs (task.white_channel_imgs().size());
        std::vector<float> um2px_rs (task.white_channel_imgs().size());
        std::vector<bool>  success  (task.white_channel_imgs().size());
        Utils::FOVMarkerRegionMap fov_marker_regs;
        Utils::FOVMap<cv::Mat>    fov_mk_append;
        for(auto&& [fov_id, mat] : task.white_channel_imgs()) {
            fov_marker_regs[fov_id] = {};
            fov_mk_append[fov_id] = cv::Mat();
        }
        auto aruco_mk_det = [&, this](
            const cv::Mat& mat, 
            const cv::Point& fov_id, 
            double um2px_r
        ) {
            auto fov_mk_num = fov_marker_num.at(fov_id);
            auto mk_regs = aruco_mk_detector(
                static_cast<const cv::Mat_<std::uint8_t>&>(mat), 
                task.mk_wd_px(um2px_r),    task.mk_hd_px(um2px_r),
                task.mk_w_px(um2px_r),     task.mk_h_px(um2px_r),
                fov_mk_num.x,       fov_mk_num.y,
                task.pyramid_level(),
                cell_layout.mk_id_fov_to_chip(fov_id.x, fov_id.y)
            );
            __alias::cmk_det::filter_low_score_marker(mk_regs);
            return mk_regs;
        };
        auto aruco_iter_rot_cali = crot::make_iteration_cali(
            [&, this](
                const cv::Mat& mat, 
                const cv::Point& fov_id, 
                double um2px_r
            ) {
                auto mk_regs = aruco_mk_det(mat, fov_id, um2px_r);
                auto theta   = rotate_detector(mk_regs, nucleona::stream::null_out);
                return theta;
            },
            [&, this](cv::Mat& mat, auto theta) {
                rotate_calibrator(mat, theta /*debug viewer*/);
            }
        );

        task.white_channel_imgs()
        | nucleona::range::indexed()
        | ranges::view::transform([&](auto&& p){
            auto& i             = p.first;
            auto& fov_id_mat    = p.second;
            auto& fov_id        = fov_id_mat.first;
            auto& path          = std::get<0>(fov_id_mat.second);
            auto& mat           = std::get<1>(fov_id_mat.second);
            try {
                auto& mk_num    = fov_marker_num.at(fov_id);

                // * count theta
                auto theta      = aruco_iter_rot_cali(
                                    mat, 
                                    nucleona::make_tuple(
                                        fov_id, task.um2px_r()
                                    )
                                  );
                // std::cout << "white channel detect theta " 
                //     << fov_id << ": " << theta << std::endl;
                cv::Mat mat_loc = mat.clone();
                rotate_calibrator(mat_loc, theta);

                // * detect marker regions
                auto mk_regs = aruco_mk_det(mat_loc, fov_id, task.um2px_r());
                task.aruco_ch_mk_seg_view(fov_id.y, fov_id.x)(
                    chipimgproc::marker::view(mat_loc, mk_regs)
                );
                mk_regs = __alias::cmk_det::reg_mat_infer(mk_regs, mk_num.y, mk_num.x);

                // * detect um2px_r
                auto um2px_r = reg_mat_um2px_r_det(
                    mk_regs, 
                    task.mk_wd_um(),
                    task.mk_hd_um(),
                    nucleona::stream::null_out
                );
                mk_regs = aruco_mk_det(mat_loc, fov_id, um2px_r);
                mk_regs = __alias::cmk_det::reg_mat_infer(mk_regs, mk_num.y, mk_num.x);

                if(model.marker_append()) {
                    auto fov_wh_mk_append = white_mk_append(
                        mat_loc, mk_regs, task, um2px_r
                    );
                    fov_mk_append.at(fov_id) = fov_wh_mk_append;
                }
                // std::cout << "white channel detect um2px rate " 
                //     << fov_id << ": " << um2px_r << std::endl;

                // std_reg_mat(mk_regs);
                fov_marker_regs.at(fov_id) = std::move(mk_regs);
                rot_degs.at(i) = theta;
                um2px_rs.at(i) = um2px_r;
                success.at(i)  = true;

            } catch (...) {
                success.at(i) = false;
            }
            return 0;
        })
        | nucleona::range::p_endp(executor)
        ;

        std::size_t success_num = 0;
        for(auto f : success) {
            if(f) success_num ++;
        }
        if(success_num == 0) return false;

        // * consensus
        rot_degs = rot_degs 
            | nr::indexed() 
            | ranges::view::filter([&](auto&& p ){ return success.at(p.first); }) 
            | nr::transformed([&](auto&& p){ return p.second; })
            | ranges::to_vector
        ;
        um2px_rs = um2px_rs
            | nr::indexed() 
            | ranges::view::filter([&](auto&& p ){ return success.at(p.first); }) 
            | nr::transformed([&](auto&& p){ return p.second; })
            | ranges::to_vector
        ;
        auto rot_deg = Utils::mean(rot_degs);
        auto um2px_r = Utils::mean(um2px_rs);
        task.set_rot_degree(rot_deg);
        task.set_um2px_r(um2px_r);
        task.set_fov_mk_regs(std::move(fov_marker_regs));
        if(model.marker_append()) {
            task.collect_fovs_mk_append(fov_mk_append);
        }
        return true;
    }
    bool probe_channel_proc(model::Task& task) const {
        using namespace __alias;
        cmk_det::RegMat probe_mk_detector       ;
        int  sel_fov_row = task.fov_rows() / 2;
        int  sel_fov_col = task.fov_cols() / 2;
        auto [ch_i, ch]  = task.first_probe_channel();
        auto [mat, img_path] = Utils::read_img<std::uint16_t>(
            task.chip_dir(),
            sel_fov_row, sel_fov_col,
            ch["name"].get<std::string>(),
            task.is_img_enc(),
            task.model()
        );
        auto& mks_pair           = task.get_marker_patterns(ch["marker_type"]);
        auto& markers            = mks_pair.marker;
        auto& masks              = mks_pair.mask;
        auto& fov_marker_num     = task.get_fov_marker_num(sel_fov_row, sel_fov_col);
        auto  probe_mk_layout    = cmk::make_single_pattern_reg_mat_layout(
            markers.at(0), masks.at(0),
            task.cell_h_um(), task.cell_w_um(),
            task.space_um(),
            fov_marker_num.y,
            fov_marker_num.x,
            task.mk_wd_cl(),
            task.mk_hd_cl(),
            task.um2px_r()
        );
        std::vector<cv::Point>   low_score_marker_idx;
        auto  pbmk_iter_rot_cali = crot::make_iteration_cali(
            [&, this](const cv::Mat& mat) {
                auto mk_regs    = probe_mk_detector(
                    static_cast<const cv::Mat_<std::int16_t>&>(mat), 
                    probe_mk_layout, 
                    __alias::cimp::MatUnit::PX, 0,
                    nucleona::stream::null_out
                );
                low_score_marker_idx = cmk_det::filter_low_score_marker(mk_regs);
                auto theta      = rotate_detector(mk_regs, nucleona::stream::null_out);
                return theta;
            },
            [&, this](cv::Mat& mat, auto theta) {
                rotate_calibrator(mat, theta /*debug viewer*/);
            }
        );

        // * marker detection and rotate
        auto theta      = pbmk_iter_rot_cali(mat);
        // std::cout << "probe channel detect theta: " << theta << std::endl;
        cv::Mat mat_loc = mat.clone();
        rotate_calibrator(mat_loc, theta);

        // * um2px_r auto scaler
        cimp::algo::Um2PxAutoScale auto_scaler(
            mat_loc, 
            task.cell_w_um(), task.cell_h_um(),
            task.space_um()
        );
        auto [best_um2px_r, score_mat] = auto_scaler.linear_steps(
            probe_mk_layout, task.um2px_r(), 0.002, 7,
            low_score_marker_idx, nucleona::stream::null_out
        );
        task.set_rot_degree(theta);
        task.set_um2px_r(best_um2px_r);
        return true;
    }
    decltype(auto) operator()(model::Task& task) const {
        using namespace __alias;
        auto& model = task.model();
        auto& executor = task.model().executor();
        try {
            auto timer = nucleona::proftool::make_timer([&](auto&& du){
                auto du_ms = std::chrono::duration_cast<std::chrono::milliseconds>(du).count();
                task.set_proc_time(du_ms / 1000.0);
            });
            auto& wh_ch_log = task.grid_log()["white_channel_proc"];
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
                }
            } else {
                wh_ch_log = false;
                auto& in_grid_log = task.model().in_grid_log();
                task.set_rot_degree(in_grid_log.at("rotate_degree"));
                task.set_um2px_r(in_grid_log.at("um_to_pixel_rate"));
            }
            task.grid_log()["rotate_degree"] = task.rot_degree().value();
            task.grid_log()["um_to_pixel_rate"] = task.um2px_r();
            task.probe_channels()
            | nucleona::range::indexed()
            | ranges::view::transform([&task](auto&& i_jch){
                auto& i = std::get<0>(i_jch);
                auto& jch = std::get<1>(i_jch);
                model::Channel ch_mod;
                ch_mod.init(task, jch, i);
                channel(ch_mod);
                task.channel_log().at(i) = ch_mod.grid_log();
                return 0;
            })
            | nucleona::range::p_endp(executor)
            ;
            task.grid_log()["input"] = task.model().input().string();
            task.grid_log()["chip_dir"] = task.chip_dir().string();
            task.grid_log()["output_formats"] = task.model().FormatDecoder::to_string();
            task.grid_log()["output"] = task.model().output().string();
            task.grid_log()["no_bgp"] = task.model().no_bgp();
            task.grid_log()["shared_dir"] = task.model().shared_dir_path().string();
            task.grid_log()["secure_dir"] = task.model().secure_dir_path().string();
            task.grid_log()["marker_append"] = task.model().marker_append();
            task.grid_log()["auto_gridding"] = task.model().auto_gridding();
            task.summary_channel_log();
            task.model().heatmap_writer().flush();
        } catch( const std::exception& e ) {
            task.set_grid_done(false);
            task.grid_log()["grid_fail_reason"] = e.what();
        }
        task.write_log();
        task.copy_chip_log();
        task.create_complete_file();
        return 0;
    }
private:

    __alias::crot::MarkerVec<float>             rotate_detector         ;
    __alias::crot::Calibrate                    rotate_calibrator       ;
    __alias::cmk::RegMatUm2PxRDet               reg_mat_um2px_r_det     ;
    
};

}