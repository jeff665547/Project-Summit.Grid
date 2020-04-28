/**
 * @file task_group.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::model::Model
 * 
 */
#pragma once
#include "macro.hpp"
#include <summit/app/grid/task_id.hpp>
namespace summit::app::grid::model {
struct Model;
}
namespace summit::app::grid::model {

/**
 * @brief The task group level parameter model,
 *      provide and integrate the parameters used
 *      in the task group level and the level lower than the task group.
 * @details Note that, task group is implementation defined level, not in the specification.
 * 
 */
struct TaskGroup
{
    void set_model(const Model& _model) {
        model_ = &_model;
    }
    void set_rfid(const std::string& rfid) {
        rfid_ = rfid;
    }
    void set_task_ids(const std::vector<TaskID>& _task_ids) {
        task_ids_ = &_task_ids;
    }

    VAR_PTR_GET(Model, model)
    VAR_PTR_GET(std::vector<TaskID>, task_ids)

private:
    VAR_GET(std::string, rfid)

};
// using TaskGroupMap = std::map<
//     std::string,    // rfid
//     TaskGroup
// >;

}