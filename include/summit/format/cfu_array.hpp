#pragma once
#include <ChipImgProc/marker/layout.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <CFU/format/chip_sample/array.hpp>
#include <summit/utils/datetime.hpp>
#include <nlohmann/json.hpp>
#include <ChipImgProc/multi_warped_mat.hpp>
namespace summit::format {

cfu::format::chip_sample::Array init_array(
    const nlohmann::json& chip_spec
);

// using FLOAT = float;
// using GLID = int;
template<class FOV>
void push_to_cfu_array(
    cfu::format::chip_sample::Array& array,
    const chipimgproc::MultiWarpedMat<FOV, true>& multi_warped_mat,
    const std::string& ch_name
) {
    cfu::format::chip_sample::Channel channel;
    channel.name = ch_name;
    using Rows = decltype(multi_warped_mat.rows());
    using Cols = decltype(multi_warped_mat.cols());
    auto vec_size = multi_warped_mat.rows() * multi_warped_mat.cols();
    channel.intensity       .reserve(vec_size); 
    channel.stddev          .reserve(vec_size); 
    channel.pixel           .reserve(vec_size); 
    // channel.mask            .reserve(vec_size); 
    // channel.outlier         .reserve(vec_size); 
    channel.raw_pixel_value .reserve(vec_size);    
    channel.cv_value        .reserve(vec_size); 
    for(Rows r = 0; r < multi_warped_mat.rows(); r ++ ) {
        for(Cols c = 0; c < multi_warped_mat.cols(); c ++ ) {
            auto cell = multi_warped_mat.at_cell(r, c);
            std::vector<int> raw_pixels(
                cell.patch.template begin<std::uint16_t>(), cell.patch.template end<std::uint16_t>() // NOTE: a little bit dangerous...
            );
            channel.intensity       .push_back(cell.mean);
            channel.stddev          .push_back(cell.stddev);
            channel.pixel           .push_back(cell.num);
            // channel.mask            .push_back();
            // channel.outlier         .push_back();
            channel.raw_pixel_value .push_back(raw_pixels);
            channel.cv_value        .push_back(cell.cv);
        }
    }
    array.channels().push_back(channel);
}

}