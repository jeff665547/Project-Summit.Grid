#pragma once
#include <CFU/format/chip_sample/array.hpp>
#include "cell_info_writer.hpp"
#include <summit/format/cfu_array.hpp>
#include <CFU/format/cen/file.hpp>
#include <fmt/format.h>
namespace summit::app::grid::heatmap_writer{

template<class FLOAT, class GLID>
struct CENWriter 
: public CellInfoWriter<FLOAT, GLID>
{
    template<class Task>
    CENWriter(const std::string& path, const Task& task)
    : array_()
    , file_path_(path)
    {
        init(task);
    }
    template<class Task>
    void init(const Task& task) {
        array_.date() = summit::utils::datetime("%Y%m%d%H%M%S", std::chrono::system_clock::now()); 
        array_.type() = task.chip_spec_name();
        array_.feature_columns() = task.spec_w_cl();
        array_.feature_rows()    = task.spec_h_cl();
    }
    virtual void write(const CellInfo<FLOAT, GLID>& ci, const std::string& task_id) override {}
    virtual bool is_write_entire_mat() override { return true; }
    virtual void write(
        const chipimgproc::MultiTiledMat<FLOAT, GLID>& mat, 
        const std::string& task_id, 
        const std::string& ch_name, 
        const int&         ch_id,
        const std::string& filter
    ) override {
        std::string cen_ch_name = fmt::format("channel-{}", ch_id);
        summit::format::push_to_cfu_array(array_, mat, cen_ch_name);
    }
    virtual void flush() override {
        cfu::format::cen::File file(file_path_, H5F_ACC_TRUNC);
        file.fill_data(array_);
    }
    virtual void close() override {}
    virtual ~CENWriter() override {}
private:
    cfu::format::chip_sample::Array array_;
    std::string file_path_;
};

}