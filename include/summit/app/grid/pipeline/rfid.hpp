/**
 * @file rfid.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::pipeline::RFID
 * 
 */
#pragma once
#include <summit/app/grid/pipeline/chip.hpp>
#include <summit/app/grid/model/task_group.hpp>
#include <summit/app/grid/model/task.hpp>
#include <summit/format/rfid.hpp>
namespace summit::app::grid::pipeline {

/**
 * @brief RFID level process, implementation defined, not specified in specification
 * @details Parallel process each chip in RFID.
 */
constexpr struct RFID {
    /**
     * @brief Run task group process pipeline
     * 
     * @param task_group Tasks group by RFID
     * @return decltype(auto) exit code
     */
    decltype(auto) operator()(model::TaskGroup& task_group) const {

        /*
         * declare chip process pipeline
         */
        Chip chip;

        task_group.task_ids()
        /*
         * pipe tasks to chip process pipeline
         */
        | ranges::view::transform([&](auto&& task_id){
            try {
                /*
                 * create task data model, prepare data
                 */
                model::Task task;
                task.set_model(task_group.model());
                task.set_task_id(task_id);
                /*
                 * run chip process pipeline
                 */
                return chip(task);
            } catch(const std::exception& e) {
                summit::grid::log.error("BUG: {}", e.what());
                return 1;
            }
        })
        /*
         * pipe chip pipeline to parallel engine
         */
        | nucleona::range::p_endp( // TODO: careful the memory limitation
            task_group.model().executor()
        )
        ;
        return 0;
    }
} rfid;

}