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
        auto& mk_cell = marker_patterns.begin()->second.marker.at(0);
        auto mk_h_px = mk_cell.rows * task.cell_h_um() * um2px_r;
        auto mk_w_px = mk_cell.cols * task.cell_w_um() * um2px_r;
        auto loc_mk_regs = mk_regs;
        int max_x(0), max_y(0);
        for(auto&& mk_r : loc_mk_regs) {
            mk_r.height = mk_h_px;
            mk_r.width  = mk_w_px;
            mk_r.x -= (mk_w_px / 2);
            mk_r.y -= (mk_h_px / 2);
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
} white_mk_append;

}