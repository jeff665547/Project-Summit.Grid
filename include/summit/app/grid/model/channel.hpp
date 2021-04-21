/**
 * @file summit/app/grid/model/channel.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::model::Channel
 * 
 */
#pragma once
#include "macro.hpp"
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <summit/app/grid/utils.hpp>
#include "single_img_proc_res.hpp"
#include "task.hpp"
#include "make_multi_tiled_mat.hpp"
#include "type.hpp"
#include "model.hpp"
#include <ChipImgProc/utils.h>
#include "make_multi_warped_mat.hpp"

namespace summit::app::grid::model {
/**
 * @brief The channel level parameter model, 
 *    provide and integrate the parameters used 
 *    in the channel level and the level lower than the channel.
 * 
 */
struct Channel {

    void init(const Task& _task, const nlohmann::json& jch) {
        warn_ = false;
        task_ = &_task;
        json_ = &jch;
        ch_name_ = json_->at("name");
        grid_log_["name"] = ch_name_;
        id_ = json_->at("id");
        sh_mk_pats_ = task_->get_marker_patterns_by_marker_type(
            jch.at("marker_type")
        );
    }
    auto heatmap_writer() {
        return [this](const MWMat& mw_mat){
            task_->model().heatmap_writer().write(
                *task_,
                ch_name_,
                id_,
                task_->model().filter(),
                mw_mat
            );
        };
    }
    auto heatmap_writer() const {
        return nucleona::remove_const(*this).heatmap_writer();
    }
    auto background_writer() {
        return [this](const Utils::FOVMap<float>& bg_vs){
            task_->model().background_writer().write(
                task_->id(),
                ch_name_,
                bg_vs
            );
        };
    }
    auto background_writer() const {
        return nucleona::remove_const(*this).background_writer();
    }
    void write_log() const {
        std::ofstream fout(task_->channel_grid_log_path()(ch_name_));
        fout << grid_log_.dump(2);
        fout << std::flush;
    }
    auto pch_rot_view(int r, int c) const {
        return debug_img_view(r, c, "rot", true);
    }
    auto pch_mk_seg_view(int r, int c) const {
        return debug_img_view(r, c, "dc_mkseg", false);
    }
    auto final_mk_seg_view(int r, int c) const {
        return debug_img_view(r, c, "mkseg", false);
    }
    auto pch_grid_view(int r, int c) const {
        return debug_img_view(r, c, "grid", false);
    }
    auto pch_margin_view_0(int r, int c, bool no_bgp) const {
        if(no_bgp)
            return debug_img_view(r, c, "margin", false);
        else 
            return std::function<void(const cv::Mat&)>(nullptr);
    }
    auto pch_margin_view_1(int r, int c, bool no_bgp) const {
        if(no_bgp)
            return std::function<void(const cv::Mat&)>(nullptr);
        else 
            return debug_img_view(r, c, "margin", false);
    }
    auto pch_margin_view(int r, int c) const {
        return debug_img_view(r, c, "margin", false);
    }
    auto mk_append_view() const {
        return [this](const cv::Mat& view) {
            auto path = task().model().marker_append_path(
                task().id(), ch_name_
            );
            cv::imwrite(path.string(), view);
        };
    }
    auto fov_image(
        const std::string& tag,
        int r, int c
    ) const {
        return task_->fov_image(tag, r, c, ch_name_);
    }
    auto stitch_image(
        const std::string& tag
    ) const {
        return task_->stitch_image(tag, ch_name_);
    }
    auto gridline() const {
        return task_->gridline(ch_name_);
    }
    auto stitch_gridline(
        const std::string& tag
    ) const {
        return task_->stitch_gridline(tag, ch_name_);
    }
    template<class FOVMod>
    void collect_fovs_mk_append(Utils::FOVMap<FOVMod>& fov_mods) {
        Utils::FOVMap<cv::Mat> fov_mats;
        for(auto&& [fov_id, mod] : fov_mods) {
            fov_mats[fov_id] = mod.mk_append();
        }
        mk_append_mat_ = Utils::make_fovs_mk_append(fov_mats, 
            task_->fov_rows(), task_->fov_cols(),
            [](auto&& mat) {
                return (mat * 8.192) + 8.192;
            }
        );
        if(task_->ref_from_probe_ch()) {
            for(auto&& [fov_id, mod] : fov_mods) {
                fov_mk_append_dn_[fov_id] = mod.mk_append_denoised();
            }
        }
    }
    // template<class FOVMod>
    // void collect_fovs(Utils::FOVMap<FOVMod>& fov_mods) {
    //     nucleona::remove_const(*task_).set_multi_tiled_mat(ch_name_, make_multi_tiled_mat(fov_mods, *task_));
    //     collect_fovs_mk_append(fov_mods);
    // }
    template<class FOVMod>
    void collect_fovs(Utils::FOVMap<FOVMod>& fov_mods) {
        nucleona::remove_const(*task_).set_multi_warped_mat(
            ch_name_, summit::app::grid::make_multi_warped_mat(fov_mods, *task_)
        );
        collect_fovs_mk_append(fov_mods);
    }
    template<class FOVMod>
    void summary_fov_log(Utils::FOVMap<FOVMod>& fov_mods) {
        auto& fovs = grid_log_["fovs"];
        fovs = nlohmann::json::array();
        for(auto&& [fov_id, fov] : fov_mods) {
            fovs.push_back(fov.grid_log());
        }
        update_grid_done();
        update_grid_bad();
    }
    decltype(auto) multi_tiled_mat() { 
        return nucleona::remove_const(*task_).multi_tiled_mat().at(ch_name_);
    }
    decltype(auto) multi_warped_mat() { 
        return nucleona::remove_const(*task_).multi_warped_mat().at(ch_name_);
    }
    void set_stitched_img(GLRawImg&& grid_raw_img) {
        nucleona::remove_const(*task_).set_stitched_img(ch_name_, std::move(grid_raw_img));
    }
    template<class GLID>
    void set_gridline(
        const std::vector<GLID>& glx, 
        const std::vector<GLID>& gly
    ) {
        grid_log_["gl_x"] = glx;
        grid_log_["gl_y"] = gly;
    }
    void update_grid_done() {
        bool flag = true;
        for(auto&& fov_log : grid_log_.at("fovs")) {
            flag = flag && fov_log.at("grid_done").get<bool>();
        }
        grid_log_["grid_done"] = flag;
    }
    void update_grid_bad() {
        bool flag = false;
        for(auto&& fov_log : grid_log_.at("fovs")) {
            flag = flag || fov_log.at("grid_bad").get<bool>();
        }
        grid_log_["grid_bad"] = flag;
    }
    void set_grid_failed(const std::string& reason) {
        grid_log_["grid_done"] = false;
        grid_log_["grid_bad"] = true;
        grid_log_["grid_fail_reason"] = reason;
    }
    void set_warning() {
        grid_log_["warning"] = warn_;
    }
    auto& in_grid_log() const {
        return task_->channel_in_grid_log(id_);
    }
    
    VAR_GET(std::string,                            ch_name             )
    VAR_GET(std::vector<const MKPat*>,              sh_mk_pats          )
    VAR_IO(cv::Mat,                                 mk_append_mat       )
    VAR_IO(Utils::FOVMap<cv::Mat>,                  fov_mk_append_dn    )
    VAR_IO(nlohmann::json,                          grid_log            )
    VAR_IO(bool,                                    warn                )

    VAR_PTR_GET(Task,                               task)
    VAR_PTR_GET(nlohmann::json,                     json)
private:
    int id_;
    std::string task_id() const {
        return task().id().string();
    }
    std::function<void(const cv::Mat&)> debug_img_view(
        int r, int c, 
        const std::string& tag, 
        bool viewable = false
    ) const {
        return task().debug_img_view(ch_name_, r, c, tag, viewable);
    }
};

}