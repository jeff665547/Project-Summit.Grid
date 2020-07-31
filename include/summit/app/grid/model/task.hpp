/**
 * @file task.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::model::Task
 */
#pragma once
#include "macro.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#include <summit/app/grid/task_id.hpp>
#include <summit/config/cell_fov.hpp>
#include <summit/config/chip.hpp>
#include <summit/utils.h>
#include "marker_base.hpp"
#include <Nucleona/util/remove_const.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include "type.hpp"
#include <summit/app/grid/utils.hpp>
#include "model.hpp"

namespace summit::app::grid::model {
/**
 * @brief Chip level parameter model,
 *    provide and integrate the parameters used 
 *    in the chip level and the level lower than the chip.
 * 
 */
struct Task {
    template<class T>
    using ChnMap = std::map<std::string, T>;
    void set_model(const Model& _model) {
        model_ = &_model;
    }
    void set_task_id(const TaskID& task_id) {
        id_ = task_id;
        set_chip_dir(id_.path());
    }
    void set_white_channel_imgs(Utils::FOVImages<std::uint8_t>&& wci) {
        white_channel_imgs_ = std::move(wci);
    }
    void set_rot_degree(const std::optional<float>& _rot_degree) {
        rot_degree_ = _rot_degree;
    }
    nlohmann::json& grid_log() { return grid_log_; }
    bool rot_degree_done() const {
        return rot_degree_.has_value();
    }
    void set_fov_mk_regs(Utils::FOVMarkerRegionMap&& _fov_mk_regs) {
        fov_mk_regs_ = std::move(_fov_mk_regs);
    }
    void set_um2px_r(double _um2px_r) {
        um2px_r_ = _um2px_r;
    }
    bool um_to_px_r_done() const {
        return !(um2px_r_ < 0);
    }
    void set_white_ch_proc_failed(bool flag) {
        white_ch_proc_failed_ = flag;
    }
    auto first_probe_channel() {
        for(int i = 0; i < channels_.size(); i ++){
            auto& ch = channels_.at(i);
            if(ch["filter"].get<int>() != 0) {
                return nucleona::make_tuple(std::move(i), ch);
            }
        }
        throw std::runtime_error("the chip log doesn't include any probe channel");
    }
    auto white_channel() {
        for(int i = 0; i < channels_.size(); i ++){
            auto& ch = channels_.at(i);
            if(ch["filter"].get<int>() == 0) {
                return nucleona::make_tuple(std::move(i), ch);
            }
        }
        throw std::runtime_error("the chip log doesn't include any white channel");
    }
    auto get_marker_patterns_by_marker_type(const std::string& marker_type) {
        return marker_patterns_->get_by_marker_type(marker_type);
    }
    template<class T>
    auto get_marker_patterns(const std::string& query_key, T value) {
        return marker_patterns_->get(query_key, value);
    }
    const cv::Point& get_fov_marker_num(int r, int c) {
        return fov_marker_num_.at(cv::Point(c, r));
    }
    double mk_wd_um() const {
        return mk_wd_cl_ * (cell_w_um_ + space_um_);
    }
    double mk_hd_um() const {
        return mk_hd_cl_ * (cell_h_um_ + space_um_);
    }
    double mk_w_um() const {
        return mk_w_cl_ * (cell_w_um_ + space_um_);
    }
    double mk_h_um() const {
        return mk_h_cl_ * (cell_h_um_ + space_um_);
    }
    double mk_wd_px() const {
        return mk_wd_um() * um2px_r_;
    }
    double mk_hd_px() const {
        return mk_hd_um() * um2px_r_;
    }
    double mk_w_px() const {
        return mk_w_um() * um2px_r_;
    }
    double mk_h_px() const {
        return mk_h_um() * um2px_r_;
    }
    double mk_wd_px(double um2px_r) const {
        return mk_wd_um() * um2px_r;
    }
    double mk_hd_px(double um2px_r) const {
        return mk_hd_um() * um2px_r;
    }
    double mk_w_px(double um2px_r) const {
        return mk_w_um() * um2px_r;
    }
    double mk_h_px(double um2px_r) const {
        return mk_h_um() * um2px_r;
    }
    auto grid_log_path() const {
        return model_->task_grid_log(id_).string();
    }
    auto channel_grid_log_path() const {
        return [this](const auto& chname) {
            return model_->channel_grid_log(id_, chname).string();
        };
    }
    void set_grid_done(bool flag) {
        grid_done_ = flag;
    }
    void summary_channel_log() {
        grid_done_ = true;
        for(auto&& ch : *channel_log_) {
            grid_done_ = grid_done_ && ch.at("grid_done").get<bool>();
        }

        grid_bad_ = false;
        for(auto&& ch : *channel_log_) {
            grid_bad_ = grid_bad_ || ch.at("grid_bad").get<bool>();
        }
    }
    void write_log() {
        grid_log_["proc_time"] = proc_time_;
        grid_log_["grid_done"] = grid_done_;
        std::ofstream fout(grid_log_path());
        fout << grid_log_.dump(2);
        fout << std::flush;
    }
    auto fov_image(
        const std::string& tag,
        int r, int c, const std::string& ch
    ) const {
        return model_->fov_image(id_,tag, r, c, ch);
    }
    auto stitch_image(
        const std::string& tag,
        const std::string& ch 
    ) const {
        return model_->stitch_image(id_, tag, ch);
    }
    auto gridline(const std::string& ch_name) const {
        return model_->gridline(id_, ch_name);
    }
    void set_proc_time(float time) {
        proc_time_ = time;
    }
    auto& channel_in_grid_log(int ch_i) const {
        return model_->in_grid_log().at("channels").at(ch_i);
    }
    void create_complete_file() const {
        model_->create_complete_file(id_);
    }
    void copy_chip_log() const {
        std::ofstream grid_cl(model_->grid_chip_log(id_).string());
        grid_cl << chip_log_;
        grid_cl.close();
        if(!model_->secure_output_enabled()) return ;
        std::ofstream fout(model_->sc_chip_log().string());
        fout << chip_log_;
        fout.close();
    }
    bool support_aruco() const {
        return origin_infer_algo_ == "aruco_detection";
    }
    void collect_fovs_mk_append(Utils::FOVMap<cv::Mat>& fov_mk_append) {
        auto wh_mk_append_mat = Utils::make_fovs_mk_append(
            fov_mk_append, 
            fov_rows(), fov_cols(),
            [](auto&& mat){ return mat; }
        );
        auto path = model().marker_append_path(
            id_, ""
        );
        cv::imwrite(path.string(), wh_mk_append_mat);
    }
    boost::filesystem::path debug_img(
        const std::string& ch_name, 
        int r, int c, 
        const std::string& tag
    ) const {
        return model().debug_img(id(), ch_name, r, c, tag);
    }
    template<class Str>
    std::function<void(const cv::Mat&)> debug_img_view(
        Str&& ch_name, 
        int r, int c, 
        const std::string& tag, 
        bool viewable = false
    ) const {
        if(model().debug() >= 5 ) {
            if(viewable) {
                return [
                    chn = FWD(ch_name), 
                    r, c, tag, 
                    this
                ](const cv::Mat& view) {
                    auto path = debug_img(chn, r, c, tag);
                    cv::imwrite(path.string(), chipimgproc::viewable(view));
                };
            } else {
                return [
                    chn = FWD(ch_name), 
                    r, c, tag, 
                    this
                ](const cv::Mat& view) {
                    auto path = debug_img(chn, r, c, tag);
                    cv::imwrite(path.string(), view);
                };
            }
        } else {
            return nullptr;
        }
    }
    auto aruco_ch_mk_seg_view(int r, int c) const {
        return debug_img_view("aruco", r, c, "aruco_mkseg", false);
    }
    void set_multi_tiled_mat(const std::string& chname, MTMat mat) {
        multi_tiled_mat_[chname] = std::move(mat);
    }
    void set_stitched_img(const std::string& chname, GLRawImg glraw_img) {
        stitched_img_[chname] = std::move(glraw_img);
    }
    auto debug_stitch(const std::string& tag = "") {
        return model().debug_stitch(id(), tag);
    }
    VAR_GET(nlohmann::json,                 chip_log            )
    VAR_GET(nlohmann::json,                 grid_log            )
    VAR_GET(std::vector<nlohmann::json>,    channels            )
    VAR_GET(std::vector<nlohmann::json>,    probe_channels      )
    VAR_GET(boost::filesystem::path,        chip_dir            )
    VAR_GET(bool,                           is_img_enc          )
    VAR_GET(double,                         um2px_r             )
    VAR_GET(std::string,                    chip_info_name      )
    VAR_GET(std::string,                    chip_spec_name      )
    VAR_GET(float,                          cell_h_um           )
    VAR_GET(float,                          cell_w_um           )
    VAR_GET(float,                          space_um            )
    VAR_GET(int,                            fov_rows            )
    VAR_GET(int,                            fov_cols            )
    VAR_GET(int,                            fov_w               )
    VAR_GET(int,                            fov_h               )
    VAR_GET(int,                            fov_wd              )
    VAR_GET(int,                            fov_hd              )
    VAR_GET(Utils::FOVImages<std::uint8_t>, white_channel_imgs  )
    VAR_GET(std::int32_t,                   pyramid_level       )
    VAR_GET(std::int32_t,                   border_bits         )
    VAR_GET(std::int32_t,                   fringe_bits         )
    VAR_GET(double,                         bit_w               )
    VAR_GET(double,                         margin_size         )
    VAR_GET(boost::filesystem::path,        frame_template      )
    VAR_GET(boost::filesystem::path,        frame_mask          )
    VAR_GET(std::int32_t,                   nms_count           )
    VAR_GET(std::int32_t,                   nms_radius          )
    VAR_GET(std::int32_t,                   cell_size_px        )
    VAR_GET(std::string,                    db_key              )
    VAR_GET(std::optional<float>,           rot_degree          )
    VAR_GET(Utils::FOVMarkerRegionMap,      fov_mk_regs         )
    VAR_GET(bool,                           white_ch_proc_failed)
    VAR_GET(Utils::FOVMarkerNum,            fov_marker_num      )
    VAR_GET(std::uint32_t,                  mk_wd_cl            )
    VAR_GET(std::uint32_t,                  mk_hd_cl            )
    VAR_GET(std::uint32_t,                  mk_w_cl             )
    VAR_GET(std::uint32_t,                  mk_h_cl             )
    VAR_GET(std::uint32_t,                  mk_col_cl           )
    VAR_GET(std::uint32_t,                  mk_row_cl           )
    VAR_GET(std::uint32_t,                  mk_xi_cl            )
    VAR_GET(std::uint32_t,                  mk_yi_cl            )
    VAR_GET(std::uint32_t,                  spec_h_cl           )
    VAR_GET(std::uint32_t,                  spec_w_cl           )
    VAR_GET(float,                          proc_time           )
    VAR_GET(bool,                           grid_done           )
    VAR_GET(bool,                           grid_bad            )
    VAR_GET(std::string,                    origin_infer_algo   )
    VAR_GET(summit::app::grid::TaskID,      id                  )

