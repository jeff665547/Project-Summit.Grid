#pragma once
#include <ChipImgProc/utils.h>
#include <map>
// TODO: remove this
namespace summit::app::grid {

struct SupImprocData
{
    std::map<cv::Point, float, 
        chipimgproc::PointLess> backgrounds;
};

}