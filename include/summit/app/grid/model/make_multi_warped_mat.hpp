#pragma once
#include <ChipImgProc/multi_warped_mat.hpp>
#include "task.hpp"
#include <summit/app/grid/utils.hpp>

namespace summit::app::grid {


constexpr struct MakeMultiWarpedMat {
    template<class ImgProcRes>
    auto operator()(
        const Utils::FOVMap<ImgProcRes>& fov_imp_res,
        const model::Task& task
    ) const {
        using WarpedMat = typename ImgProcRes::WarpedMat;
        std::vector<WarpedMat> fovs;
        std::vector<cv::Point2d> st_pts;
        for(auto&& [fov_id, fov_mod] : fov_imp_res) {
            fovs.push_back(fov_mod.warped_mat());
            st_pts.emplace_back(
                fov_id.x * (task.fov_wd() * task.cell_wd_rum()),
                fov_id.y * (task.fov_hd() * task.cell_hd_rum())
            );
        }
        return chipimgproc::make_multi_warped_mat(
            std::move(fovs), std::move(st_pts),
            {task.xi_rum(), task.yi_rum()},
            task.cell_wd_rum(), task.cell_hd_rum(),
            task.cell_wd_rum() * ((task.fov_wd() * task.fov_cols()) + task.mk_w_cl()),
            task.cell_hd_rum() * ((task.fov_hd() * task.fov_rows()) + task.mk_h_cl())
        );

    }
} make_multi_warped_mat;
    
} // namespace summit::app::grid
