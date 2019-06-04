#pragma once
#include <summit/app/grid2/pipeline/chip.hpp>
#include <summit/app/grid2/model/task_group.hpp>
#include <summit/format/rfid.hpp>
namespace summit::app::grid2::pipeline {

constexpr struct RFID {
    bool is_rfid_format(const std::string& rfid) const {
        try{
            auto v = format::RFID::parse(rfid);
            return true;
        } catch(...) {
            return false;
        }
    }
    decltype(auto) operator()(model::TaskGroup& task_group) const {
        // if(is_rfid_format(task_group.rfid()) && task_group.size() > 3) {
        // }
        task_group
        | ranges::view::values
        | ranges::view::transform(chip)
        | nucleona::range::p_endp(
            task_group.get_model().executor()
        )
        ;
        return 0;
    }
} rfid;

}