#pragma once
#include <ChipImgProc/utils/cv.h>
#include <algorithm>
namespace summit::app {

struct LightMean {
    double operator()(std::basic_string<double>& src, int light_num) const {
        std::sort(src.begin(), src.end(), [](auto&& a, auto&& b){
            return b < a;
        });
        std::basic_string_view<double> sv(src);
        auto upper_mk = sv.substr(0, light_num);
        double sum = 0;
        for(auto&& v : upper_mk) {
            sum += v;
        }
        return sum / light_num;
    }
};

}