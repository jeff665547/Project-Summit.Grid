#pragma once
#include <ChipImgProc/aruco.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <summit/config/aruco_db.hpp>
#include "model/task.hpp"
namespace summit::app::grid {

constexpr struct ArUcoSetter {
    bool operator()(
       chipimgproc::marker::detection::ArucoRegMat& marker_detector, 
       const model::Task& task
    ) const {
        if(!task.um_to_px_r_done()) return false;

        auto& arucodb = summit::config::arucodb();
        marker_detector.set_dict(arucodb, task.db_key());
        marker_detector.set_detector_ext(
            task.pyramid_level(),
            task.border_bits(),
            task.fringe_bits(),
            task.bit_w() * task.um2px_r(),
            task.margin_size() * task.um2px_r(),
            task.frame_template().string(),
            task.frame_mask().string(),
            task.nms_count(),
            task.nms_radius() * task.um2px_r(),
            task.cell_size_px(),
            chipimgproc::aruco::Utils::aruco_ids(task.id_map()),
            chipimgproc::aruco::Utils::aruco_points(task.id_map())
        );
        return true;
    }
} aruco_setter;

}