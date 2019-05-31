#pragma once
#include <ChipImgProc/utils.h>
#include <map>
// TODO: remove this
namespace summit::app::grid2 {

struct SupImprocData
{
    std::map<cv::Point, float, 
        chipimgproc::PointLess> backgrounds;
};

}