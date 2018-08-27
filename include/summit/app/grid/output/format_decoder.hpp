#pragma once
#include <vector>
#include <string>
#include <Nucleona/algo/split.hpp>
#include <set>
namespace summit::app::grid::output {

struct FormatDecoder {
    FormatDecoder(const std::vector<std::string>& formats ) 
    : names_                ( formats )
    , enabled_heatmap_fmts_ ()
    , enable_cen_           ( false )
    {
        static const std::set<std::string> heatmap_fmts = {
            "tsv", "csv", "html"
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
    const std::vector<std::string>& enabled_heatmap_fmts() const {
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
    std::vector<std::string> enabled_heatmap_fmts_;
    bool                     enable_cen_ ;
};

}