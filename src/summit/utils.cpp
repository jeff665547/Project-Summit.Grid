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
    const cv::Mat_<std::uint16_t>& mat16 = static_cast<const cv::Mat_<std::uint16_t>&>(mat);
    auto mat8 = chipimgproc::norm_u8(mat16, 0.01, 0);
    cv::Mat_<std::uint8_t> bin;
    cv::threshold(mat8, bin, 150, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    cv::Mat_<std::uint16_t> res = mat16.clone();
    auto max_px_itr = std::max_element(res.begin(), res.end());
    float rate = 40000 / *max_px_itr;
    res.forEach([&bin, &rate](std::uint16_t& px, const int* pos){
        auto& b = bin(pos[0], pos[1]);
        if(b != 0) {
            px = std::round(px*rate);
        }
        
    });
    res = chipimgproc::viewable(res, 0.001, 0.001);
    return res;
}



}