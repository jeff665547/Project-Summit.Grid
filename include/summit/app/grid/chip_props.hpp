#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <CFU/format/chip_sample/array.hpp>
#include <CFU/format/cen/file.hpp>
#include <summit/config/cell_fov.hpp>
#include <summit/config/chip.hpp>
#include "output/format_decoder.hpp"
#include "utils.hpp"
namespace summit::app::grid {

struct ChipProps {
    void set_chiplog(const nlohmann::json& chip_log) {
        chip_type_name_ = chip_log["chip"]["name"].get<std::string>();
        std::cout << "load chip type: " << chip_type_name_ << std::endl;

        cell_fov_       = &(summit::config::cell_fov()
            .get_fov_type(chip_type_name_));

        chip_spec_      = &(summit::config::chip()
            .get_spec((*cell_fov_)["spec"].get<std::string>()));

        fov_cols_ = chip_log["chip"]["fov"]["cols"].get<int>();
        fov_rows_ = chip_log["chip"]["fov"]["rows"].get<int>();
        is_img_enc_ = is_image_encrypted(chip_log);

    }
    const nlohmann::json&   cell_fov()          const   { return *cell_fov_;        }
    const nlohmann::json&   chip_spec()         const   { return *chip_spec_;       }
    const bool&             is_image_enc()      const   { return is_img_enc_;       }
    const std::string&      chip_type_name()    const   { return chip_type_name_;   }
    const int&              fov_cols()          const   { return fov_cols_;         }
    const int&              fov_rows()          const   { return fov_rows_;         }
    const float&            um2px_r()           const   { return um2px_r_;          }
    bool                    rot_est_done()      const   { return (bool)rot_degree_; }
    bool                    um2px_r_done()      const   { return um2px_r_ > 0;      }
    const auto&             rot_degree()        const   { return rot_degree_;       }
    const auto&             fov_mk_regs()       const   { return fov_mk_regs_;      }

    void set_rot_est_result( float degree ) {
        rot_degree_ = degree;
    }
    void set_um2px_r(float um2px_r) {
        um2px_r_ = um2px_r;
    }
    void set_fov_mk_regs(
        Utils::FOVMarkerRegionMap&& _fov_mk_regs
    ) {
        fov_mk_regs_ = std::move(_fov_mk_regs);
    }
private:
    static bool is_image_encrypted(
        const nlohmann::json&               chip_log
    ) {
        auto is_img_enc_itr = chip_log.find("img_encrypted");
        bool is_img_enc = false;
        if(is_img_enc_itr != chip_log.end()) {
            is_img_enc = is_img_enc_itr->get<bool>();
        }
        return is_img_enc;
    }
    const nlohmann::json*                               cell_fov_                   ;
    const nlohmann::json*                               chip_spec_                  ;
    bool                                                is_img_enc_                 ;
    std::string                                         chip_type_name_             ;
    int                                                 fov_cols_                   ;
    int                                                 fov_rows_                   ;
    std::optional<float>                                rot_degree_                 ;
    float                                               um2px_r_           { -1  }  ;
    Utils::FOVMarkerRegionMap                           fov_mk_regs_                ;
};

}