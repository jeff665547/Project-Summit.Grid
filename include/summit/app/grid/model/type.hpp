#pragma once
#include <cstdint>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <ChipImgProc/multi_warped_mat.hpp>
#include <ChipImgProc/warped_mat.hpp>
#include <optional>
namespace summit::app::grid::model {
    using GridLineID    = std::uint16_t;
    using GLID          = GridLineID;
    using Float         = float;
    using TiledMat      = chipimgproc::TiledMat<GLID>;
    using StatMats      = chipimgproc::stat::Mats<Float>;
    using MTMat         = chipimgproc::MultiTiledMat<Float, GLID>;
    using OptMTMat      = std::optional<MTMat>;
    using GLRawImg      = chipimgproc::GridRawImg<GLID>;
    using GLRawImgFlt   = chipimgproc::GridRawImg<Float>;

    using WarpedMat     = chipimgproc::WarpedMat<>;
    using MWMat         = chipimgproc::MultiWarpedMat<WarpedMat>;
}