/**
 * @file scan_mode.hpp
 * @author Chi-Shuan Ho (jeffho@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::model::ScanMode
 * 
 */
#pragma once
#include <string>
#include <map>

namespace summit::app::grid::model {

/**
 * @brief Enumerated type for all avaliable scan modes and the corresponding string conversion.
 * 
 */
struct ScanMode {
    /**
     * @brief All available scan modes.
     * 
     */
    enum Modes{
        precise,
        regular,
        quick,
        unknown,
        abandoned
    };
    /**
     * @brief Convert string to scan mode.
     * 
     * @param str Scan mode in string.
     * @return Scan mode of type enum Modes.
     */
    static Modes from_string(const std::string& str) {
        static std::map<std::string, Modes> mapper {
            {"precise"    , precise   },
            {"regular"    , regular   },
            {"quick"      , quick     },
            {"unknown"    , unknown   },
            {"abandoned"  , abandoned }
        };
        try {
            return mapper.at(str);
        } catch(...) {
            // TODO: logger
            return unknown;
        }
    }
    /**
     * @brief Convert scan mode to string.
     * 
     * @param mode Scan mode of type enum Modes.
     * @return std::string Scan mode in string.
     */
    static std::string to_string(const Modes& mode) {
        static std::map<Modes, std::string> mapper {
            { precise,   "precise"   },
            { regular,   "regular"   },
            { quick,     "quick"     },
            { unknown,   "unknown"   },
            { abandoned, "abandoned" }  
        };
        try {
            return mapper.at(mode);
        } catch(...) {
            return "unkonwn";
        }
    }
};

}