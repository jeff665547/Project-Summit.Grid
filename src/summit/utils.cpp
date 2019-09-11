#include <summit/utils.h>
#include <Nucleona/sys/executable_dir.hpp>
#include <boost/filesystem.hpp>
#include <range/v3/all.hpp>
#include <algorithm>
namespace summit{

boost::filesystem::path install_path() {
    return nucleona::sys::get_executable_dir() / "..";
}
boost::filesystem::path path_check( const boost::filesystem::path& p ) {
    if( p.is_absolute() ) {
        return p;
    } else {
        return install_path() / p;
    }
}
boost::filesystem::path path_check( const std::string& p ){
    return path_check(boost::filesystem::path(p));
}
std::vector<std::string> to_str_vector( const nlohmann::json& j ) {
    std::vector<std::string> res;
    for( auto& jstr : j ) {
        res.push_back(jstr.get<std::string>());
    }
    return res;
}

cv::Mat better_viewable16(const cv::Mat& mat) {
    if(mat.depth() != CV_16U) {
        throw std::runtime_error("assert fail, better_viewable16 expect input is 16bit image");
    }
    return chipimgproc::viewable(mat);
}

}