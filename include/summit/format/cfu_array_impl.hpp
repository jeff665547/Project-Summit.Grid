#pragma once
#include <summit/format/cfu_array.hpp>
namespace summit::format {

cfu::format::chip_sample::Array init_array(
    const nlohmann::json& chip_spec
) {
    cfu::format::chip_sample::Array array;
    array.date() = summit::utils::datetime("%Y%m%d%H%M%S", std::chrono::system_clock::now()); 
    array.type() = chip_spec["name"];
    array.feature_columns() = chip_spec["w_cl"];
    array.feature_rows()    = chip_spec["h_cl"];
    return array;
}

}