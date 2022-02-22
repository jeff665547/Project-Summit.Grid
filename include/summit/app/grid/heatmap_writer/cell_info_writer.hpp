#pragma once
#include <iostream>
#include "cell_info.hpp"
#include <summit/app/grid/model/type.hpp>
namespace summit::app::grid::heatmap_writer{

struct CellInfoWriter {
    virtual void write(const CellInfo& ci, const std::string& task_id) = 0;
    virtual void write_heatmap(const CellInfo& ci, const std::string& task_id) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual bool is_write_entire_mat() = 0;
    virtual void write(
        const model::MWMat& mat, 
        const std::string& task_id, 
        const std::string& ch_name, 
        const int&         ch_id,
        const std::string& filter
    ) = 0;
    virtual ~CellInfoWriter() {}
};

}