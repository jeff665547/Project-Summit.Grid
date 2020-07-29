#pragma once
#include <ChipImgProc/aruco.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <summit/config/aruco_db.hpp>
#include "model/task.hpp"
namespace summit::app::grid {

constexpr struct ArUcoSetter {
    auto operator()(const model::Task& task) const {
        namespace cmkd = chipimgproc::marker::detection;

        if(!task.um_to_px_r_done()) {
            debug_throw(std::runtime_error("ArUco detection require um2px rate is satisfied"));
        }
        if(!task.support_aruco()) {
            debug_throw(std::runtime_error("Current task doesn't support ArUco"));
        };
        auto [templ, mask] = chipimgproc::aruco::create_location_marker(
            task.tm_outer_width(), 
            task.tm_inner_width(), 
            task.tm_padding(),
            task.tm_margin(), 
            task.um2px_r()
        );

        // prepare pixel domain anchors
        auto detector(cmkd::make_aruco_random(
            summit::config::arucodb_path().string(), 
            task.db_key(),
            templ, mask,
            task.aruco_width() * task.um2px_r(),
            task.pyramid_level(), 
            task.nms_count() + 3, 
            task.tm_outer_width() * task.um2px_r(),
            task.ext_width()
        ));

        return detector;
    }

} aruco_setter;

}