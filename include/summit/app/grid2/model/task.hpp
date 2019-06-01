#pragma once
#include "macro.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
namespace summit::app::grid2 {
struct Model;
}
namespace summit::app::grid2::model {

struct Task {
    void set_chip_log(const boost::filesystem::file& path) {
        std::ifstream fin(path.string());
        fin >> chip_log_;
        channels_ = chip_log_["channels"];
    }
    void set_model(const Model& _model) {
        model_ = &_model;
    }

    VAR_GET(nlohmann::json, chip_log)
    VAR_GET(std::vector<nlohmann::json>, channels)

    VAR_PTR_GET(Model, model)
    VAR_PTR_GET(nlohmann::json, chipinfo)
    VAR_PTR_GET(nlohmann::json, chipspec)

private:
    

};

}