#include <summit/config/chip.hpp>
#include <summit/config/path.hpp>
namespace summit::config {

struct ChipHelper {

friend Chip& chip();
friend void reload_chip();

private:
    static std::unique_ptr<Chip> p_data_;
};

void reload_chip() {
    ChipHelper::p_data_.reset(new Chip());
    std::ifstream fin((
        private_dir()/ "chip.json"
    ).string());
    fin >> *ChipHelper::p_data_;
}

Chip& chip() {
    if(!ChipHelper::p_data_) {
        reload_chip();
    }
    return *ChipHelper::p_data_;
}

std::unique_ptr<Chip> ChipHelper::p_data_(nullptr);

};