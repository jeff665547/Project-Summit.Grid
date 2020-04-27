/**
 * @file format_decoder.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::FormatDecoder
 */
#pragma once
#include <vector>
#include <string>
#include <Nucleona/algo/split.hpp>
#include <set>
#include "output_format.hpp"
namespace summit::app::grid {

/**
 * @brief Command-line output format option parser.
 * @details Parse the string element of command-line output format options, 
 *      and covert to internal file type label.
 */
struct FormatDecoder {
    /**
     * @brief Construct a new Format Decoder object.
     * 
     */
    FormatDecoder()
    : enabled_heatmap_fmts_()
    {}
    /**
     * @brief Construct a new Format Decoder object.
     * 
     * @param formats Command-line output format option required to generate.
     */
    FormatDecoder(const std::vector<std::string>& formats ) 
    : enabled_heatmap_fmts_ ()
    {
        set_formats(formats);
    }
    /**
     * @brief Add the formats without check the duplicate.
     * 
     * @param formats The formats required to generate.
     */
    void set_formats(const std::vector<std::string>& formats) {
        for( auto& n : formats) {
            enabled_heatmap_fmts_.push_back(
                OutputFormat::from_string(n)
            );
        }
    }
    /**
     * @brief Add the formats with check and avoid the duplicate.
     * 
     * @param formats The formats required to generate.
     */
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
    /**
     * @brief Covert internal labels to strings.
     * 
     * @return auto A vector of string.
     */
    auto to_string() const {
        std::vector<std::string> res;
        for(auto&& lab : enabled_heatmap_fmts_) {
            res.push_back(OutputFormat::to_string(lab));
        }
        return res;
    }
    /**
     * @brief The formats will generated in current process.
     * 
     * @return const auto& A vector of format labels.
     */
    const auto& enabled_heatmap_fmts() const {
        return enabled_heatmap_fmts_;
    }
protected:
    /**
     * @brief The formats will generated in current process.
     * 
     */
    std::vector<OutputFormat::Labels>   enabled_heatmap_fmts_   ;
};

}