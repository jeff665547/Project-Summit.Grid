#pragma once
#include "macro.hpp"
namespace summit::app::grid2 {
struct Model;
}
namespace summit::app::grid2::model {

struct Task {
    void set_chip_log(const nlohmann::json& _chip_log) {
        chip_log_ = &_chip_log;
    }
    void set_model(const Model& _model) {
        model_ = &_model;
    }

    VAR_GET(std::add_const_t<nlohmann::json>*, chip_log)
    VAR_GET(std::add_const_t<Model>*, model)
};

}