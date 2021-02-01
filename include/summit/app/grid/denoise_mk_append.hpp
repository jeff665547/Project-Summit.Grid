#pragma once
#include "model/task.hpp"
#include <ChipImgProc/utils.h>

namespace summit::app::grid {

constexpr struct DenoiseMKAppend {
    cv::Mat operator()(
        const cv::Mat& src,
        double         coding_size_w,    double         coding_size_h,
        double         mk_w,             double         mk_h,
        int            mk_r_n,           int            mk_c_n
    ) const {
        cv::Mat_<uint8_t> dst;
        if (src.depth() == CV_8U)
            dst = src.clone();
        else if (src.depth() == CV_16U)
            src.convertTo(dst, CV_8U, 255.0 / 16383.0);

        // Top-left point of the coding area.
        auto x_i = (mk_w - coding_size_w - 1) / 2.0;
        auto y_i = (mk_h - coding_size_h - 1) / 2.0;
        int black = 0;
        cv::Rect coding_region(0, 0, std::ceil(coding_size_w), std::ceil(coding_size_h));

        for(int i = 0; i < mk_r_n; i ++) {
            for(int j = 0; j < mk_c_n; j ++) {
                auto coding_lt_x = (j * mk_w) + x_i;
                auto coding_lt_y = (i * mk_h) + y_i;
                coding_region.x = std::round(coding_lt_x);
                coding_region.y = std::round(coding_lt_y);
                // std::cout << coding_lt_x << "/n";
                // std::cout << coding_lt_y << "/n";
                cv::rectangle(dst, coding_region, black, -1);
            }
        }
        return dst;
    }
} denoise_mk_append;

}