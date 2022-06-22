#pragma once
#include <ChipImgProc/aruco.hpp>
#include <ChipImgProc/marker/detection/aruco_random.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/utils.h>
#include <summit/config/aruco_db.hpp>
#include "model/task.hpp"
#include <cmath>
namespace summit::app::grid {

constexpr class CustomArUcoSetter {
public:
    auto draw_mk_pos(
        const cv::Mat& input,
        const cv::Mat& templ,
        const std::vector<cv::Point2d>& marker_center_pos
    ) const {
        int mark_radius = sqrt(pow(templ.cols/2, 2) + pow(templ.rows/2, 2));
        cv::Mat image = input.clone();
        for(auto mk: marker_center_pos) {
            cv::circle(image, mk, mark_radius, cv::Scalar(0), -1);
        }
        return image;
    }
    auto operator()(const model::Task& task, const int& remaining_mk_ct) const {
        namespace cmkd = chipimgproc::marker::detection;

        if(!task.um_to_px_r_done()) {
            debug_throw(std::runtime_error("ArUco detection require um2px rate is satisfied"));
        }
        if(!task.support_aruco()) {
            debug_throw(std::runtime_error("Current task doesn't support ArUco"));
        };

        // ***** for brighter background of aruco2 ***** 
        double sum(0.0);
        for (auto&& [_, tuple] : task.white_channel_imgs())
            sum += cv::sum(std::get<1>(tuple))[0];
        
        auto&& mat(std::get<1>(task.white_channel_imgs().cbegin()->second));
        sum /= mat.rows * mat.cols * task.white_channel_imgs().size();
        if (sum > 256 || sum < 0)
        {
            summit::grid::log.warn( "{}:{}\n"
                                    "******************************\n"
                                    "background auto-detection fail\n"
                                    "******************************\n", 
                                    __FILE__, __LINE__);
        }
        std::uint8_t bg_color(std::floor(static_cast<std::uint8_t>(sum)));
        // ***** end *****

        auto [templ, mask] = chipimgproc::aruco::create_location_marker(
            task.tm_outer_width(), 
            task.tm_inner_width(), 
            task.tm_padding(),
            task.tm_margin(), 
            task.um2px_r(), 
            bg_color
        );

        int mark_radius = sqrt(pow(templ.cols/2, 2) + pow(templ.rows/2, 2));

        // prepare pixel domain anchors
        auto detector(cmkd::make_aruco_random(
            summit::config::arucodb_path().string(), 
            task.db_key(),
            templ, mask,
            task.aruco_width() * task.um2px_r(),
            2, 
            task.ref_ch_theor_max_val(),
            remaining_mk_ct + task.nms_tolerance(), 
            task.nms_radius() / 3.0 * task.um2px_r(),
            task.ext_width()
        ));
        return detector;
    }
} custom_aruco_setter;

}