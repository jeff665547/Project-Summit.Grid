#include <memory>
#include <summit/config/path.hpp>
#include <summit/config/aruco_db.hpp>
#define ARUCODB_FILEPATH "aruco_db.json"
namespace summit::config {

struct ArucoDBHelper {
  friend void reload_arucodb(void);
  friend ArucoDB& arucodb(void);
  private:
    static std::unique_ptr<ArucoDB> p_data_;
};
std::unique_ptr<ArucoDB> ArucoDBHelper::p_data_(nullptr);

void reload_arucodb(void) {
    ArucoDBHelper::p_data_.reset(new ArucoDB);
    std::ifstream is((private_dir() / ARUCODB_FILEPATH).string());
    is >> *ArucoDBHelper::p_data_;
}
ArucoDB& arucodb(void) {
    if (!ArucoDBHelper::p_data_)
        reload_arucodb();
    return *ArucoDBHelper::p_data_;
}

boost::filesystem::path arucodb_path(void) {
    return private_dir() / ARUCODB_FILEPATH;
}

}