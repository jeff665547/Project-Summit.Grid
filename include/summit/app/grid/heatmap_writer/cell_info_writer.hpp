#pragma once
#include <iostream>
#include "cell_info.hpp"
#include <ChipImgProc/multi_tiled_mat.hpp>
namespace summit::app::grid::heatmap_writer{

template<class FLOAT, class GLID>
struct CellInfoWriter {
    virtual void write(const CellInfo<FLOAT, GLID>& ci, const std::string& task_id) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual bool is_write_entire_mat() = 0;
    virtual void write(
        const chipimgproc::MultiTiledMat<FLOAT, GLID>& mat, 
        const std::string& task_id, 
        const std::string& ch_name, 
        const std::string& filter
    ) = 0;
    virtual ~CellInfoWriter() {}
};

}