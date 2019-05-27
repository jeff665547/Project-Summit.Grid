#pragma once
#include <Nucleona/algo/split.hpp>
#include <Nucleona/language.hpp>
#include <summit/grid/version.h>
namespace summit::grid {

struct Version {
private:
    static auto parse_micro_and_tag( const std::string& str) {
        auto mic_tag = nucleona::algo::split(str, "-");
        auto micro = std::stoi(mic_tag.at(0));
        std::string tag;
        if(mic_tag.size() > 1) {
            tag = mic_tag.at(1);
        }
        return std::make_tuple(micro, tag);
    }
public:
    Version() = default;
    Version(int _major, int _minor, const std::string& _micro_tag )
    : major(_major)
    , minor(_minor)
    {
        std::tie(micro, tag) = parse_micro_and_tag(_micro_tag);
    }
    bool operator<=( const Version& v) const {
        if( major == v.major ) {
            if(minor == v.minor) {
                if( micro == v.micro ) {
                    return true;
                } else return micro < v.micro;
            } else return minor < v.minor;
        } else return major < v.major;
    }
    bool operator>( const Version& v) const {
        return !(operator<=(v));
    }
    std::string to_string() const {
        auto res = 
            std::to_string(major) + "." 
            + std::to_string(minor) + "."
            + std::to_string(micro)
        ;
        if(!tag.empty()) {
            res += ("-" + tag);
        }
        return res;
    }
    static Version parse( const std::string& str) {
        Version vsn;
        auto version_str_vec = nucleona::algo::split(str, ".");

        vsn.major = std::stoi(version_str_vec.at(0));
        vsn.minor = std::stoi(version_str_vec.at(1));
        std::tie(
            vsn.micro, 
            vsn.tag
        ) = parse_micro_and_tag(version_str_vec.at(2));
        return vsn;
    }
    static Version self() {
        return Version(
            SummitGrid_MAJOR_VERSION,
            SummitGrid_MINOR_VERSION,
            UNWRAP_SYM_STR(SummitGrid_MICRO_VERSION)
        );
    }
    static std::string self_str() {
        return UNWRAP_SYM_STR(SummitGrid_VERSION);
    }
    int major;
    int minor;
    int micro;
    std::string tag;
};


}