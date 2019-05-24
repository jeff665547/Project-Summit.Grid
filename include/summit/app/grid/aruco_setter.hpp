#pragma once
#include <ChipImgProc/aruco.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <summit/config/aruco_db.hpp>
#include "chip_props.hpp"
namespace summit::app::grid {

constexpr struct ArUcoSetter {
    bool operator()(
       chipimgproc::marker::detection::ArucoRegMat& marker_detector, 
       const ChipProps& chip_props,
       const nlohmann::json& chip_log 
    ) const {
        auto um2px_r_itr = chip_log.find("um_to_px_coef");
        if(um2px_r_itr == chip_log.end()) return false;
        double um2px_r = *um2px_r_itr;
        const auto& chip                = chip_log["chip"];
        const auto& chip_spec           = chip_props.chip_spec();
        const auto& chip_aruco_marker   = chip_spec["aruco_marker"];
        const auto& chip_origin_infer   = chip["origin_infer"];
        const auto& chip_mk_id_map      = chip_aruco_marker["id_map"];
        const auto& db_key              = chip_origin_infer["db_key"];
        const auto& shooting_marker     = chip_spec["shooting_marker"];
        const auto& marker_pos          = shooting_marker["position"];

        auto& arucodb = summit::config::arucodb();
        marker_detector.set_dict(arucodb, db_key);
        marker_detector.set_detector_ext(
            chip_origin_infer["pyramid_level"],
            chip_aruco_marker["border_bits"],
            chip_aruco_marker["fringe_bits"],
            chip_aruco_marker["bit_w"].get<double>() * um2px_r,
            chip_aruco_marker["margin_size"].get<double>() * um2px_r,
            (summit::install_path() / chip_aruco_marker["frame_template"].get<std::string>()).string(),
            (summit::install_path() / chip_aruco_marker["frame_mask"].get<std::string>()).string(),
            chip_origin_infer["nms_count"],
            chip_aruco_marker["nms_radius"].get<double>() * um2px_r,
            chip_origin_infer["cell_size_px"],
            chipimgproc::aruco::Utils::aruco_ids(chip_mk_id_map),
            chipimgproc::aruco::Utils::aruco_points(chip_mk_id_map)
        );
        return true;
    }
} aruco_setter;

}