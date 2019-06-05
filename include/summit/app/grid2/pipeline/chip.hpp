#pragma once
#include <summit/app/grid2/model.hpp>
#include <Nucleona/range.hpp>
#include "channel.hpp"
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/rotation/marker_vec.hpp>
#include <ChipImgProc/rotation/iteration_cali.hpp>
#include <ChipImgProc/marker/reg_mat_um2px_r_det.hpp>
#include <summit/app/grid2/aruco_setter.hpp>
#include <Nucleona/util/remove_const.hpp>
#include <summit/app/grid2/model/marker_base.hpp>

namespace summit::app::grid2::pipeline {
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
        cmk_det::ArucoRegMat    aruco_mk_detector               ;
        auto                    aruco_iter_rot_cali = crot::make_iteration_cali(
            [&, this](const cv::Mat& mat) {
                auto mk_regs    = aruco_mk_detector(mat, 0, 0, std::cout);
                auto theta      = rotate_detector(mk_regs, std::cout);
                return theta;
            },
            [&, this](cv::Mat& mat, auto theta) {
                rotate_calibrator(mat, theta /*debug viewer*/);
            }
        );
        auto& executor = task.get_model().executor();
        
        task.set_white_channel_imgs(Utils::read_white_channel(
            task.channels(),
            task.chip_dir(),
            task.fov_rows(),
            task.fov_cols(),
            task.is_img_enc(),
            task.model()
        ));
        if(task.white_channel_imgs().size() == 0) return false;
        if(!aruco_setter(nucleona::remove_const(aruco_mk_detector), task)) return false;

        auto fov_marker_num = Utils::generate_fov_marker_num(
            task.chipspec(), task.fov()
        );
        std::vector<float> rot_degs (task.white_channel_imgs().size());
        std::vector<float> um2px_rs (task.white_channel_imgs().size());
        std::vector<bool>  success  (task.white_channel_imgs().size());
        Utils::FOVMarkerRegionMap fov_marker_regs;
        for(auto&& [fov_id, mat] : task.white_channel_imgs()) {
            fov_marker_regs[fov_id] = {};
        }

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
                auto theta      = aruco_iter_rot_cali(mat, std::cout);
                std::cout << "white channel detect theta " 
                    << fov_id << ": " << theta << std::endl;
                cv::Mat mat_loc = mat.clone();
                rotate_calibrator(mat_loc, theta);

                // * detect marker regions
                auto mk_regs = aruco_mk_detector(mat_loc, 0, 0, std::cout);
                mk_regs = __alias::cmk_det::reg_mat_infer(mk_regs, mk_num.y, mk_num.x);

                // * detect um2px_r
                auto um2px_r = reg_mat_um2px_r_det(
                    mk_regs, 
                    task.mk_wd_um(),
                    task.mk_hd_um(),
                    std::cout 
                );
                std::cout << "white channel detect um2px rate " 
                    << fov_id << ": " << um2px_r << std::endl;

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
        return true;
    }
    bool dark_channel_proc(model::Task& task) const {
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
        auto  pbmk_iter_rot_cali = crot::make_iteration_cali(
            [&, this](const cv::Mat& mat) {
                auto mk_regs    = probe_mk_detector(
                    static_cast<const cv::Mat_<std::int16_t>&>(mat), 
                    probe_mk_layout, 
                    __alias::cimp::MatUnit::PX, 0,
                    std::cout
                );
                auto theta      = rotate_detector(mk_regs, std::cout);
                return theta;
            },
            [&, this](cv::Mat& mat, auto theta) {
                rotate_calibrator(mat, theta /*debug viewer*/);
            }
        );

        // * marker detection and rotate
        auto theta      = pbmk_iter_rot_cali(mat, std::cout);
        std::cout << "probe channel detect theta: " << theta << std::endl;
        cv::Mat mat_loc = mat.clone();
        rotate_calibrator(mat_loc, theta);

        // * um2px_r auto scaler

        return true;
    }
    decltype(auto) operator()(model::Task& task) const {
        using namespace __alias;
        auto& executor = task.get_model().executor();

        if(!white_channel_proc(task)) {
            task.set_white_ch_proc_failed(true);
            if(!task.um_to_px_r_done()) {
                std::cout << "no way to evaluate um2px_r, surrender chip\n";
                return 1;
            } 
            if(!dark_channel_proc(task)) {
                std::cout << "unknown failed\n";
                return 1;
            }
        }
        task.channels()
        | ranges::view::transform([&task](auto& jch){
            model::Channel ch_mod;
            ch_mod.set_task(task);
            ch_mod.set_channel(jch);
            return ch_mod;
        })
        | ranges::view::transform(channel)
        | nucleona::range::p_endp(executor)
        ;
        return 0;
    }
private:

    __alias::crot::MarkerVec<float>             rotate_detector         ;
    __alias::crot::Calibrate                    rotate_calibrator       ;
    __alias::cmk::RegMatUm2PxRDet               reg_mat_um2px_r_det     ;
    
};

}