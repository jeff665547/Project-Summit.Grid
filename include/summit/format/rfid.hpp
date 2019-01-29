#pragma once
#include <string>
#include <Nucleona/algo/split.hpp>
namespace summit::format {

struct RFID {
    std::string tray_type;
    std::string serial_num;
    std::string region_code;
    static RFID parse( const std::string& rfid ) {
        std::vector<std::string> svec;
        boost::split( svec, rfid, boost::is_any_of("-"));

        auto tray_type_id  = 0;
        auto serial_num_id = 1;
        auto region_id     = 2;

        if( (std::max({tray_type_id, serial_num_id, region_id}) + 1) > svec.size() ) {
            throw std::runtime_error("rfid parse fail");
        } else {
            RFID res;
            res.tray_type   = svec.at(tray_type_id);
            res.serial_num  = svec.at(serial_num_id);
            res.region_code = svec.at(region_id);
            return res;
        }
    }
};

}