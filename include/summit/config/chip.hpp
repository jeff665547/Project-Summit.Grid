#pragma once
#include <nlohmann/json.hpp>
#include <summit/exception/debug_throw.hpp>
#include <summit/exception/chip_spec_not_found.hpp>

namespace summit{ namespace config{
struct Chip : public nlohmann::json {
    using Base = nlohmann::json;
    using Base::Base;
    auto& get_spec(const std::string& name) {
        auto& chip_specs = static_cast<nlohmann::json&>(*this);
        for( auto& cs : chip_specs ) {
            if( cs["name"].template get<std::string>() == name ) {
                return cs;
            }
        }
        std::stringstream ss;
        ss << "chip spec: " << name << "not found";
        debug_throw( std::runtime_error(ss.str()));
    }
private:
};
Chip& chip();
void reload_chip();
}}