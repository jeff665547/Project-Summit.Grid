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

namespace summit::app::grid::model {

struct Channel {

    void init(const Task& _task, const nlohmann::json& jch, int ch_id) {
        task_ = &_task;
        json_ = &jch;
        ch_name_ = json_->at("name");
        grid_log_["name"] = ch_name_;
        id_ = ch_id;
    }
    auto heatmap_writer() {
        return [this](MTMat& mt_mat){
            task_->model().heatmap_writer().write(
                *task_,
                ch_name_,
                task_->model().filter(),
                mt_mat
            );
        };
    }
    auto heatmap_writer() const {
        return nucleona::remove_const(*this).heatmap_writer();
    }
    auto background_writer() {
        return [this](const Utils::FOVMap<float>& bg_vs){
            task_->model().background_writer().write(
                task_->id().string(),
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
    auto mk_append_view() const {
        return [this](const cv::Mat& view) {
            cv::Mat tmp = (view * 8.192) + 8192;
            auto path = task().model().marker_append_path(
                task_id(), ch_name_
            );
            cv::imwrite(path.string(), tmp);
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
    template<class FOVMod>
    void collect_fovs_mk_append(Utils::FOVMap<FOVMod>& fov_mods) {
        Utils::FOVMap<cv::Rect> roi;
        int y_start = 0;
        for(auto y : nucleona::range::irange_0(task_->fov_rows())) {
            int row_max = 0;
            int x_start = 0;
            for(auto x : nucleona::range::irange_0(task_->fov_cols())) {
                cv::Point fov_id(x, y);
                auto&& mk_a = fov_mods.at(fov_id).mk_append();
                auto& fov_roi = roi[fov_id];
                fov_roi.x = x_start;
                fov_roi.y = y_start;
                fov_roi.width  = mk_a.cols;
                fov_roi.height = mk_a.rows;
                summit::grid::log.trace(
                    "fov_roi({},{},{},{})", 
                    fov_roi.x, fov_roi.y, 
                    fov_roi.width, fov_roi.height
                );
                if(row_max < mk_a.rows) {
                    row_max = mk_a.rows;
                }
                x_start += mk_a.cols;
            }
            y_start += row_max;
        }
        cv::Point first(0,0);
        cv::Point last(task_->fov_cols() - 1, task_->fov_rows() - 1);
        auto& last_roi = roi.at(last);
        auto& first_ma = fov_mods.at(first).mk_append();
        cv::Mat data(
            last_roi.y + last_roi.height, 
            last_roi.x + last_roi.width,
            first_ma.type()
        );
        for(auto y : nucleona::range::irange_0(task_->fov_rows())) {
            for(auto x : nucleona::range::irange_0(task_->fov_cols())) {
                cv::Point fov_id(x, y);
                auto& fov_roi = roi.at(fov_id);
                auto&& mk_a = fov_mods.at(fov_id).mk_append();
                mk_a.copyTo(data(fov_roi));
            }
        }
        mk_append_mat_ = data;
    }
    template<class FOVMod>
    void collect_fovs(Utils::FOVMap<FOVMod>& fov_mods) {
        auto& fovs = grid_log_["fovs"];
        fovs = nlohmann::json::array();
        for(auto&& [fov_id, fov] : fov_mods) {
            fovs.push_back(fov.grid_log());
        }
        multi_tiled_mat_ = make_multi_tiled_mat(fov_mods, *task_);
        collect_fovs_mk_append(fov_mods);
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
    auto& in_grid_log() const {
        return task_->channel_in_grid_log(id_);
    }
    
    VAR_GET(std::string,                            ch_name             )
    VAR_GET(int,                                    id                  )
    VAR_IO(OptMTMat,                                multi_tiled_mat     )
    VAR_IO(cv::Mat,                                 mk_append_mat       )
    VAR_IO(nlohmann::json,                          grid_log            )

    VAR_PTR_GET(Task,                               task)
    VAR_PTR_GET(nlohmann::json,                     json)
private:
    std::string task_id() const {
        return task().id().string();
    }
    boost::filesystem::path debug_img(int r, int c, const std::string& tag) const {
        return task().model().debug_img(task_id(), ch_name_, r, c, tag);
    }
    std::function<void(const cv::Mat&)> debug_img_view(
        int r, int c, const std::string& tag, bool viewable = false
    ) const {
        if(task().model().debug() > 1) {
            if(viewable) {
                return [r, c, tag, this](const cv::Mat& view) {
                    auto path = debug_img(r, c, tag);
                    cv::imwrite(path.string(), chipimgproc::viewable(view));
                };
            } else {
                return [r, c, tag, this](const cv::Mat& view) {
                    auto path = debug_img(r, c, tag);
                    cv::imwrite(path.string(), view);
                };
            }
        } else {
            return nullptr;
        }
    }
};

}