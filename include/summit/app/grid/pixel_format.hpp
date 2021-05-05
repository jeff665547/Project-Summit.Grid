/**
 * @file pixel_format.hpp
 * @author Chi-Hsuan Ho (jeffho@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::PixelFormat
 * 
 */
#pragma once
#include <iostream>
#include <string>
#include <map>
#include <cmath>
namespace summit::app::grid {
struct PixelFormat {
    /**
     * @brief All supported pixel formats label.
     * 
     */
    enum Formats{
        unknown,
        Mono8,
        Mono10,
        Mono12,
        Mono14,
        Mono16
    };
    /**
     * @brief Covert string to pixel format label.
     * 
     * @param str Pixel format name in string.
     * @return Pixel format label.
     */
    static Formats from_string(const std::string& str) {
        static std::map<std::string, Formats> mapper {
            {"Mono8"   , Mono8  },
            {"Mono10"  , Mono10 },
            {"Mono12"  , Mono12 },
            {"Mono14"  , Mono14 },
            {"Mono16"  , Mono16 }
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
     * @param fmt Pixel format label.
     * @return std::string Pixel format name in string.
     */
    static std::string to_string(const Formats& fmt) {
        static std::map<Formats, std::string> mapper {
            { Mono8,   "Mono8"  },
            { Mono10,  "Mono10" },
            { Mono12,  "Mono12" },
            { Mono14,  "Mono14" },
            { Mono16,  "Mono16" } 
        };
        try {
            return mapper.at(fmt);
        } catch(...) {
            return "unkonwn";
        }
    }
    /**
     * @brief The theoritical maximum value of the intensity for the corresponding image pixel format.
     * 
     * @param fmt Pixel format label.
     * @return double The maximum value for the corresponding image pixel format.
     */
    static double to_theor_max_val(const std::string& str) {
        Formats fmt = from_string(str);
        static std::map<Formats, double> mapper {
            { Mono8,   std::pow(2,  8) - 1  },
            { Mono10,  std::pow(2, 10) - 1  },
            { Mono12,  std::pow(2, 12) - 1  },
            { Mono14,  std::pow(2, 14) - 1  },
            { Mono16,  std::pow(2, 16) - 1  }
        };
        try {
            return mapper.at(fmt);
        } catch(...) {
            return std::pow(2, 16)-1;
        }
    }
};

}