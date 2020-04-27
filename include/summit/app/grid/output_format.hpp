/**
 * @file output_format.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::OutputFormat
 * 
 */
#pragma once
#include <string>
#include <map>
namespace summit::app::grid {

/**
 * @brief Enum type of all supported output formats and related string conversion.
 * 
 */
struct OutputFormat {
    /**
     * @brief All supported output formats label.
     * 
     */
    enum Labels{
        unknown,
        tsv_probe_list, 
        csv_probe_list, 
        html_probe_list,
        cen_file,
        csv_matrix,
        mat_tiff
    };
    /**
     * @brief Covert string to format label.
     * 
     * @param str Format name in string.
     * @return Labels Format label.
     */
    static Labels from_string(const std::string& str) {
        static std::map<std::string, Labels> mapper {
            {"tsv_probe_list"   , tsv_probe_list    },
            {"csv_probe_list"   , csv_probe_list    },
            {"html_probe_list"  , html_probe_list   },
            {"cen_file"         , cen_file          },
            {"csv_matrix"       , csv_matrix        },
            {"mat_tiff"         , mat_tiff          }
        };
        try {
            return mapper.at(str);
        } catch(...) {
            // TODO: logger
            return unknown;
        }
    }
    /**
     * @brief Convert format label to string.
     * 
     * @param lab Format label.
     * @return std::string Format name in string.
     */
    static std::string to_string(const Labels& lab) {
        static std::map<Labels, std::string> mapper {
            { tsv_probe_list,   "tsv_probe_list"  },
            { csv_probe_list,   "csv_probe_list"  },
            { html_probe_list,  "html_probe_list" },
            { cen_file,         "cen_file"        },
            { csv_matrix,       "csv_matrix"      },
            { mat_tiff,         "mat_tiff"        }  
        };
        try {
            return mapper.at(lab);
        } catch(...) {
            return "unkonwn";
        }
    }
    /**
     * @brief The file postfix of the specific output format.
     * 
     * @param lab The output format label.
     * @return std::string The file postfix and extension.
     */
    static std::string to_file_postfix(const Labels& lab) {
        static std::map<Labels, std::string> mapper {
            { tsv_probe_list,   ".tsv"  },
            { csv_probe_list,   ".csv"  },
            { html_probe_list,  ".html" },
            { cen_file,         ".cen"  },
            { csv_matrix,       "-mat.csv"},  
            { mat_tiff,         "-mat.tiff"}
        };
        try {
            return mapper.at(lab);
        } catch(...) {
            return "unkonwn";
        }
    }
};

}