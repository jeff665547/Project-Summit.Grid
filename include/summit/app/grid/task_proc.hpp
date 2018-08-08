#pragma once
#include <ChipImgProc/multi_tiled_mat.hpp>
namespace summit::app::grid {

struct TaskProc
{
    using Float = float;
    using GridLineID = std::uint16_t;
    // virtual chipimgproc::MultiTiledMat<Float, GridLineID>
    //     operator()(
    //         const boost::filesystem::path&  src_path,
    //         const std::string&              chip_type,
    //         
    //     )
};

}