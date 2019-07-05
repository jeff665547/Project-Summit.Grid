#pragma once
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <summit/app/grid/utils.hpp>
#include "single_img_proc_res.hpp"
#include "task.hpp"
#include "type.hpp"
namespace summit::app::grid::model {

constexpr struct MakeMultiTiledMat {
    template<class ImgProcRes>
    auto operator()(
        Utils::FOVMap<ImgProcRes>& fov_imp_res, 
        const Task& task
    ) const {
        std::vector<chipimgproc::TiledMat<GLID>>  mats;
        std::vector<chipimgproc::stat::Mats<Float>>     stats;
        std::vector<cv::Point>                          cell_st_pts;
        std::vector<cv::Point>                          fov_ids;

        auto st_points = Utils::generate_stitch_points(task.fov());
        for(auto&& p : fov_imp_res) {
            auto& fov_id = p.first;
            auto& single_img_proc_res = p.second;
            auto& st_point = st_points.at(fov_id);
            fov_ids.push_back(fov_id);
            mats.emplace_back(
                std::move(single_img_proc_res.take_tiled_mat())
            );
            cell_st_pts.emplace_back(std::move(st_point));
            stats.emplace_back(
                std::move(single_img_proc_res.take_stat_mats())
            );
        }
        return chipimgproc::MultiTiledMat<Float, GLID>(
            mats, stats, cell_st_pts, fov_ids
        );
    }
} make_multi_tiled_mat;

}