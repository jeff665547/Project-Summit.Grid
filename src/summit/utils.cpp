#include <summit/utils.h>
#include <Nucleona/sys/executable_dir.hpp>
#include <boost/filesystem.hpp>
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



}