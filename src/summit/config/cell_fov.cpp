#include <summit/config/cell_fov.hpp>
#include <summit/config/path.hpp>
namespace summit::config {

struct CellFOVHelper {

friend CellFOV& cell_fov();
friend void reload_cell_fov();

private:
    static std::unique_ptr<CellFOV> p_data_;
};

void reload_cell_fov() {
    CellFOVHelper::p_data_.reset(new CellFOV());
    std::ifstream fin((
        private_dir()/ "cell_fov.json"
    ).string());
    fin >> *CellFOVHelper::p_data_;
}

CellFOV& cell_fov() {
    if(!CellFOVHelper::p_data_) {
        reload_cell_fov();
    }
    return *CellFOVHelper::p_data_;
}

std::unique_ptr<CellFOV> CellFOVHelper::p_data_(nullptr);

};