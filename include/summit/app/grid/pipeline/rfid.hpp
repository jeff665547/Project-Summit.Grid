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
        auto tmp_timer(std::chrono::steady_clock::now());
        std::chrono::duration<double, std::milli> d;
        int exit_code = 0;

        /*
         * declare chip process pipeline
         */
        Chip chip;

        task_group.task_ids()
        /*
         * pipe tasks to chip process pipeline
         */
        | ranges::view::transform([&](auto&& task_id){
            model::Task task;
            try {
                /*
                 * create task data model, prepare data
                 */
                task.set_model(task_group.model());
                task.set_task_id(task_id);
                /*
                 * run chip process pipeline
                 */
                exit_code |= chip(task);
            } catch(const std::exception& e) {
                summit::grid::log.error("BUG: {}", e.what());
                exit_code = 1;
            }
            if(exit_code) {
                task.create_warning_file();
            }
            task.create_complete_file();
            return 0;
        })
        /*
         * run each chip sequencially
         */
        | nucleona::range::endp
        ;
        d = std::chrono::steady_clock::now() - tmp_timer;
        std::cout << "rfid: " << d.count() << " ms\n";
        return exit_code;
    }
} rfid;

}