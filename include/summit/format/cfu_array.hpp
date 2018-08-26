#pragma once
#include <ChipImgProc/marker/layout.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <CFU/format/chip_sample/array.hpp>
#include <summit/utils/datetime.hpp>
#include <nlohmann/json.hpp>
namespace summit::format {

cfu::format::chip_sample::Array init_array(
    const nlohmann::json& chip_spec
);

// using FLOAT = float;
// using GLID = int;
template<class FLOAT, class GLID>
void push_to_cfu_array(
    cfu::format::chip_sample::Array& array,
    chipimgproc::MultiTiledMat<FLOAT, GLID> multi_tiled_mat,
    const std::string& ch_name
) {
    cfu::format::chip_sample::Channel channel;
    channel.name = ch_name;
    using Rows = decltype(multi_tiled_mat.rows());
    using Cols = decltype(multi_tiled_mat.cols());
    auto vec_size = multi_tiled_mat.rows() * multi_tiled_mat.cols();
    channel.intensity       .reserve(vec_size); 
    channel.stddev          .reserve(vec_size); 
    channel.pixel           .reserve(vec_size); 
    // channel.mask            .reserve(vec_size); 
    // channel.outlier         .reserve(vec_size); 
    channel.raw_pixel_value .reserve(vec_size);    
    channel.cv_value        .reserve(vec_size); 
    for(Rows r = 0; r < multi_tiled_mat.rows(); r ++ ) {
        for(Cols c = 0; c < multi_tiled_mat.cols(); c ++ ) {
            auto [pixels, all_info] = multi_tiled_mat.at(r, c, multi_tiled_mat.min_cv_all_data());
            std::vector<typename decltype(channel.raw_pixel_value)::value_type::value_type> raw_pixels(
                pixels.template begin<std::uint16_t>(), pixels.template end<std::uint16_t>() // NOTE: a little bit dangerous...
            );
            channel.intensity       .push_back(all_info.mean);
            channel.stddev          .push_back(all_info.stddev);
            channel.pixel           .push_back(all_info.num);
            // channel.mask            .push_back();
            // channel.outlier         .push_back();
            channel.raw_pixel_value .push_back(raw_pixels);
            channel.cv_value        .push_back(all_info.cv);
        }
    }
    array.channels().push_back(channel);
}

}