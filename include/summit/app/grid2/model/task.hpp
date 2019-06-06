#pragma once
#include "macro.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#include <summit/app/grid2/task_id.hpp>
#include <summit/config/cell_fov.hpp>
#include <summit/config/chip.hpp>
#include <summit/utils.h>
#include "marker_base.hpp"
#include <Nucleona/util/remove_const.hpp>
namespace summit::app::grid2 {
struct Model;
}
namespace summit::app::grid2::model {

struct Task {
    void set_chip_dir(const boost::filesystem::path& path) {
        chip_dir_ = path;
        std::ifstream fin((path / "chip_log.json").string());
        fin >> chip_log_;
        for(auto&& cl : chip_log_["channels"]) {
            channels_.push_back(cl);
        }
        chipinfo_         = &chip_log_["chip"];
        is_img_enc_       = chip_log_.at("img_encrypted");
        um2px_r_          = chip_log_.value<double>("um_to_px_coef", -1);
        chip_spec_name_   = chipinfo_->at("name");
        origin_infer_     = &chipinfo_->at("origin_infer");
        db_key_           = origin_infer_->at("db_key");
        cell_size_px_     = origin_infer_->at("cell_size_px");

        // chipinfo_->erase("spec");
        fov_              = &summit::config::cell_fov().get_fov_type(chip_spec_name_);
        fov_rows_         = fov_->at("rows");
        fov_cols_         = fov_->at("cols");

        chipspec_         = &summit::config::chip().get_spec(chip_spec_name_);
        cell_h_um_        = chipspec_->at("cell_w_um");
        cell_w_um_        = chipspec_->at("cell_h_um");
        space_um_         = chipspec_->at("space_um");
        aruco_marker_     = &chipspec_->at("aruco_marker");
        shooting_marker_  = &chipspec_->at("shooting_marker");
        id_map_           = &aruco_marker_->at("id_map");
        frame_template_   = summit::install_path() / aruco_marker_->at("frame_template").get<std::string>();
        frame_mask_       = summit::install_path() / aruco_marker_->at("frame_mask").get<std::string>();

        sh_mk_pos_cl_     = &shooting_marker_->at("position_cl");
        mk_wd_cl_         = sh_mk_pos_cl_->at("w_d");
        mk_hd_cl_         = sh_mk_pos_cl_->at("h_d");

        // TODO: all use cache
        fov_marker_num_   = Utils::generate_fov_marker_num(
            chipspec(), fov()
        );

        marker_patterns_  = &MarkerBase::get().reg_mat_chip_mks(chip_spec_name_);
    }
    void set_model(const Model& _model) {
        model_ = &_model;
    }
    void set_task_id(const TaskID& task_id) {
        id_ = task_id;
    }
    void set_white_channel_imgs(Utils::FOVImages<std::uint8_t>&& wci) {
        white_channel_imgs_ = std::move(wci);
    }
    void set_rot_degree(const std::optional<float>& _rot_degree) {
        rot_degree_ = _rot_degree;
    }
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
    }
    const MarkersPair& get_marker_patterns(const std::string& marker_type) {
        return marker_patterns_->at(marker_type);
    }
    const cv::Point& get_fov_marker_num(int r, int c) {
        return fov_marker_num_.at(cv::Point(c, r));
    }
    float mk_wd_um() const {
        return mk_wd_cl_ * cell_w_um_;
    }
    float mk_hd_um() const {
        return mk_hd_cl_ * cell_h_um_;
    }
    Model& get_model() { return const_cast<Model&>(*model_); }

    VAR_GET(nlohmann::json,                 chip_log            )
    VAR_GET(std::vector<nlohmann::json>,    channels            )
    VAR_GET(boost::filesystem::path,        chip_dir            )
    VAR_GET(bool,                           is_img_enc          )
    VAR_GET(double,                         um2px_r             )
    VAR_GET(std::string,                    chip_spec_name      )
    VAR_GET(float,                          cell_h_um           )
    VAR_GET(float,                          cell_w_um           )
    VAR_GET(float,                          space_um            )
    VAR_GET(int,                            fov_rows            )
    VAR_GET(int,                            fov_cols            )
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

    VAR_PTR_GET(Model,                      model               )
    VAR_PTR_GET(nlohmann::json,             chipinfo            )
    VAR_PTR_GET(nlohmann::json,             chipspec            )
    VAR_PTR_GET(nlohmann::json,             fov                 )
    VAR_PTR_GET(nlohmann::json,             aruco_marker        )
    VAR_PTR_GET(nlohmann::json,             id_map              )
    VAR_PTR_GET(nlohmann::json,             origin_infer        )
    VAR_PTR_GET(nlohmann::json,             shooting_marker     )
    VAR_PTR_GET(nlohmann::json,             sh_mk_pos_cl        )
    VAR_PTR_GET(MarkerPatterns,             marker_patterns     )
private:
    VAR_GET(summit::app::grid2::TaskID, id)

};
using TaskMap = std::map<
    std::string,    // task id
    Task            // task
>;

}