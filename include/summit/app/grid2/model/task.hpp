#pragma once

namespace summit::app::grid2::model {

struct Task {
    void set_chip_log(const nlohmann::json& _chip_log) {
        chip_log_ = &_chip_log;
    }

    VAR_GET(std::add_const_t<nlohmann::json>*, chip_log)

};

}