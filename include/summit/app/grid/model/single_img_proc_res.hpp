#pragma once
#include <ChipImgProc/tiled_mat.hpp>
#include <ChipImgProc/stat/mats.hpp>
#include <optional>
namespace summit::app::grid::model {

struct SingleImgProcRes {
    chipimgproc::TiledMat<> tiled_mat;
    chipimgproc::stat::Mats<> stat_mats;
    std::vector<float> bg_means;
};

using OptSingleImgProcRes = std::optional<SingleImgProcRes>;

}