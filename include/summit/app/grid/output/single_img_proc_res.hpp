#pragma once
#include <ChipImgProc/tiled_mat.hpp>
#include <ChipImgProc/stat/mats.hpp>
namespace summit::app::grid::output {

struct SingleImgProcRes {
    chipimgproc::TiledMat<> tiled_mat;
    chipimgproc::stat::Mats<> stat_mats;
    std::vector<float> bg_means;
};

}