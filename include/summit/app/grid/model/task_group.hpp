#pragma once
#include "macro.hpp"
#include <summit/app/grid/task_id.hpp>
namespace summit::app::grid::model {
struct Model;
}
namespace summit::app::grid::model {

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