#pragma once
#include <string>
namespace summit::app::grid2 {

struct TaskID {
    TaskID() = default;
    TaskID(const std::string& cid, const std::string& rfid) 
    : chip_id_(cid)
    , rfid_(rfid)
    {}
    const std::string& chip_id()    const { return chip_id_; }
    const std::string& rfid()       const { return rfid_; }
    std::string string() const {
        return rfid_ + "_" + chip_id_;
    }
    bool operator<(const TaskID& t) const {
        return string() < t.string();
    }
private:
    std::string chip_id_;
    std::string rfid_;
};

}