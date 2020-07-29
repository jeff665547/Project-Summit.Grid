#pragma once
#include "model/task.hpp"
#include <ChipImgProc/marker/detection/mk_region.hpp>
#include <ChipImgProc/marker/roi_append.hpp>
#include <ChipImgProc/utils.h>
#include <ChipImgProc/marker/view.hpp>
namespace summit::app::grid {

constexpr struct WhiteMKAppend {
    using MKRegion = chipimgproc::marker::detection::MKRegion;
    cv::Mat operator()(
        const cv::Mat&                  src,
        const std::vector<MKRegion>&    mk_regs,
        const model::Task&              task,
        float                           um2px_r
    ) const {
        auto& marker_patterns = task.marker_patterns();
        // auto& mk_cell = marker_patterns.begin()->second.marker.at(0);
        auto& mk_cell = marker_patterns.marker_type_index.begin()->second.at(0)->marker;
        auto mk_h_px = mk_cell.rows * task.cell_h_um() * um2px_r;
        auto mk_w_px = mk_cell.cols * task.cell_w_um() * um2px_r;
        auto loc_mk_regs = mk_regs;
        int max_x(0), max_y(0);
        for(auto&& mk_r : loc_mk_regs) {
            mk_r.x = std::round(mk_r.x + (mk_r.width  / 2.0) - (mk_w_px / 2.0));
            mk_r.y = std::round(mk_r.y + (mk_r.height / 2.0) - (mk_h_px / 2.0));
            mk_r.height = mk_h_px;
            mk_r.width  = mk_w_px;
            if(max_x < mk_r.x_i) max_x = mk_r.x_i;
            if(max_y < mk_r.y_i) max_y = mk_r.y_i;
        }
        cv::Mat_<std::int16_t> mk_idx(max_y + 1, max_x + 1);
        int k = 0;
        for(int i = 0; i <= max_y; i ++ ) {
            for(int j = 0; j <= max_x; j ++ ) {
                mk_idx(i, j) = k;
                k++;
            }
        }

        return chipimgproc::marker::roi_append(
            src, mk_idx, loc_mk_regs
        );
    }

    std::tuple<
        cv::Mat, 
        std::vector<MKRegion>
    > operator()(
        cv::Mat src,
        cv::Mat warp_mat,
        double  x_i,      double    y_i,
        double  spec_w,   double    spec_h,
        double  mk_w,     double    mk_h,
        double  mk_wd,    double    mk_hd,
        int     mk_r_n,   int       mk_c_n
    ) const {
        cv::Mat iwarp_mat;
        cv::Mat std_src;
        cv::invertAffineTransform(warp_mat, iwarp_mat);
        cv::warpAffine(src, std_src, iwarp_mat, cv::Size(
            std::round(spec_w), 
            std::round(spec_h)
        ));
        std::vector<MKRegion> mk_regs;
        cv::Mat_<std::int16_t> mk_map(mk_r_n, mk_c_n);
        for(int i = 0; i < mk_r_n; i ++) {
            for(int j = 0; j < mk_c_n; j ++) {
                MKRegion mkr;
                auto mk_lt_x = (j * mk_wd) + x_i;
                auto mk_lt_y = (i * mk_hd) + y_i;
                mkr.x        = std::round(mk_lt_x);
                mkr.y        = std::round(mk_lt_y);
                mkr.width    = std::round(mk_w);
                mkr.height   = std::round(mk_h);
                mkr.x_i      = j;
                mkr.y_i      = i;
                mk_regs.emplace_back(std::move(mkr));
                mk_map(i, j) = (i * mk_c_n) + j;
            }
        }
        return nucleona::make_tuple(
            chipimgproc::marker::roi_append(src, mk_map, mk_regs), 
            std::move(mk_regs)
        );

    }
} white_mk_append;

}