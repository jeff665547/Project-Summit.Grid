/**
 * @file task_id.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::TaskID
 */
#pragma once
#include <string>
#include <boost/filesystem.hpp>
namespace summit::app::grid {

/**
 * @brief Each chip is a task for Summit.Grid, and Summit.Grid will give it a task ID.
 * 
 * @details Consider the chip location, the directory name is the chip ID and
 *          by the Summit.Daemon specification, 
 *          the parent of chip diretory is RFID string.
 *          The task ID is formatted (RFID)_(chip id).
 */
struct TaskID {
    /**
     * @brief Construct a new Task ID object
     * 
     */
    TaskID() = default;
    /**
     * @brief Construct a new Task ID object.
     * 
     * @param cid The Chip ID.
     * @param rfid The RFID.
     * @param src_path The scanned output directory.
     */
    TaskID(const std::string& cid, const std::string& rfid, const boost::filesystem::path& src_path) 
    : chip_id_(cid)
    , rfid_(rfid)
    , src_path_(src_path)
    {}
    /**
     * @brief Get chip ID.
     * 
     * @return const std::string& Chip ID.
     */
    const std::string& chip_id()    const { return chip_id_; }
    /**
     * @brief Get RFID.
     * 
     * @return const std::string& RFID.
     */
    const std::string& rfid()       const { return rfid_; }
    /**
     * @brief Convert task ID to string.
     * 
     * @return std::string Task ID in string.
     */
    std::string string() const {
        return rfid_ + "_" + chip_id_;
    }
    /**
     * @brief Get the task path
     * 
     * @return boost::filesystem::path The task path
     */
    boost::filesystem::path path() const {
        return src_path_ / rfid_ / chip_id_;
    }
    /**
     * @brief The less than operator.
     * 
     * @param t Compared task ID.
     * @return true Is less than compare target.
     * @return false Is not less than compare target.
     */
    bool operator<(const TaskID& t) const {
        return string() < t.string();
    }
private:
    /**
     * @brief Chip ID
     * 
     */
    std::string chip_id_;
    /**
     * @brief RFID
     * 
     */
    std::string rfid_;
    /**
     * @brief Summit scanner output directory
     * 
     */
    boost::filesystem::path src_path_;
};

}