    VAR_PTR_GET(Model,                      model               )
    VAR_PTR_GET(nlohmann::json,             chipinfo            )
    VAR_PTR_GET(nlohmann::json,             chipspec            )
    VAR_PTR_GET(nlohmann::json,             fov                 )
    VAR_PTR_GET(nlohmann::json,             aruco_marker        )
    VAR_PTR_GET(nlohmann::json,             id_map              )
    VAR_PTR_GET(nlohmann::json,             origin_infer        )
    VAR_PTR_GET(nlohmann::json,             shooting_marker     )
    VAR_PTR_GET(nlohmann::json,             sh_mk_pats_cl       )
    VAR_PTR_GET(nlohmann::json,             sh_mk_pos_cl        )
    VAR_PTR_GET(nlohmann::json,             sh_mk_pats          )
    VAR_PTR_GET(ChipSpecMarkerBase,         marker_patterns     )

    VAR_LOCAL_PTR_GET(nlohmann::json,       channel_log         )

    VAR_GET(ChnMap<OptMTMat>,               multi_tiled_mat     )
    VAR_GET(ChnMap<GLRawImg>,               stitched_img        )
private:

    
    void set_chip_dir(const boost::filesystem::path& path) {
        grid_log_["channels"] = nlohmann::json::array();
        channel_log_ = &grid_log_["channels"];
        chip_dir_ = path;
        {
            std::size_t i = 0;
            std::ifstream fin((path / "chip_log.json").string());
            fin >> chip_log_;
            for(auto&& cl : chip_log_["channels"]) {
                channels_.push_back(cl);
                auto filter_id = cl.at("filter");
                if(filter_id != 0) {
                    probe_channels_.push_back(cl);
                    probe_channels_.back()["id"] = i;
                    (*channel_log_)[i] = nlohmann::json::object();
                    i ++;
                }
            }
            // if probe channels has any invalid channel id, all channel assign to sequencial id.
            for(auto&& pch : probe_channels_) {
                if(pch.at("id") < 0) {
                    for(std::size_t i = 0; i < probe_channels_.size(); i++) {
                        auto& _pch = probe_channels_[i];
                        _pch.at("id") = i;
                    }
                    break;
                }
            }
        }

        chipinfo_         = &chip_log_["chip"];
        is_img_enc_       = chip_log_.at("img_encrypted");
        um2px_r_          = chip_log_.value<double>("um_to_px_coef", -1);
        chip_info_name_   = chipinfo_->at("name");
        chip_spec_name_   = chipinfo_->at("spec").at("name");
        origin_infer_     = &chipinfo_->at("origin_infer");
        origin_infer_algo_= origin_infer_->at("algo");
        chipspec_         = &summit::config::chip().get_spec(chip_spec_name_);
        if(support_aruco()) {
            // pyramid_level_    = origin_infer_->at("pyramid_level");
            pyramid_level_    = 1;
            nms_count_        = origin_infer_->at("nms_count");
            cell_size_px_     = origin_infer_->at("cell_size_px");
            aruco_marker_     = &chipspec_->at("aruco_marker");
            if(auto iter = origin_infer_->find("db_key"); iter == origin_infer_->end()) {
                db_key_ =  aruco_marker_->value("db_key", "");
            } else {
                db_key_ = iter.value();
            }
            id_map_           = &aruco_marker_->at("id_map");
            border_bits_      = aruco_marker_->at("border_bits");
            fringe_bits_      = aruco_marker_->at("fringe_bits");
            bit_w_            = aruco_marker_->at("bit_w");
            margin_size_      = aruco_marker_->at("margin_size");
            nms_radius_       = aruco_marker_->at("nms_radius");
            frame_template_   = (summit::install_path() / aruco_marker_->at("frame_template").get<std::string>()).make_preferred();
            frame_mask_       = (summit::install_path() / aruco_marker_->at("frame_mask").get<std::string>()).make_preferred();
        }

        // chipinfo_->erase("spec");

        fov_              = &summit::config::cell_fov().get_fov_type(chip_info_name_);
        auto& tmp_fov     = fov_->at("fov");
        fov_rows_         = tmp_fov.at("rows");
        fov_cols_         = tmp_fov.at("cols");
        fov_wd_           = tmp_fov.at("w_d");
        fov_hd_           = tmp_fov.at("h_d");
        fov_w_            = tmp_fov.at("w");
        fov_h_            = tmp_fov.at("h");

        cell_h_um_        = chipspec_->at("cell_w_um");
        cell_w_um_        = chipspec_->at("cell_h_um");
        space_um_         = chipspec_->at("space_um");
        shooting_marker_  = &chipspec_->at("shooting_marker");
        spec_h_cl_        = chipspec_->at("h_cl");
        spec_w_cl_        = chipspec_->at("w_cl");

        sh_mk_pats_cl_    = &shooting_marker_->at("mk_pats_cl");
        sh_mk_pos_cl_     = &shooting_marker_->at("position_cl");

        sh_mk_pats_       = &shooting_marker_->at("mk_pats");

        mk_wd_cl_         = sh_mk_pos_cl_->at("w_d");
        mk_hd_cl_         = sh_mk_pos_cl_->at("h_d");
        mk_w_cl_          = sh_mk_pos_cl_->at("w");
        mk_h_cl_          = sh_mk_pos_cl_->at("h");
        mk_xi_cl_         = sh_mk_pos_cl_->at("x_i");
        mk_yi_cl_         = sh_mk_pos_cl_->at("y_i");
        mk_col_cl_        = sh_mk_pos_cl_->at("col");
        mk_row_cl_        = sh_mk_pos_cl_->at("row");



        // TODO: all use cache
        fov_marker_num_   = Utils::generate_fov_marker_num(
            chipspec(), fov()
        );

        marker_patterns_  = &MarkerBase::get().reg_mat_chip_mks(
            chip_spec_name_, sh_mk_pats_cl(), sh_mk_pats()
        );

        for(auto&& [fov_id, _num] : fov_marker_num_ ) {
            fov_mk_regs_[fov_id] = {};
        }
    }
};
using TaskMap = std::map<
    std::string,    // task id
    Task            // task
>;

}