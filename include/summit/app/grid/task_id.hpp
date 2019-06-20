#pragma once
#include <string>
#include <boost/filesystem.hpp>
namespace summit::app::grid {

struct TaskID {
    TaskID() = default;
    TaskID(const std::string& cid, const std::string& rfid, const boost::filesystem::path& src_path) 
    : chip_id_(cid)
    , rfid_(rfid)
    , src_path_(src_path)
    {}
    const std::string& chip_id()    const { return chip_id_; }
    const std::string& rfid()       const { return rfid_; }
    std::string string() const {
        return rfid_ + "_" + chip_id_;
    }
    boost::filesystem::path path() const {
        return src_path_ / rfid_ / chip_id_;
    }
    bool operator<(const TaskID& t) const {
        return string() < t.string();
    }
private:
    std::string chip_id_;
    std::string rfid_;
    boost::filesystem::path src_path_;
};

}