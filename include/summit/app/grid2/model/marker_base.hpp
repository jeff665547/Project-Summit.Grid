#pragma once
#include <map>
#include <ChipImgProc/utils.h>
#include <summit/config/chip.hpp>
#include <ChipImgProc/marker/loader.hpp>
#include <mutex>
namespace summit::app::grid2 {
struct MarkerPair {
    cv::Mat_<std::uint8_t> marker;
    cv::Mat_<std::uint8_t> mask;
};
using MarkerPatterns = std::map<
    std::string, // marker type (AM1/AM3)
    std::vector<MarkerPair>
>
using ChipRegMatMarkers = std::map<
    std::string, // chip spec name
    MarkerPatterns // Cell level marker types
>;

struct MarkerBase {
    
    MarkerPatterns& reg_mat_chip_mks(const std::string& spec_name) {
        auto itr = chip_reg_mat_markers_.find(spec_name);
        if(itr == chip_reg_mat_markers_.end()) {
            std::lock_guard<std::mutex> lock(chip_reg_mat_markers_mux_);
            if(auto itr2 = chip_reg_mat_markers_.find(spec_name);
                itr2 != chip_reg_mat_markers_.end()) return itr2->second;
            auto& mk_pats = chip_reg_mat_markers_[spec_name];
            auto& jchip_spec = summit::config::chip().get_spec(spec_name);
            auto& jpats_paths = jchip_spec.at("mk_pats_cl");
            for(auto&& jmk_pat : jpats_paths) {
                std::string mk_type_name = jmk_pat.at("marker_type");
                std::string mk_path = jmk_pat.at("path");
                auto path = summit::install_path() / mk_path;
                std::ifstream fin(path.string());
                auto [mk_cl, mask_cl] = chipimgproc::marker::Loader::from_txt(fin);
                mk_pats[mk_type_name].push_back({mk_cl, mask_cl});
            }
        }
        return itr->second;
    }

private:
    std::mutex chip_reg_mat_markers_mux_;
    ChipRegMatMarkers chip_reg_mat_markers_;
};

}