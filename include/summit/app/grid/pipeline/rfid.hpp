#pragma once
#include <summit/app/grid/pipeline/chip.hpp>
#include <summit/app/grid/model/task_group.hpp>
#include <summit/app/grid/model/task.hpp>
#include <summit/format/rfid.hpp>
namespace summit::app::grid::pipeline {

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
        Chip chip;
        task_group.task_ids()
        | ranges::view::transform([&](auto&& task_id){
            model::Task task;
            task.set_model(task_group.model());
            task.set_task_id(task_id);
            return chip(task);
        })
        | nucleona::range::p_endp(
            task_group.model().executor()
        )
        ;
        return 0;
    }
} rfid;

}