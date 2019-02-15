#pragma once
#include <ChipImgProc/utils.h>
#include <map>

namespace summit::app::grid::output {

struct SupImprocData
{
    std::map<cv::Point, float, 
        chipimgproc::PointLess> backgrounds;
};

}