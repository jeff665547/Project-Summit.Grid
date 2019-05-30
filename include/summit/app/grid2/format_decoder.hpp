#pragma once
#include <vector>
#include <string>
#include <Nucleona/algo/split.hpp>
#include <set>
#include "output_format.hpp"
namespace summit::app::grid2 {

struct FormatDecoder {
    FormatDecoder(const std::vector<std::string>& formats ) 
    : names_                ( formats )
    , enabled_heatmap_fmts_ ()
    , enable_cen_           ( false )
    {
        static const std::set<std::string> heatmap_fmts = {
            "tsv_probe_list", "csv_probe_list", 
            "html_probe_list", "cen_file",
            "csv_matrix"
        };
        for( auto& n : names_ ) {
            if( heatmap_fmts.find(n) != heatmap_fmts.end() ) {
                enabled_heatmap_fmts_.push_back(n);
            }
            if( n == "cen") {
                enable_cen_ = true;
            }
        }
    }
    const std::vector<std::string>& names() const {
        return names_;
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
    std::vector<std::string> names_      ;
    std::vector<OutputFormat::Labels> enabled_heatmap_fmts_;
    bool                     enable_cen_ ;
};

}