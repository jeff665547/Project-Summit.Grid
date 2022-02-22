#pragma once
#include <iostream>
#include "cell_info.hpp"
#include "cell_info_writer.hpp"
namespace summit::app::grid::heatmap_writer{

struct HtmlTableWriter : public CellInfoWriter {
    HtmlTableWriter(std::ostream& os)
    : os_ ( os )
    {
        os_ << "<html>";
        os_ << "<table>";
        os_ << htag("tr",
            htag(
                "th", "task_id", "probe", "x", "y", 
                "num", "mean", "stddev", "cv", 
                "is_mk", "mk_x", "mk_y", 
                "mk_sub_x", "mk_sub_y" 
            )
        );
    }
    template<class T>
    std::string htag_(const std::string& tag, const T& str) {
        std::stringstream ss;
        ss << "<" << tag << ">";
        ss << str;
        ss << "</" << tag << ">";
        return ss.str();
    }
    std::string htag( const std::string& str ) {
        return "";
    }
    template<class ARG1, class... ARGS>
    std::string htag(const std::string& tag, ARG1&& arg1, ARGS&&... args ) {
        return htag_(tag, arg1) + htag(tag, args...);
    }
    static cv::Mat to_u8( const cv::Mat& pix) {
        cv::Mat res;
        pix.convertTo(res, CV_8U, 0.00390625);
        return res;
    }
    std::string img_tag(const cv::Mat& raw) {
        auto b64 = chipimgproc::jpg_base64(to_u8(raw));
        std::stringstream ss;
        ss << "<img src=\"data:image/jpg;base64," << b64 << "\"/>";
        return ss.str();
    }
    std::string bool_str(bool b) {
        return b ? "true" : "false";
    }
    virtual void write(const CellInfo& ci, const std::string& task_id) override {
        os_ << htag("tr",
            htag(
                "td", task_id, img_tag(ci.probe), ci.cl_x, ci.cl_y,
                ci.num, ci.mean, ci.stddev, ci.cv,
                bool_str(ci.marker_info.is_marker), ci.marker_info.mk_id_x, ci.marker_info.mk_id_y,
                ci.marker_info.sub_x, ci.marker_info.sub_y
            )
        );

    }
    virtual void write_heatmap(const CellInfo& ci, const std::string& task_id) override {}
    virtual void write(
        const model::MWMat& mat, 
        const std::string&  task_id, 
        const std::string&  ch_name, 
        const int&          ch_id,
        const std::string&  filter
    ) {}
    virtual bool is_write_entire_mat() override { return false; }
    virtual void flush() override {
        os_ << "</table>";
        os_ << "</html>";
    }
    virtual void close() override {}
    virtual ~HtmlTableWriter() override {}

private:
    std::ostream& os_;
};
struct HtmlTableFileWriter 
: protected std::ofstream
, public HtmlTableWriter 
{
    HtmlTableFileWriter(const std::string& path)
    : std::ofstream(path)
    , HtmlTableWriter(static_cast<std::ofstream&>(*this))
    {}
    virtual void flush() override {
        HtmlTableWriter::flush();
        std::ofstream::flush();
    }
    virtual void close() override {
        std::ofstream::close();
    }
    virtual ~HtmlTableFileWriter() override {}
};

}