#pragma once
#include <nlohmann/json.hpp>
#include "paths.hpp"
#include <Nucleona/language.hpp>
#include <type_traits>

#define VAR_GET(T, sym) \
public: std::add_const_t<T>& sym() const { return sym##_; } \
private: T sym##_;

namespace summit::app::grid2 {

struct Model : public Paths{

    template<class... Args>
    decltype(auto) set_paths(Args&&... args) {
        return Paths::set(FWD(args)...);
    }

    void set_chip_log(const nlohmann::json& _chip_log) {
        chip_log_ = &_chip_log;
    }

    VAR_GET(std::add_const_t<nlohmann::json>*, chip_log)
};

#undef VAR_GET
}