#pragma once
#include <cstdint>
#include <ChipImgProc/multi_tiled_mat.hpp>
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
}