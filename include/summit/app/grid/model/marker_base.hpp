#pragma once
#include <map>
#include <ChipImgProc/utils.h>
#include <summit/config/chip.hpp>
#include <ChipImgProc/marker/loader.hpp>
#include <mutex>
#include <summit/utils.h>
namespace summit::app::grid::model {

struct MarkersPair {
    std::vector<cv::Mat_<std::uint8_t>> marker;
    std::vector<cv::Mat_<std::uint8_t>> mask;
};
using MarkerPatterns = std::map<
    std::string, // marker type (AM1/AM3)
    MarkersPair
>;
using ChipRegMatMarkers = std::map<
    std::string, // chip spec name
    MarkerPatterns // Cell level marker types
>;

struct MarkerBase {
    
    MarkerPatterns& reg_mat_chip_mks(
        const std::string& spec_name, 
        const nlohmann::json& sh_mk_pats_cl
    ) {
        auto itr = chip_reg_mat_markers_.find(spec_name);
        if(itr == chip_reg_mat_markers_.end()) {
            std::lock_guard<std::mutex> lock(chip_reg_mat_markers_mux_);
            if(auto itr2 = chip_reg_mat_markers_.find(spec_name);
                itr2 != chip_reg_mat_markers_.end()
            ) return itr2->second;
            auto& mk_pats = chip_reg_mat_markers_[spec_name];
            for(auto&& jmk_pat : sh_mk_pats_cl) {
                std::string mk_type_name = jmk_pat.at("marker_type");
                std::string mk_path = jmk_pat.at("path");
                auto path = summit::install_path() / mk_path;
                std::ifstream fin(path.string());
                auto [mk_cl, mask_cl] = chipimgproc::marker::Loader::from_txt(fin);
                auto& mk_pat = mk_pats[mk_type_name];
                mk_pat.marker.push_back(mk_cl);
                mk_pat.mask.push_back(mask_cl);
            }
            return mk_pats;
        } else {
            return itr->second;
        }
    }
    static MarkerBase& get() {
        static MarkerBase base;
        return base;
    }

private:
    std::mutex chip_reg_mat_markers_mux_;
    ChipRegMatMarkers chip_reg_mat_markers_;
};

}