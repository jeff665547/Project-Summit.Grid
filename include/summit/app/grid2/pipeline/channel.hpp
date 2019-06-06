#pragma once
#include <summit/app/grid2/model.hpp>
#include <limits>
namespace summit::app::grid2::pipeline {

namespace __alias {

namespace cimp      = chipimgproc;
namespace cmk       = chipimgproc::marker;
namespace crot      = chipimgproc::rotation;
namespace cmk_det   = chipimgproc::marker::detection;
namespace cimg      = chipimgproc::gridding;

}

constexpr struct Channel {
    using Float = typename Model::Float;
    using GridLineID = typename Model::GridLineID;
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
        return nucleona::make_tuple(min_theta_diff, low_score_marker_idx);
    }
    auto gridding(model::Channel& channel) const {
        using namespace __alias;
        auto& task = channel.task();
        if(!task.rot_degree_done())
            throw std::runtime_error("[BUG]: no rotation degree, surrender gridding");
        if(!task.um_to_px_r_done())
            throw std::runtime_error("[BUG]: no um to pixel rate, surrender gridding");
        auto& executor = task.model().executor();
        auto imgs = Utils::read_imgs<std::uint16_t>(
            task.chip_dir(),
            task.fov_rows(),
            task.fov_cols(),
            channel.ch_name(),
            task.is_img_enc(),
            task.model()
        );
        // TODO: use cache
        auto fov_mk_layouts = Utils::generate_sgl_pat_reg_mat_marker_layout(
            task.um2px_r(),
            task.chipspec(),
            task.fov(),
            channel.json()
        );
        auto fov_good = channel.fov_good();

        imgs
        | ranges::view::transform([&](auto&& fov){
            try {
                auto& fov_id     = fov.first;
                auto& path       = std::get<0>(fov.second);
                auto& mat        = std::get<1>(fov.second);
                auto& mk_layout  = fov_mk_layouts.at(fov_id);
                auto& wh_mk_regs = task.fov_mk_regs().at(fov_id);
                // * determine the no rotation marker region
                auto td_lsmk     = find_and_set_best_marker(
                    mat, mk_layout, task.rot_degree().value()
                );
                auto& theta_diff = std::get<0>(td_lsmk);
                auto& ls_mk_idx  = std::get<1>(td_lsmk);
                rotate_calibrator(mat, task.rot_degree().value());
                auto  mk_regs    = cmk_det::reg_mat_no_rot(
                    mat, mk_layout,
                    cimp::MatUnit::PX, ls_mk_idx
                );
                auto& mk_des     = mk_layout.get_single_pat_marker_des();
                auto& std_mk_px  = mk_des.get_std_mk(cimp::MatUnit::PX);
                if(theta_diff > 0.5)  {
                    if(wh_mk_regs.empty()) {
                        fov_good.at(fov_id) = false;
                    } else {
                        mk_regs = wh_mk_regs;
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
                        }
                    }
                }
                // * gridding
                auto grid_res = gridder(mat, mk_layout, mk_regs);
                if(task.model().marker_append()) {
                    auto mk_append_res = cmk::roi_append(
                        mat, mk_layout, mk_regs
                    );
                }
                auto tiled_mat = cimp::TiledMat<>::make_from_grid_res(
                    grid_res, mat, mk_layout
                );
                cimp::GridRawImg<> grid_raw_img(
                    mat, grid_res.gl_x, grid_res.gl_y
                );

                auto tiles = tiled_mat.get_tiles();
                cimp::Margin<Float, GridLineID> margin;
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
                    std::cout 
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
                return 0;
            } catch(const std::exception& e) {
                std::cout << e.what() << std::endl;
                return 1;
            }
        })
        | nucleona::range::p_endp(executor)
        ;

        channel.set_fov_good(std::move(fov_good));
    }
    decltype(auto) operator()(model::Channel&& channel) const {
        // for each fov,
        // detect dark marker
        // compare theta
        // run white channel support algorithm
        try{
            gridding(channel);
            return 0;
        } catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
            return 1;
        }
    }
private:
    __alias::crot::Calibrate                    rotate_calibrator       ;
    __alias::cmk_det::RegMat                    probe_mk_detector       ;
    __alias::crot::MarkerVec<float>             rotate_detector         ;
    __alias::cimg::RegMat                       gridder                 ;
} channel;

}