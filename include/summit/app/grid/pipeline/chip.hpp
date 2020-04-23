/**
 * @file chip.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::pipeline::Chip
 */
#pragma once
#include "channel.hpp"
#include <summit/app/grid/model.hpp>
#include <summit/app/grid/aruco_setter.hpp>
#include <summit/app/grid/model/marker_base.hpp>
#include <summit/app/grid/white_mk_append.hpp>
#include <summit/grid/version.hpp>
#include <ChipImgProc/algo/um2px_auto_scale.hpp>
#include <ChipImgProc/rotation/marker_vec.hpp>
#include <ChipImgProc/rotation/iteration_cali.hpp>
#include <ChipImgProc/marker/detection/aruco_reg_mat.hpp>
#include <ChipImgProc/marker/reg_mat_um2px_r_det.hpp>
#include <ChipImgProc/marker/cell_layout.hpp>
#include <ChipImgProc/marker/detection/reg_mat_infer.hpp>
#include <Nucleona/range.hpp>
#include <Nucleona/util/remove_const.hpp>
namespace summit::app::grid::pipeline {
namespace __alias {

namespace cimp      = chipimgproc;
namespace cmk       = chipimgproc::marker;
namespace crot      = chipimgproc::rotation;
namespace cmk_det   = chipimgproc::marker::detection;

}
/**
 * @brief Chip level process, evaluate chip-wide parameters
 *        (e.g. rotation degree and micron to pixel rate) and 
 *        collect chip-wide result.
 * @details The workflow diagram 
 *          @image html chip-level-process.png
 *          @image latex chip-level-process
 *          See @ref chip-level-process "Chip level process" for more details
 */
struct Chip {
    /**
     * @brief Construct a new Chip object
     */
    Chip() 
    : rotate_detector       () 
    , rotate_calibrator     () 
    , reg_mat_um2px_r_det   () 
    {}
    /**
     * @brief White channel image process, 
     *        evaluate rotation degree and micron to pixel rate.
     * @details The workflow diagram
     *          @image html white-channel-image-process.png
     *          @image latex white-channel-image-process
     * @param task chip parameter model.
     * @return true Process success
     * @return false Process failed, possible reason: 
     *  1. No white channel
     *  2. Not ArUco marker
     *  3. Bad image quality (process failed)
     */
    bool white_channel_proc(model::Task& task) const {
        namespace nr = nucleona::range;
        using namespace __alias;
        
        task.set_white_channel_imgs(Utils::read_white_channel(
            task.channels(),
            task.chip_dir(),
            task.fov_rows(),
            task.fov_cols(),
            task.is_img_enc(),
            task.model()
        ));
        if(task.white_channel_imgs().size() == 0) return false;
        if(!white_channel_proc_aruco(task)) {
            return white_channel_proc_general(task);
        } else {
            return true;
        }
    }
    bool white_channel_proc_aruco(model::Task& task) const {
        namespace nr = nucleona::range;
        using namespace __alias;
        cmk_det::ArucoRegMat    aruco_mk_detector       ;
        auto& executor  = task.model().executor();
        auto& model     = task.model();
        
        if(!aruco_setter(aruco_mk_detector, task)) return false;

        auto& fov_marker_num = task.fov_marker_num();
        std::vector<float> rot_degs (task.white_channel_imgs().size());
        std::vector<float> um2px_rs (task.white_channel_imgs().size());
        std::vector<bool>  success  (task.white_channel_imgs().size());
        Utils::FOVMarkerRegionMap fov_marker_regs;
        Utils::FOVMap<cv::Mat>    fov_mk_append;
        for(auto&& [fov_id, mat] : task.white_channel_imgs()) {
            fov_marker_regs[fov_id] = {};
            fov_mk_append[fov_id] = cv::Mat();
        }
        auto aruco_mk_det = [&, this](
            const cv::Mat& mat, 
            const cv::Point& fov_id, 
            double um2px_r
        ) {
            auto fov_mk_num = fov_marker_num.at(fov_id);
            auto mk_regs = aruco_mk_detector(
                static_cast<const cv::Mat_<std::uint8_t>&>(mat), 
                task.mk_w_px(um2px_r),     task.mk_h_px(um2px_r)
            );
            // __alias::cmk_det::filter_low_score_marker(mk_regs);
            return mk_regs;
        };
        auto aruco_iter_rot_cali = crot::make_iteration_cali(
            [&, this](
                const cv::Mat& mat, 
                const cv::Point& fov_id, 
                double um2px_r
            ) {
                auto mk_regs = aruco_mk_det(mat, fov_id, um2px_r);
                auto theta   = rotate_detector(mk_regs, nucleona::stream::null_out);
                summit::grid::log.debug("theta: {}", theta);
                return theta;
            },
            [&, this](cv::Mat& mat, auto theta) {
                rotate_calibrator(mat, theta /*debug viewer*/);
            }
        );

        summit::grid::log.debug("white channel image nums: {}", task.white_channel_imgs().size());

        task.white_channel_imgs()
        | nucleona::range::indexed()
        | ranges::view::transform([&](auto&& p){
            auto& i             = p.first;
            auto& fov_id_mat    = p.second;
            auto& fov_id        = fov_id_mat.first;
            auto& path          = std::get<0>(fov_id_mat.second);
            auto& mat           = std::get<1>(fov_id_mat.second);
            summit::grid::log.debug("white channel, fov id:({}, {}) start process", fov_id.x, fov_id.y);
            try {
                auto& mk_num    = fov_marker_num.at(fov_id);

                // * count theta
                auto theta      = aruco_iter_rot_cali(
                                    mat, 
                                    nucleona::make_tuple(
                                        fov_id, task.um2px_r()
                                    )
                                  );
                summit::grid::log.debug("white channel, fov id:({}, {}) theta: {}", fov_id.x, fov_id.y, theta);
                // std::cout << "white channel detect theta " 
                //     << fov_id << ": " << theta << std::endl;
                cv::Mat mat_loc = mat.clone();
                rotate_calibrator(mat_loc, theta);

                // * detect marker regions
                auto mk_regs = aruco_mk_det(mat_loc, fov_id, task.um2px_r());
                auto aruco_ch_mk_seg_view = task.aruco_ch_mk_seg_view(fov_id.y, fov_id.x);
                if(aruco_ch_mk_seg_view)
                    aruco_ch_mk_seg_view(chipimgproc::marker::view(mat_loc, mk_regs));
                mk_regs = __alias::cmk_det::reg_mat_infer(mk_regs, mk_num.y, mk_num.x);

                // * detect um2px_r
                auto um2px_r = reg_mat_um2px_r_det(
                    mk_regs, 
                    task.mk_wd_um(),
                    task.mk_hd_um(),
                    nucleona::stream::null_out
                );
                summit::grid::log.debug("white channel, fov id:({}, {}) um2px_r: {}", fov_id.x, fov_id.y, um2px_r);
                mk_regs = aruco_mk_det(mat_loc, fov_id, um2px_r);
                mk_regs = __alias::cmk_det::reg_mat_infer(mk_regs, mk_num.y, mk_num.x);

                if(model.marker_append()) {
                    auto fov_wh_mk_append = white_mk_append(
                        mat_loc, mk_regs, task, um2px_r
                    );
                    // cv::imwrite(
                    //     fmt::format("{}-{}-mk_ap.tiff", fov_id.x, fov_id.y), 
                    //     fov_wh_mk_append
                    // );
                    fov_mk_append.at(fov_id) = fov_wh_mk_append.clone();
                }
                // std::cout << "white channel detect um2px rate " 
                //     << fov_id << ": " << um2px_r << std::endl;

                // std_reg_mat(mk_regs);
                fov_marker_regs.at(fov_id) = std::move(mk_regs);
                rot_degs.at(i) = theta;
                um2px_rs.at(i) = um2px_r;
                success.at(i)  = true;

            } catch (...) {
                success.at(i) = false;
            }
            summit::grid::log.debug("white channel, fov id:({}, {}) end process", fov_id.x, fov_id.y);
            return 0;
        })
        | nucleona::range::p_endp(executor)
        ;

        summit::grid::log.debug("white channel FOVs process done");

        std::size_t success_num = 0;
        for(auto f : success) {
            if(f) success_num ++;
        }
        if(success_num == 0) return false;

        // * consensus
        rot_degs = rot_degs 
            | nr::indexed() 
            | ranges::view::filter([&](auto&& p ){ return success.at(p.first); }) 
            | nr::transformed([&](auto&& p){ return p.second; })
            | ranges::to_vector
        ;
        um2px_rs = um2px_rs
            | nr::indexed() 
            | ranges::view::filter([&](auto&& p ){ return success.at(p.first); }) 
            | nr::transformed([&](auto&& p){ return p.second; })
            | ranges::to_vector
        ;
        auto rot_deg = Utils::mean(rot_degs);
        auto um2px_r = Utils::mean(um2px_rs);
        task.set_rot_degree(rot_deg);
        task.set_um2px_r(um2px_r);
        task.set_fov_mk_regs(std::move(fov_marker_regs));
        if(model.marker_append()) {
            task.collect_fovs_mk_append(fov_mk_append);
        }
        return true;
    }
    bool white_channel_proc_general(model::Task& task) const {
        namespace nr = nucleona::range;
        using namespace __alias;
        cmk_det::RegMat mk_detector;
        int  sel_fov_row = task.fov_rows() / 2;
        int  sel_fov_col = task.fov_cols() / 2;
        auto [ch_i, ch] = task.white_channel();
        auto [img_path, mat] = task.white_channel_imgs().at(cv::Point(sel_fov_col, sel_fov_row));
        auto  mks                = task.get_marker_patterns(
                                    "filter", 0
                                   );
        auto& marker             = mks.at(0)->marker;
        auto& mask               = mks.at(0)->mask;
        auto& fov_marker_num     = task.get_fov_marker_num(sel_fov_row, sel_fov_col);
        auto mk_layout           = cmk::make_single_pattern_reg_mat_layout(
            marker, mask,
            task.cell_h_um(), task.cell_w_um(),
            task.space_um(),
            fov_marker_num.y,
            fov_marker_num.x,
            task.mk_wd_cl(),
            task.mk_hd_cl(),
            task.um2px_r()
        );
        std::vector<cv::Point>   low_score_marker_idx;
        auto mk_rot_cali = crot::make_iteration_cali(
            [&, this](const cv::Mat& mat) {
                auto mk_regs = mk_detector(
                    static_cast<const cv::Mat_<std::int8_t>&>(mat), 
                    mk_layout, 
                    __alias::cimp::MatUnit::PX, 0,
                    nucleona::stream::null_out
                );
                low_score_marker_idx = cmk_det::filter_low_score_marker(mk_regs);
                auto theta      = rotate_detector(mk_regs, nucleona::stream::null_out);
                return theta;
            },
            [&, this](cv::Mat& mat, auto theta) {
                rotate_calibrator(mat, theta /*debug viewer*/);
            }
        );
        // * marker detection and rotate
        auto theta      = mk_rot_cali(mat);
        // std::cout << "probe channel detect theta: " << theta << std::endl;
        cv::Mat mat_loc = mat.clone();
        rotate_calibrator(mat_loc, theta);

        // * um2px_r auto scaler
        cimp::algo::Um2PxAutoScale auto_scaler(
            mat_loc, 
            task.cell_w_um(), task.cell_h_um(),
            task.space_um()
        );
        auto [best_um2px_r, score_mat] = auto_scaler.linear_steps(
            mk_layout, task.um2px_r(), 0.002, 7,
            low_score_marker_idx, nucleona::stream::null_out
        );
        task.set_rot_degree(theta);
        task.set_um2px_r(best_um2px_r);
        return true;

    }
    /**
     * @brief Probe channel image process, 
     *        similar to white_channel_proc but use probe marker detection.
     * @param task chip parameter model.
     * @return true Process finished normally.
     * @return false Current implementation won't return false.
     */
    bool probe_channel_proc(model::Task& task) const {
        using namespace __alias;
        cmk_det::RegMat probe_mk_detector       ;
        int  sel_fov_row = task.fov_rows() / 2;
        int  sel_fov_col = task.fov_cols() / 2;
        auto [ch_i, ch]  = task.first_probe_channel();
        auto [mat, img_path] = Utils::read_img<std::uint16_t>(
            task.chip_dir(),
            sel_fov_row, sel_fov_col,
            ch["name"].get<std::string>(),
            task.is_img_enc(),
            task.model()
        );
        auto mks                 = task.get_marker_patterns_by_marker_type(ch.at("marker_type"));
        auto& marker             = mks.at(0)->marker;
        auto& mask               = mks.at(0)->mask;
        auto& fov_marker_num     = task.get_fov_marker_num(sel_fov_row, sel_fov_col);
        auto  probe_mk_layout    = cmk::make_single_pattern_reg_mat_layout(
            marker, mask,
            task.cell_h_um(), task.cell_w_um(),
            task.space_um(),
            fov_marker_num.y,
            fov_marker_num.x,
            task.mk_wd_cl(),
            task.mk_hd_cl(),
            task.um2px_r()
        );
        std::vector<cv::Point>   low_score_marker_idx;
        auto  pbmk_iter_rot_cali = crot::make_iteration_cali(
            [&, this](const cv::Mat& mat) {
                auto mk_regs    = probe_mk_detector(
                    static_cast<const cv::Mat_<std::int16_t>&>(mat), 
                    probe_mk_layout, 
                    __alias::cimp::MatUnit::PX, 0,
                    nucleona::stream::null_out
                );
                low_score_marker_idx = cmk_det::filter_low_score_marker(mk_regs);
                auto theta      = rotate_detector(mk_regs, nucleona::stream::null_out);
                return theta;
            },
            [&, this](cv::Mat& mat, auto theta) {
                rotate_calibrator(mat, theta /*debug viewer*/);
            }
        );

        // * marker detection and rotate
        auto theta      = pbmk_iter_rot_cali(mat);
        // std::cout << "probe channel detect theta: " << theta << std::endl;
        cv::Mat mat_loc = mat.clone();
        rotate_calibrator(mat_loc, theta);

        // * um2px_r auto scaler
        cimp::algo::Um2PxAutoScale auto_scaler(
            mat_loc, 
            task.cell_w_um(), task.cell_h_um(),
            task.space_um()
        );
        auto [best_um2px_r, score_mat] = auto_scaler.linear_steps(
            probe_mk_layout, task.um2px_r(), 0.002, 7,
            low_score_marker_idx, nucleona::stream::null_out
        );
        task.set_rot_degree(theta);
        task.set_um2px_r(best_um2px_r);
        return true;
    }
    /**
     * @brief Draw grid line on image
     * 
     * @tparam T Grid line position type
     * @param mat Image
     * @param gl_x Grid line position along x dimension
     * @param gl_y Grid line position along y dimension
     * @param line_value Line gray color value
     */
    template<class T>
    void draw_grid(
        cv::Mat& mat, 
        const std::vector<T>& gl_x, 
        const std::vector<T>& gl_y, 
        int line_value
    ) const {
        for(auto&& xl : gl_x) {
            cv::line(mat,
                cv::Point(xl, 0), 
                cv::Point(xl, mat.rows),
                line_value
            );
        }
        for(auto&& yl : gl_y) {
            cv::line(mat,
                cv::Point(0, yl), 
                cv::Point(mat.cols, yl),
                line_value
            );
        }
    }
    /**
     * @brief Create debug grid line stitched images and write to filesystem.
     * 
     * @param task chip parameter model
     */
    void gridline_debug_image_proc(model::Task& task) const { 
        chipimgproc::stitch::GridlineBased gl_stitcher;
        std::map<std::string, const nlohmann::json*> channel_params;
        /*
         * Find white channel name
         */
        std::string wh_name = "";
        for(auto&& ch : task.channels()) {
            channel_params[ch.at("name").get<std::string>()] = &ch;
            if(ch.at("filter").get<int>() == 0) {
                wh_name = ch.at("name").get<std::string>();
            }
        }
        /*
         * Filter id to color mapping, 0 = blue, 1 = green, 2 = red
         */
        std::vector<int> filter_to_color({0, 0, 1, 0, 2, 0});
        /*
         * Create white channel stitched image.
         */
        if(!wh_name.empty()) {
            auto tpl_mtm = task.multi_tiled_mat().begin()->second.value();
            for(auto&& [fov_id, img_data] : task.white_channel_imgs()) {
                auto& [path, img] = img_data;
                cv::Mat img_loc = img.clone();
                rotate_calibrator(img_loc, task.rot_degree().value());
                auto& tpl_gri = tpl_mtm.get_fov_img(fov_id.x, fov_id.y);
                tpl_gri.mat() = img_loc;
            }
            auto gl_wh_stitch = gl_stitcher(tpl_mtm);
            task.set_stitched_img(wh_name, std::move(gl_wh_stitch));
        }

        /*
         * Stitch each probe channel image and write to files.
         * White channel convert 8 bit to 16 bit.
         */
        std::vector<cv::Mat> channels;
        channels.resize(task.stitched_img().size());
        for(auto& [chid, st_img] : task.stitched_img()) {
            auto loc_st_img = st_img.mat().clone();
            if(chid == wh_name) {
                cv::Mat tmp;
                loc_st_img.convertTo(tmp, CV_16UC1, 256);
                loc_st_img = tmp;
            } else {
                loc_st_img = chipimgproc::viewable(loc_st_img, 5000);
            }
            draw_grid(loc_st_img, st_img.gl_x(), st_img.gl_y(), 32767);
            auto ch_filter = channel_params.at(chid)->at("filter").get<int>();
            channels.at(filter_to_color.at(ch_filter)) = loc_st_img;
            cv::imwrite(task.debug_stitch(chid).string(), loc_st_img);
        }
        /*
         * Merge all channel and write to file.
         */
        cv::Mat stitched_grid_all;
        cv::merge(channels, stitched_grid_all);
        cv::imwrite(task.debug_stitch("merged").string(), stitched_grid_all);
    }
    /**
     * @brief Run chip level process.
     * @details See @ref chip-level-process "Chip level process" for more details.
     * @param task chip parameter model
     * @return decltype(auto) exit code
     */
    decltype(auto) operator()(model::Task& task) const {
        using namespace __alias;
        auto& model = task.model();
        auto& executor = task.model().executor();
        try {
            auto timer = nucleona::proftool::make_timer([&](auto&& du){
                auto du_ms = std::chrono::duration_cast<std::chrono::milliseconds>(du).count();
                task.set_proc_time(du_ms / 1000.0);
            });
            task.grid_log()["date"] = summit::utils::datetime("%Y/%m/%d %H:%M:%S", std::chrono::system_clock::now());
            auto& wh_ch_log = task.grid_log()["white_channel_proc"];
            if(task.model().auto_gridding()) {
                if(!white_channel_proc(task)) {
                    task.set_white_ch_proc_failed(true);
                    wh_ch_log = false;
                    auto& probe_ch_log = task.grid_log()["probe_channel_proc"];
                    if(!task.um_to_px_r_done()) {
                        throw std::runtime_error("no on spec um to pixel rate provided");
                    } 
                    if(!probe_channel_proc(task)) {
                        probe_ch_log = false;
                        throw std::runtime_error("both white/probe channel are failed");
                    } else {
                        probe_ch_log = true;
                    }
                } else {
                    wh_ch_log = true;
                }
            } else {
                wh_ch_log = false;
                auto& in_grid_log = task.model().in_grid_log();
                task.set_rot_degree(in_grid_log.at("rotate_degree"));
                task.set_um2px_r(in_grid_log.at("um_to_pixel_rate"));
            }
            task.grid_log()["rotate_degree"] = task.rot_degree().value();
            task.grid_log()["um_to_pixel_rate"] = task.um2px_r();
            task.probe_channels()
            | nucleona::range::indexed()
            | ranges::view::transform([&task](auto&& i_jch){
                auto& i = std::get<0>(i_jch);
                auto& jch = std::get<1>(i_jch);
                model::Channel ch_mod;
                ch_mod.init(task, jch);
                channel(ch_mod);
                task.channel_log().at(i) = ch_mod.grid_log();
                return 0;
            })
            | nucleona::range::p_endp(executor)
            ;
            task.grid_log()["input"] = task.model().input().string();
            task.grid_log()["chip_dir"] = task.chip_dir().string();
            task.grid_log()["output_formats"] = task.model().FormatDecoder::to_string();
            task.grid_log()["output"] = task.model().output().string();
            task.grid_log()["no_bgp"] = task.model().no_bgp();
            task.grid_log()["method"] = task.model().method();
            task.grid_log()["shared_dir"] = task.model().shared_dir_path().string();
            task.grid_log()["secure_dir"] = task.model().secure_dir_path().string();
            task.grid_log()["marker_append"] = task.model().marker_append();
            task.grid_log()["auto_gridding"] = task.model().auto_gridding();
            task.grid_log()["version"] = summit::grid::version().to_string();
            task.summary_channel_log();
            task.model().heatmap_writer().flush();
            if(task.model().debug() >= 5) {
                gridline_debug_image_proc(task);
            }

        } catch( const std::exception& e ) {
            summit::grid::log.error("grid failed with reason: {}", e.what());
            task.set_grid_done(false);
            task.grid_log()["grid_fail_reason"] = e.what();
        }
        task.write_log();
        task.copy_chip_log();
        task.create_complete_file();
        return 0;
    }
private:

    __alias::crot::MarkerVec<float>             rotate_detector         ;
    __alias::crot::Calibrate                    rotate_calibrator       ;
    __alias::cmk::RegMatUm2PxRDet               reg_mat_um2px_r_det     ;
    
};

}
