#pragma once
#include "macro.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#include <summit/app/grid2/task_id.hpp>
#include <summit/config/cell_fov.hpp>
#include <summit/config/chip.hpp>
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
        chipinfo_       = &chip_log_["chip"];
        is_img_enc_     = chip_log_.at("img_encrypted");
        um2px_r_        = chip_log_.at("um_to_px_coef");
        chip_spec_name_ = chipinfo_->at("name");

        // chipinfo_->erase("spec");
        fov_            = &summit::config::cell_fov().get_fov_type(chip_spec_name_);
        fov_rows_       = fov_->at("rows");
        fov_cols_       = fov_->at("cols");

        chipspec_       = &summit::config::chip().get_spec(chip_spec_name_);
        cell_h_um_      = chipspec_->at("cell_w_um");
        cell_w_um_      = chipspec_->at("cell_h_um");

    }
    void set_model(const Model& _model) {
        model_ = &_model;
    }
    void set_task_id(const TaskID& task_id) {
        id_ = task_id;
    }

    VAR_GET(nlohmann::json,                 chip_log        )
    VAR_GET(std::vector<nlohmann::json>,    channels        )
    VAR_GET(boost::filesystem::path,        chip_dir        )
    VAR_GET(bool,                           is_img_enc      )
    VAR_GET(double,                         um2px_r         )
    VAR_GET(std::string,                    chip_spec_name  )
    VAR_GET(float,                          cell_h_um       )
    VAR_GET(float,                          cell_w_um       )
    VAR_GET(int,                            fov_rows        )
    VAR_GET(int,                            fov_cols        )

    VAR_PTR_GET(Model, model)
    VAR_PTR_GET(nlohmann::json, chipinfo)
    VAR_PTR_GET(nlohmann::json, chipspec)
    VAR_PTR_GET(nlohmann::json, fov)
private:
    VAR_GET(summit::app::grid2::TaskID, id)

};
using TaskMap = std::map<
    std::string,    // task id
    Task            // task
>;

}