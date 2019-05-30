#pragma once
#include <optional>
#include "chip_props.hpp"
#include "utils.hpp"
#include "aruco_setter.hpp"
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/rotation/marker_vec.hpp>
#include <ChipImgProc/rotation/iteration_cali.hpp>
#include <ChipImgProc/marker/reg_mat_um2px_r_det.hpp>
namespace summit::app::grid {

constexpr struct WhiteChannelProc {
    template<class Rng>
    auto mean(Rng&& rng) const {
        using Value = nucleona::range::ValueT<std::decay_t<Rng>>;
        Value sum(0);
        for(auto&& v : rng) {
            sum += v;
        }
        return sum / ranges::distance(rng);
    }
    void std_reg_mat(
        std::vector<chipimgproc::marker::detection::MKRegion>& mks
    ) const {
        namespace cmkdet = chipimgproc::marker::detection;
        cmkdet::MKRegion::Group<cmkdet::MKRegion*> x_group;
        cmkdet::MKRegion::Group<cmkdet::MKRegion*> y_group;
        for(auto&& r : mks ) {
            x_group[r.x_i].push_back(&r);
        }
        for(auto&& r : mks ) {
            y_group[r.y_i].push_back(&r);
        }
        
        for(auto&& [dir_id, dir_vec] : x_group ) {
            auto x_mean = mean(dir_vec | nucleona::range::transformed([](auto&& mk_r){
                return mk_r->x;
            }));
            for(auto&& mk_r : dir_vec) {
                mk_r->x = x_mean;
            }
        }
        for(auto&& [dir_id, dir_vec] : y_group ) {
            auto y_mean = mean(dir_vec | nucleona::range::transformed([](auto&& mk_r){
                return mk_r->y;
            }));
            for(auto&& mk_r : dir_vec) {
                mk_r->y = y_mean;
            }
        }
    }
    template<class Exetor, class Mats>
    void core(
        Exetor&                 exe_tor,
        const Mats&             mats, 
        ChipProps&              chip_props,
        const nlohmann::json&   chip_log
    ) const {
        namespace cimp      = chipimgproc;
        namespace cmk       = chipimgproc::marker;
        namespace crot      = chipimgproc::rotation;
        namespace cmk_det   = chipimgproc::marker::detection;

        cmk_det::ArucoRegMat     marker_detector        ;
        crot::MarkerVec<float>   rotate_detector        ;
        crot::Calibrate          rotate_calibrator      ;
        cmk::RegMatUm2PxRDet     reg_mat_um2px_r_det    ;
        auto iter_rot_cali   = crot::make_iteration_cali(
            [   
                &rotate_detector,
                &marker_detector
            ](const cv::Mat& mat) {
                auto mk_regs    = marker_detector(mat, 0, 0, std::cout);
                auto theta      = rotate_detector(mk_regs, std::cout);
                return theta;
            },
            [
                &rotate_calibrator
            ](cv::Mat& mat, auto theta) {
                rotate_calibrator(mat, theta /*debug viewer*/);
            }
        );

        if(!aruco_setter(marker_detector, chip_props, chip_log)) return;
        auto fov_marker_num = Utils::generate_fov_marker_num(
            chip_props.chip_spec(),
            chip_props.cell_fov()
        );
        auto mk_pos = chip_props.chip_spec()["shooting_marker"]["position"];
        float mk_wd_um = mk_pos["w_d"];
        float mk_hd_um = mk_pos["h_d"];
        std::vector<float> rot_degs(mats.size());
        std::vector<float> um2px_rs(mats.size());
        Utils::FOVMarkerRegionMap fov_marker_regs;
        for(auto&& [fov_id, mat] : mats) {
            fov_marker_regs[fov_id] = {};
        }
        
        std::vector<typename Exetor::template Future<bool>> futs;
        for(auto&& [i, fov_id_mat] : mats | nucleona::range::indexed()) {
            auto p_mat = &fov_id_mat.second;
            auto p_fov_id = &fov_id_mat.first;
            futs.push_back(exe_tor.submit([&, i, p_mat, p_fov_id](){
                try {
                    auto& mat = *p_mat;
                    auto& fov_id = *p_fov_id;
                    auto& mk_num = fov_marker_num.at(fov_id);
                    // * count theta
                    auto theta   = iter_rot_cali(mat, std::cout);
                    std::cout << "white channel detect theta [" 
                        << fov_id << "]: " << theta << std::endl;
                    cv::Mat mat_loc = mat.clone();
                    rotate_calibrator(mat_loc, theta);

                    // * detect marker regions
                    auto mk_regs = marker_detector(mat_loc, 0, 0, std::cout);
                    mk_regs = cmk_det::reg_mat_infer(mk_regs, mk_num.y, mk_num.x);

                    // * detect um2px_r
                    auto um2px_r = reg_mat_um2px_r_det(
                        mk_regs, 
                        mk_wd_um,
                        mk_hd_um,
                        std::cout 
                    );
                    std::cout << "white channel detect um2px rate [" 
                        << fov_id << "]: " << um2px_r << std::endl;

                    std_reg_mat(mk_regs);
                    fov_marker_regs[fov_id] = std::move(mk_regs);
                    rot_degs[i] = theta;
                    um2px_rs[i] = um2px_r;
                    return true;
                } catch(...) {
                    return false;
                }
            }));
        }
        for(auto& f : futs) {
            if(!f.sync()) 
                throw std::runtime_error("white channel process failed, probably bad aruco detection");
        }
        // * consensus
        auto rot_deg = mean(rot_degs);
        auto um2px_r = mean(um2px_rs);
        chip_props.set_rot_est_result(rot_deg);
        chip_props.set_um2px_r(um2px_r);
        chip_props.set_fov_mk_regs(std::move(fov_marker_regs));
    }

    template<class Exetor, class Mats>
    void operator()(
        Exetor&                     exe_tor,
        const Mats&                 mats, 
        ChipProps&                  chip_props,
        const nlohmann::json&       chip_log
    ) const {
        core(exe_tor, mats, chip_props, chip_log);
    }
} white_channel_proc;

}