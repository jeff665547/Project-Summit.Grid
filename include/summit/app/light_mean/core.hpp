#pragma once
#include <ChipImgProc/utils/cv.h>
#include <algorithm>
#include <summit/config/chip.hpp>
#include <summit/utils.h>
#include <ChipImgProc/marker/loader.hpp>
#include <numeric>
#include <fmt/format.h>
#include "heatmap_parser.hpp"
#include <Nucleona/range.hpp>
namespace summit::app::light_mean {

struct Core {
    auto get_marker_type_path( 
        const nlohmann::json& chip, 
        const std::string& marker_type
    ) const {
        auto mk_pats = chip["shooting_marker"]["mk_pats_cl"];
        for(auto&& mkp : mk_pats) {
            if( mkp["marker_type"].get<std::string>() == marker_type ) {
                return 
                    summit::install_path() / mkp["path"].get<std::string>();
            }
        }
        throw std::runtime_error(
            fmt::format("marker type: {}, doesn't exist in current chip info", marker_type)
        );
    }
    std::vector<float> extract_light_probe(
        const cv::Mat_<std::uint8_t>&   marker_spec, 
        const cv::Mat_<float>&          marker
    ) const {
        std::vector<float> res;
        for(int i = 0; i < marker_spec.rows; i ++ ) {
            for(int j = 0; j < marker_spec.cols; j ++ ) {
                if(marker_spec(i, j) == 255) {
                    res.push_back(marker(i, j));
                }
            }
        }
        return res;
    }
    struct StatResult {
        float mean;
        float stddev;
        float cv;
    };
    template<class Rng>
    auto statistic(Rng&& v) const {
        float sum = std::accumulate(v.begin(), v.end(), 0.0);
        float mean = sum / v.size();

        float sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
        float stdev = std::sqrt(sq_sum / v.size() - mean * mean);
        return StatResult{mean, stdev, stdev / mean};
    }
    template<class Rng>
    auto select_light_probe(
        Rng&& ents,
        const std::string& chip_spec,
        const std::string& marker_type 
    ) const {
        auto chip = summit::config::chip().get_spec(chip_spec);
        auto marker_path = get_marker_type_path(chip, marker_type);
        std::ifstream marker_file(marker_path.string());
        auto [marker_spec, mask] = chipimgproc::marker::Loader::from_txt(marker_file);
        std::vector<std::size_t> light_probes;
        for(std::size_t i = 0; i < ents.size(); i ++) {
            auto&& e = ents.at(i);
            if(marker_spec(e.mk_sub_y, e.mk_sub_x) == 255) {
                light_probes.push_back(i);
            }
        }
        return light_probes;

        // for(auto&& mk : markers) {
        //     auto tmp = extract_light_probe(marker_spec, mk);
        //     light_probes.insert(light_probes.end(), tmp.begin(), tmp.end());
        // }
        // return statistic(light_probes);
    }
    template<class Rng>
    auto operator()(
        Rng&& buffer,
        const std::string& chip_spec,
        const std::string& marker_type 
    ) const {
        auto light_probe_idx = select_light_probe(
            buffer, chip_spec, marker_type
        );
        auto light_probes = buffer 
            | nucleona::range::at(light_probe_idx)
        ;
        auto light_probe_means = light_probes
            | nucleona::range::transformed([](auto&& ent){
                return ent.mean;
            })
        ;
        auto res = statistic(light_probe_means);
        return res;
    }
};

}