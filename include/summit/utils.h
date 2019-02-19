#pragma once
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <ChipImgProc/utils.h>
namespace summit{

boost::filesystem::path install_path();
boost::filesystem::path path_check( const boost::filesystem::path& p );
boost::filesystem::path path_check( const std::string& p );
std::vector<std::string> to_str_vector( const nlohmann::json& j);

template<class T>
std::vector<T> to_vector( const nlohmann::json& j ) {
    std::vector<T> res;
    for( auto& jo : j ) {
        res.push_back(jo.get<T>());
    }
    return res;
}
cv::Mat better_viewable16(const cv::Mat& mat);

}