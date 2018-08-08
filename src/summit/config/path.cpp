#include <summit/utils.h>
namespace summit{ namespace config{
boost::filesystem::path private_dir(){
    return summit::install_path() / "etc" / "private";
}
boost::filesystem::path public_dir() {
    return summit::install_path() / "etc" / "public";
}

}}