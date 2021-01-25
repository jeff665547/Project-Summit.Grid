#pragma once
#include <ChipImgProc/utils.h>
#include <ChipImgProc/marker/txt_to_img.hpp>
#include <ChipImgProc/marker/detection/fusion_array.hpp>
#include "model/task.hpp"
#include "utils.hpp"
namespace summit::app::grid{

constexpr class ProbeChannelProcSetter {
public:
    auto operator()(model::Task& task, const nlohmann::json& jch) const {
        namespace cm   = chipimgproc;
        namespace cmk  = chipimgproc::marker;
        namespace cmkd = chipimgproc::marker::detection;
        
        // 
        if(!task.um_to_px_r_done()) {
            debug_throw(std::runtime_error("The information of um2px rate is required during the probe (fluorescent) marker detection."));
        }

        // Prepare an image (id: 0-0) for generating probe marker subarea used in the fluorescent marker detection.
        constexpr int fov_id_r = 0;
        constexpr int fov_id_c = 0;
        auto probe_img_template = Utils::get_fov_mat_from(task.probe_channel_imgs(), fov_id_r, fov_id_c);
        // cv::imwrite("img_template.tiff", probe_img_template);

        // Generate the marker template and its corresponding mask for the input channel (mk_pat: cell, templ/mask: pixel).
        auto  markers      = task.get_marker_patterns_by_marker_type(jch.at("marker_type"));
        auto& mk_pat       = *markers.at(0);
        auto [templ, mask] = cmk::txt_to_img(
            mk_pat.marker,
            mk_pat.mask,
            task.cell_h_um(),
            task.cell_w_um(),
            task.space_um(),
            task.um2px_r()
        );

        // Generate the probe marker layout (regular array format).
        auto& fov_marker_num  = task.get_fov_marker_num(fov_id_r, fov_id_c);
        auto  probe_mk_layout = cmk::make_single_pattern_reg_mat_layout(
            mk_pat.marker, 
            mk_pat.mask,
            task.cell_h_um(), 
            task.cell_w_um(),
            task.space_um(),
            fov_marker_num.y,
            fov_marker_num.x,
            task.mk_wd_cl(),
            task.mk_hd_cl(),
            task.um2px_r()
        );

        // Initialize the fusion array object.
        auto detector(cmkd::make_fusion_array(
            templ,
            mask,
            2,
            probe_img_template,
            probe_mk_layout
        ));

        // Set the image preprocessor to preprocess the probe (fluorescent) image.
        detector.set_pb_img_preprocessor();

        // TODO: unittest.

        return detector;
    }

} pb_ch_proc_setter;

}