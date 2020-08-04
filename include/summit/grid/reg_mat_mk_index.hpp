#pragma once
#include <vector>
#include <map>
#include <ChipImgProc/utils.h>
#include <Nucleona/tuple.hpp>
#include <iostream>
#include <ChipImgProc/marker/detection/mk_region.hpp>
namespace summit::grid {

struct RegMatMkIndex {
    using MKRegion = chipimgproc::marker::detection::MKRegion;

    RegMatMkIndex(const std::vector<MKRegion>& mks) // column major
    : markers_(mks)
    {}

    bool search(int cl_x, int cl_y, cv::Rect& mk_reg, int& mk_id_x, int& mk_id_y) const {
        for(auto& mk : markers_) {
            if( mk.contains(cv::Point(cl_x, cl_y)) ) {
                mk_reg  = mk;
                mk_id_x = mk.x_i;
                mk_id_y = mk.y_i;
                return true;
            }
        }
        return false;
    }
private:
    const std::vector<MKRegion>&    markers_;
};

}