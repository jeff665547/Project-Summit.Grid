#pragma once
#include <vector>
#include <string>
#include <Nucleona/algo/split.hpp>
#include <set>
#include "output_format.hpp"
namespace summit::app::grid2 {

struct FormatDecoder {
    FormatDecoder()
    : enabled_heatmap_fmts_()
    , enable_cen_(false)
    {}
    FormatDecoder(const std::vector<std::string>& formats ) 
    : enabled_heatmap_fmts_ ()
    , enable_cen_           ( false )
    {
        set_formats(formats);
    }
    void set_formats(const std::vector<std::string>& formats) {
        for( auto& n : formats) {
            enabled_heatmap_fmts_.push_back(
                OutputFormat::from_string(n)
            );
            if( n == "cen_file") {
                enable_cen_ = true;
            }
        }
    }
    const auto& enabled_heatmap_fmts() const {
        return enabled_heatmap_fmts_;
    }
    bool enable_cen() const {
        return enable_cen_;
    }
    bool enable_array() const {
        return enable_cen();
    }
protected:
    std::vector<OutputFormat::Labels>   enabled_heatmap_fmts_   ;
    bool                                enable_cen_             ;
};

}