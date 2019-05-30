#pragma once
#include <iostream>
#include <summit/app/grid/output/cell_info.hpp>
namespace summit::app::grid2::heatmap_writer{

template<class FLOAT, class GLID>
struct CellInfoWriter {
    virtual void write(const CellInfo<FLOAT, GLID>& ci, const std::string& task_id) = 0;
    virtual std::string ext() = 0;
};

}