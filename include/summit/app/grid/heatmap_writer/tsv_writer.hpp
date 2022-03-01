#pragma once
#include <iostream>
#include "cell_info.hpp"
#include "cell_info_writer.hpp"
#include <fstream>
namespace summit::app::grid::heatmap_writer{

struct TsvWriter : public CellInfoWriter {
    TsvWriter(std::ostream& os, const std::string& delim = "\t")
    : os_   (    os )
    , delim_( delim )
    {
        fields(
            "task_id", "x", "y", 
            "num", "mean", "stddev", "cv", "bg",
            "img_x", "img_y", "is_mk", 
            "mk_x", "mk_y", 
            "mk_sub_x", "mk_sub_y" 
        );
    }
    void fields__() {
    }
    template<class ARG1, class... ARGS>
    void fields__(ARG1&& arg1, ARGS&&... args ) {
        os_ << delim_ << arg1;
        fields__(args...);
    }
    template<class ARG1, class... ARGS>
    void fields(ARG1&& arg1, ARGS&&... args ) {
        os_ << arg1;
        fields__(args...);
        os_ << "\n";
    }
    std::string bool_str(bool b) {
        return b ? "true" : "false";
    }
    virtual void write(const CellInfo& ci, const std::string& task_id) override {
        fields(
            task_id, ci.cl_x, ci.cl_y,
            ci.num, ci.mean, ci.stddev, ci.cv, ci.bg, 
            ci.img_x, ci.img_y, bool_str(ci.marker_info.is_marker), 
            ci.marker_info.mk_id_x, ci.marker_info.mk_id_y,
            ci.marker_info.sub_x, ci.marker_info.sub_y
        );

    }
    virtual bool is_write_entire_mat() { return false; }
    virtual void write(
        const model::MWMat&   mat, 
        const std::string&   task_id, 
        const std::string&   ch_name, 
        const int&           ch_id,
        const std::string&   filter
    ) {}
    virtual void flush() override {}
    virtual void close() override {}
    virtual ~TsvWriter() override {}

private:
    std::ostream&   os_     ;
    std::string     delim_  ;
};

struct TsvFileWriter 
: protected std::ofstream
, public TsvWriter
{
    TsvFileWriter(const std::string& path, const std::string& delim = "\t")
    : std::ofstream(path)
    , TsvWriter(*this, delim)
    {}
    virtual void flush() override {
        TsvWriter::flush();
        std::ofstream::flush();
    }
    virtual void close() override {
        std::ofstream::close();
    }
    virtual ~TsvFileWriter() override {}

};

}