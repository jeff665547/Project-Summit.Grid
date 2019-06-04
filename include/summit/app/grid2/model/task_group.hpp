#pragma once
#include "macro.hpp"
#include "task.hpp"
namespace summit::app::grid2 {
struct Model;
}
namespace summit::app::grid2::model {

struct TaskGroup : public TaskMap
{
    void set_model(const Model& _model) {
        model_ = &_model;
    }
    void set_rfid(const std::string& rfid) {
        rfid_ = rfid;
    }
    Model& get_model() { return const_cast<Model&>(*model_); }

    VAR_PTR_GET(Model, model)

private:
    VAR_GET(std::string, rfid)

};
using TaskGroupMap = std::map<
    std::string,    // rfid
    TaskGroup
>;

}