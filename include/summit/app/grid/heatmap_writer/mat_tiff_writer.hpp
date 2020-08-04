#pragma once
#include "cell_info_writer.hpp"
namespace summit::app::grid::heatmap_writer{

struct MatTiffWriter 
: public CellInfoWriter
{
    MatTiffWriter(const std::string& path)
    : file_path_(path)
    {
    }
    virtual void write(const CellInfo& ci, const std::string& task_id) override {}
    virtual bool is_write_entire_mat() override { return true; }
    virtual void write(
        const model::MWMat& mat, 
        const std::string& task_id, 
        const std::string& ch_name, 
        const int&         ch_id,
        const std::string& filter
    ) override {
        cv::Mat_<std::uint16_t> res(mat.rows(), mat.cols());
        for(int i = 0; i < mat.rows(); i ++) {
            for(int j = 0; j < mat.cols(); j ++) {
                res(i, j) = std::round(mat.at_cell(i, j).mean);
            }
        }
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