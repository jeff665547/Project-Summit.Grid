#pragma once
#include "cell_info_writer.hpp"
namespace summit::app::grid::heatmap_writer{

template<class FLOAT, class GLID>
struct MatTiffWriter 
: public CellInfoWriter<FLOAT, GLID>
{
    MatTiffWriter(const std::string& path)
    : file_path_(path)
    {
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
        auto res_float = mat.dump();
        cv::Mat res;
        res_float.convertTo(res, CV_16U);
        cv::imwrite(file_path_, res);
    }
    virtual void flush() override {
    }
    virtual void close() override {}
    virtual ~MatTiffWriter() override {}
private:
    std::string file_path_;
};

}