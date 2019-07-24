#pragma once
#include <ChipImgProc/utils/cv.h>
#include <algorithm>
#include <summit/config/chip.hpp>
#include <summit/utils.h>
#include <ChipImgProc/marker/loader.hpp>
#include <numeric>
#include <fmt/format.h>
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
    struct Result {
        float mean;
        float stddev;
        float cv;
    };
    auto statistic(const std::vector<float>& v) const {
        float sum = std::accumulate(v.begin(), v.end(), 0.0);
        float mean = sum / v.size();

        float sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
        float stdev = std::sqrt(sq_sum / v.size() - mean * mean);
        return Result{mean, stdev, stdev / mean};
    }
    Result operator()(
        const std::vector<cv::Mat_<float>>& markers, 
        const std::string& chip_spec,
        const std::string& marker_type 
    ) const {
        auto chip = summit::config::chip().get_spec(chip_spec);
        auto marker_path = get_marker_type_path(chip, marker_type);
        std::ifstream marker_file(marker_path.string());
        auto [marker_spec, mask] = chipimgproc::marker::Loader::from_txt(marker_file);
        std::vector<float> light_probes;
        for(auto&& mk : markers) {
            auto tmp = extract_light_probe(marker_spec, mk);
            light_probes.insert(light_probes.end(), tmp.begin(), tmp.end());
        }
        return statistic(light_probes);
    }
};

}