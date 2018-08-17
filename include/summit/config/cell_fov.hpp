#pragma once
#include <nlohmann/json.hpp>
#include <summit/exception/debug_throw.hpp>

namespace summit{ namespace config{
struct CellFOV : public nlohmann::json {
    using Base = nlohmann::json;
    using Base::Base;
    const nlohmann::json& get_fov_type( const std::string& name ) {
        auto& chip_fov = static_cast<nlohmann::json&>(*this);
        for( auto& cf : chip_fov ) {
            if( cf["name"].template get<std::string>() == name ) {
                return cf;
            }
        }
        std::stringstream ss;
        ss << "chip fov type: " << name << " not found";
        debug_throw( std::runtime_error(ss.str()) );

    }
private:
};
CellFOV& cell_fov();
void reload_cell_fov();
}}