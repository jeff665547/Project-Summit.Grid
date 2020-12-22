#pragma once
#include <ChipImgProc/aruco.hpp>
#include <ChipImgProc/marker/detection/aruco_random.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/utils.h>
#include <summit/config/aruco_db.hpp>
#include "model/task.hpp"
namespace summit::app::grid {

constexpr class ArUcoSetter {
    auto load_templ_marker(
        const std::string& tpath,
        const std::string& mpath,
        int width,
        int height,
        double scale
    ) const {
        auto flags = cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH;
        auto templ = cv::imread(tpath, flags);
        auto mask  = cv::imread(mpath, flags);
        auto size_w = scale * width  / templ.cols;
        auto size_h = scale * height / templ.rows;
        auto rtempl = chipimgproc::affine_resize(templ, size_w, size_h, cv::INTER_AREA);
        auto rmask  = chipimgproc::affine_resize(mask,  size_w, size_h, cv::INTER_NEAREST);
        return nucleona::make_tuple(
            std::move(rtempl),
            std::move(rmask)
        );
    }
public:
    auto operator()(const model::Task& task) const {
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
            summit::grid::log.warn( "******************************\n"
                                    "background auto-detection fail\n"
                                    "******************************\n");
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
        // try to print templ
        // cv::imwrite("origin_templ.tiff", templ);

        // auto& templ_meta = task.get_marker_patterns("filter", 0).at(0)->meta; 
        // auto w_um = templ_meta.at("w_um").get<int>();
        // auto h_um = templ_meta.at("h_um").get<int>();
        // auto [templ, mask] = load_templ_marker(
        //     task.frame_template().string(),
        //     task.frame_mask().string(),
        //     w_um, h_um,
        //     task.um2px_r()
        // );

        // prepare pixel domain anchors
        auto detector(cmkd::make_aruco_random(
            summit::config::arucodb_path().string(), 
            task.db_key(),
            templ, mask,
            task.aruco_width() * task.um2px_r(),
            2, 
            task.nms_count() + task.nms_tolerance(), 
            task.nms_radius() * task.um2px_r(),
            task.ext_width()
        ));

        return detector;
    }

} aruco_setter;

}