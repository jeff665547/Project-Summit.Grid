#pragma once
#include <iostream>
#include "cell_info.hpp"
#include "cell_info_writer.hpp"
#include <fstream>
namespace summit::app::grid::heatmap_writer{

template<class FLOAT, class GLID>
struct TsvWriter : public CellInfoWriter<FLOAT, GLID> {
    TsvWriter(std::ostream& os, const std::string& delim = "\t")
    : os_   (    os )
    , delim_( delim )
    {
        os_ << fields(
                "task_id", "x", "y", 
                "num", "mean", "stddev", "cv", 
                "is_mk", "mk_x", "mk_y", 
                "mk_sub_x", "mk_sub_y" 
            );
    }
    template<class T>
    std::string fields_(const T& str) {
        std::stringstream ss;
        ss << str;
        return ss.str();
    }
    std::string fields__() {
        return "";
    }
    template<class ARG1, class... ARGS>
    std::string fields__(ARG1&& arg1, ARGS&&... args ) {
        return delim_ + fields_(arg1) + fields__(args...);
    }
    template<class ARG1, class... ARGS>
    std::string fields(ARG1&& arg1, ARGS&&... args ) {
        return fields_(arg1) + fields__(args...) + "\n";
    }
    std::string bool_str(bool b) {
        return b ? "true" : "false";
    }
    virtual void write(const CellInfo<FLOAT, GLID>& ci, const std::string& task_id) override {
        os_ <<
            fields(
                task_id, ci.cl_x, ci.cl_y,
                ci.height * ci.width, ci.mean, ci.stddev, ci.cv,
                bool_str(ci.marker_info.is_marker), ci.marker_info.mk_id_x, ci.marker_info.mk_id_y,
                ci.marker_info.sub_x, ci.marker_info.sub_y
            )
        ;

    }
    virtual bool is_write_entire_mat() { return false; }
    virtual void write(
        const chipimgproc::MultiTiledMat<FLOAT, GLID>& mat, 
        const std::string& task_id, 
        const std::string& ch_name, 
        const std::string& filter
    ) {}
    virtual void flush() override {}
    virtual void close() override {}
    virtual ~TsvWriter() override {}

private:
    std::ostream&   os_     ;
    std::string     delim_  ;
};

template<class FLOAT, class GLID>
struct TsvFileWriter 
: protected std::ofstream
, public TsvWriter<FLOAT, GLID> 
{
    TsvFileWriter(const std::string& path, const std::string& delim = "\t")
    : std::ofstream(path)
    , TsvWriter<FLOAT, GLID>(*this, delim)
    {}
    virtual void flush() override {
        TsvWriter<FLOAT, GLID>::flush();
        std::ofstream::flush();
    }
    virtual void close() override {
        std::ofstream::close();
    }
    virtual ~TsvFileWriter() override {}

};

}