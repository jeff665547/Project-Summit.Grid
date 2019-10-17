#pragma once
#include <vector>
#include <string>
#include <Nucleona/algo/split.hpp>
#include <set>
#include "output_format.hpp"
namespace summit::app::grid {

struct FormatDecoder {
    FormatDecoder()
    : enabled_heatmap_fmts_()
    {}
    FormatDecoder(const std::vector<std::string>& formats ) 
    : enabled_heatmap_fmts_ ()
    {
        set_formats(formats);
    }
    void set_formats(const std::vector<std::string>& formats) {
        for( auto& n : formats) {
            enabled_heatmap_fmts_.push_back(
                OutputFormat::from_string(n)
            );
        }
    }
    void add_formats(const std::vector<std::string>& formats) {
        for( auto& n : formats) {
            auto ftag = OutputFormat::from_string(n);
            auto itr = std::find(
                enabled_heatmap_fmts_.begin(), 
                enabled_heatmap_fmts_.end(), 
                ftag
            );
            if(itr == enabled_heatmap_fmts_.end()) {
                enabled_heatmap_fmts_.push_back(ftag);
            }
        }
    }
    auto to_string() const {
        std::vector<std::string> res;
        for(auto&& lab : enabled_heatmap_fmts_) {
            res.push_back(OutputFormat::to_string(lab));
        }
        return res;
    }
    const auto& enabled_heatmap_fmts() const {
        return enabled_heatmap_fmts_;
    }
protected:
    std::vector<OutputFormat::Labels>   enabled_heatmap_fmts_   ;
};

}