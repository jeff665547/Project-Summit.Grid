#pragma once
#include <string>
#include <map>
namespace summit::app::grid2 {

struct OutputFormat {
    enum Labels{
        unknown,
        tsv_probe_list, 
        csv_probe_list, 
        html_probe_list,
        cen_file,
        csv_matrix
    };
    static Labels from_string(const std::string& str) {
        static std::map<std::string, Labels> mapper {
            {"tsv_probe_list"   , tsv_probe_list    },
            {"csv_probe_list"   , csv_probe_list    },
            {"html_probe_list"  , html_probe_list   },
            {"cen_file"         , cen_file          },
            {"csv_matrix"       , csv_matrix        }
        };
        try {
            return mapper.at(str);
        } catch(...) {
            // TODO: logger
            return unknown;
        }
    }
    static std::string to_string(const Labels& lab) {
        static std::map<Labels, std::string> mapper {
            { tsv_probe_list,   "tsv_probe_list"  },
            { csv_probe_list,   "csv_probe_list"  },
            { html_probe_list,  "html_probe_list" },
            { cen_file,         "cen_file"        },
            { csv_matrix,       "csv_matrix"      }  
        };
        try {
            return mapper.at(lab);
        } catch(...) {
            return "unkonwn";
        }
    }
    static std::string to_file_postfix(const Labels& lab) {
        static std::map<Labels, std::string> mapper {
            { tsv_probe_list,   ".tsv"  },
            { csv_probe_list,   ".csv"  },
            { html_probe_list,  ".html" },
            { cen_file,         ".cen"  },
            { csv_matrix,       "-mat.csv"}  
        };
        try {
            return mapper.at(lab);
        } catch(...) {
            return "unkonwn";
        }
    }
};

}