#pragma once
#include <map>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <ChipImgProc/aruco.hpp>
namespace summit::config {

struct ArucoDB : public nlohmann::json {
  public:
    const chipimgproc::aruco::Dictionary& get_dictionary(const std::string& db_key) {
        
        // check db_key
        auto& json = static_cast<nlohmann::json&>(*this);
        if (json.count(db_key) == 0)
            throw std::invalid_argument(db_key + " not found");

        // fetch/load dictionary
        auto it = dicts_.find(db_key);
        if (it == dicts_.end()) {
            auto&& config = json[db_key];
            chipimgproc::aruco::Dictionary dict(
                config["coding_bits"].get<std::int32_t>()
              , config["maxcor_bits"].get<std::int32_t>()
            );
            dict.resize(config["bitmap_list"].size());
            auto&& list = config["bitmap_list"];
            for (auto item = list.begin(); item != list.end(); ++item) {
                auto&& bitmap = item.value();
                uint64_t code = 0; // store with litten-endian
                for (auto offset = 0; offset != bitmap.size(); ++offset)
                    code |= bitmap[offset].get<uint64_t>() << offset;
                dict[std::stoi(item.key())] = code;
            }
            bool unused;
            std::tie(it, unused) = dicts_.emplace(db_key, std::move(dict));
        }
        return it->second;
    }

  private:
    std::map<std::string, chipimgproc::aruco::Dictionary> dicts_;
};

void reload_arucodb(void);
ArucoDB& arucodb(void);

} // namespace config