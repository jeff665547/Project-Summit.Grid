#pragma once
#include <map>
#include <ChipImgProc/utils.h>
#include <summit/config/chip.hpp>
#include <ChipImgProc/marker/loader.hpp>
#include <mutex>
#include <summit/utils.h>
namespace summit::app::grid::model {

struct MKPat {
    nlohmann::json meta;
    cv::Mat_<std::uint8_t> marker;
    cv::Mat_<std::uint8_t> mask;
};
using MarkerPatterns = std::vector<MKPat>;
struct ChipSpecMarkerBase {
    MarkerPatterns data;
    auto get_by_marker_type(const std::string& mk_type) const {
        return marker_type_index.at(mk_type);
    }
    std::map<std::string, std::vector<MKPat*>> marker_type_index;
};
using ChipRegMatMarkers = std::map<
    std::string, // chip spec name
    ChipSpecMarkerBase
>;

struct MarkerBase {
    
    const ChipSpecMarkerBase& reg_mat_chip_mks(
        const std::string& spec_name, 
        const nlohmann::json& sh_mk_pats_cl,
        const nlohmann::json& sh_mk_pats
    ) {
        auto itr = chip_reg_mat_markers_.find(spec_name);
        if(itr == chip_reg_mat_markers_.end()) {
            std::lock_guard<std::mutex> lock(chip_reg_mat_markers_mux_);
            if(auto itr2 = chip_reg_mat_markers_.find(spec_name);
                itr2 != chip_reg_mat_markers_.end()
            ) return itr2->second;
            auto& mk_pats = chip_reg_mat_markers_[spec_name];
            for(auto jmk_pat : sh_mk_pats_cl) {
                jmk_pat["cl"] = true;
                std::string mk_path = jmk_pat.at("path");
                auto path = summit::install_path() / mk_path;
                std::ifstream fin(path.string());
                auto [mk_cl, mask_cl] = chipimgproc::marker::Loader::from_txt(fin);
                MKPat mk_pat {jmk_pat, mk_cl, mask_cl};
                mk_pats.data.emplace_back(std::move(mk_pat));
            }
            for(auto jmk_pat : sh_mk_pats) {
                jmk_pat["cl"] = false;
                std::string mk_path = jmk_pat.at("path");
                auto path = summit::install_path() / mk_path;
                auto mk_img = chipimgproc::imread(path);
                MKPat mk_pat {jmk_pat, mk_img, cv::Mat_<std::uint8_t>()};
                mk_pats.data.emplace_back(std::move(mk_pat));
            }
            auto& marker_type_index = mk_pats.marker_type_index;
            for(auto&& mk_pat : mk_pats.data) {
                auto& meta = mk_pat.meta;
                auto itr = meta.find("marker_type");
                if(itr == meta.end()) continue;
                auto pat_mk_type = itr->get<std::string>();
                marker_type_index[pat_mk_type].push_back(&mk_pat);
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