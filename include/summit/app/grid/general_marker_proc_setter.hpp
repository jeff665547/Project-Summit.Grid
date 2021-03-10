#pragma once
#include <ChipImgProc/utils.h>
#include <ChipImgProc/marker/txt_to_img.hpp>
#include <ChipImgProc/marker/detection/fusion_array.hpp>
#include "model/task.hpp"
#include "utils.hpp"
#include <cstdio>
namespace summit::app::grid{

constexpr class GeneralMarkerProcSetter {
public:
    auto operator()(model::Task& task) const {
        namespace cm   = chipimgproc;
        namespace cmk  = chipimgproc::marker;
        namespace cmkd = chipimgproc::marker::detection;
        
        if(!task.um_to_px_r_done()) {
            debug_throw(std::runtime_error("The information of um2px rate is required during the white channel general marker detection."));
        }

        // Prepare an image (id: 0-0) for generating general marker subarea used in the general marker detection.
        constexpr int fov_id_r = 0;
        constexpr int fov_id_c = 0;
        auto white_img_template = Utils::get_fov_mat_from(task.white_channel_imgs(), fov_id_r, fov_id_c);
        // cv::imwrite("img_template.tiff", probe_img_template);

        // Generate the marker template and its corresponding mask for the white channel (mk: rum, templ/mask: pixel).
        auto  markers      = task.get_marker_patterns("filter", 0);
        auto& mk           = *markers.at(0);
        cv::Mat_<std::uint8_t> templ, mask;
        chipimgproc::aruco::Utils::resize(mk.marker, templ, task.rum2px_r(), task.rum2px_r());
        chipimgproc::aruco::Utils::resize(mk.mask,   mask,  task.rum2px_r(), task.rum2px_r());
        
        // Generate the probe marker layout (regular array format).
        auto& fov_marker_num  = task.get_fov_marker_num(fov_id_r, fov_id_c);
        auto  general_mk_layout = cmk::make_single_pattern_reg_mat_layout(
            task.cell_h_um(), 
            task.cell_w_um(),
            task.space_um(),
            fov_marker_num.y,
            fov_marker_num.x,
            task.mk_wd_cl(),
            task.mk_hd_cl(),
            task.um2px_r()
        );
        general_mk_layout.set_single_mk_pat(
            {},
            {templ},
            {},
            {mask}
        );


        // Initialize the fusion array object.
        auto detector(cmkd::make_fusion_array(
            templ,
            mask,
            2,
            white_img_template,
            general_mk_layout
        ));

        // Set the image preprocessor to preprocess the white general marker image.
        detector.set_wh_img_preprocessor();

        // TODO: unittest.

        return detector;
    }

} gen_mk_proc_setter;

}