/**
 * @file task.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::model::Task
 */
#pragma once
#include "macro.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#include <summit/app/grid/task_id.hpp>
#include <summit/app/grid/pixel_format.hpp>
#include <summit/config/cell_fov.hpp>
#include <summit/config/chip.hpp>
#include <summit/utils.h>
#include "marker_base.hpp"
#include <Nucleona/util/remove_const.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <ChipImgProc/warped_mat/rescale_warp_mat.hpp>
#include "type.hpp"
#include <summit/app/grid/utils.hpp>
#include "model.hpp"
#include "scan_mode.hpp"

namespace summit::app::grid::model {
/**
 * @brief Chip level parameter model,
 *    provide and integrate the parameters used 
 *    in the chip level and the level lower than the chip.
 * 
 */
namespace cmw = chipimgproc::warped_mat;
struct Task {
    using This = Task;
    template<class T>
    using ChnMap   = std::map<std::string, T>;
    using MKRegion = chipimgproc::marker::detection::MKRegion;
    void set_model(const Model& _model) {
        model_ = &_model;
    }
    void set_task_id(const TaskID& task_id) {
        id_ = task_id;
        set_chip_dir(id_.path());
    }
    void set_white_channel_imgs(Utils::FOVImages<std::uint8_t>&& wci) {
        white_channel_imgs_ = std::move(wci);
    }
    void set_probe_channel_imgs(Utils::FOVImages<std::uint16_t>&& pci) {
        probe_channel_imgs_ = std::move(pci);
    }
    void set_rot_degree(const std::optional<float>& _rot_degree) {
        rot_degree_ = _rot_degree;
    }
    nlohmann::json& grid_log() { return grid_log_; }
    bool rot_degree_done() const {
        return rot_degree_.has_value();
    }
    void set_fov_mk_regs(Utils::FOVMarkerRegionMap&& _fov_mk_regs) {
        fov_mk_regs_ = std::move(_fov_mk_regs);
    }
    void set_um2px_r(double _um2px_r) {
        um2px_r_ = _um2px_r;
    }
    bool um_to_px_r_done() const {
        return !(um2px_r_ < 0);
    }
    void set_white_ch_proc_failed(bool flag) {
        white_ch_proc_failed_ = flag;
    }
    auto first_probe_channel() {
        for(int i = 0; i < channels_.size(); i ++){
            auto& ch = channels_.at(i);
            if(ch["filter"].get<int>() != 0) {
                auto px_fmt = ch["pixel_format"].get<std::string>();
                ref_ch_theor_max_val_ = PixelFormat::to_theor_max_val(px_fmt);
                return nucleona::make_tuple(std::move(i), ch);
            }
        }
        throw std::runtime_error("the chip log doesn't include any probe channel");
    }
    auto white_channel() {
        for(int i = 0; i < channels_.size(); i ++){
            auto& ch = channels_.at(i);
            if(ch["filter"].get<int>() == 0) {
                auto px_fmt = ch["pixel_format"].get<std::string>();
                ref_ch_theor_max_val_ = PixelFormat::to_theor_max_val(px_fmt);
                return nucleona::make_tuple(std::move(i), ch);
            }
        }
        throw std::runtime_error("the chip log doesn't include any white channel");
    }
    auto get_marker_patterns_by_marker_type(const std::string& marker_type) const {
        return marker_patterns_->get_by_marker_type(marker_type);
    }
    template<class T>
    auto get_marker_patterns(const std::string& query_key, T value) const {
        return marker_patterns_->get(query_key, value);
    }
    const cv::Point& get_fov_marker_num(int r, int c) {
        return fov_marker_num_.at(cv::Point(c, r));
    }
    double mk_wd_um() const {
        return mk_wd_cl_ * (cell_w_um_ + space_um_);
    }
    double mk_hd_um() const {
        return mk_hd_cl_ * (cell_h_um_ + space_um_);
    }
    double mk_w_um() const {
        return mk_w_cl_ * (cell_w_um_ + space_um_);
    }
    double mk_h_um() const {
        return mk_h_cl_ * (cell_h_um_ + space_um_);
    }
    double mk_wd_px() const {
        return mk_wd_um() * um2px_r_;
    }
    double mk_hd_px() const {
        return mk_hd_um() * um2px_r_;
    }
    double mk_w_px() const {
        return mk_w_um() * um2px_r_;
    }
    double mk_h_px() const {
        return mk_h_um() * um2px_r_;
    }
    double mk_wd_px(double um2px_r) const {
        return mk_wd_um() * um2px_r;
    }
    double mk_hd_px(double um2px_r) const {
        return mk_hd_um() * um2px_r;
    }
    double mk_w_px(double um2px_r) const {
        return mk_w_um() * um2px_r;
    }
    double mk_h_px(double um2px_r) const {
        return mk_h_um() * um2px_r;
    }
    auto grid_log_path() const {
        return model_->task_grid_log(id_).string();
    }
    auto channel_grid_log_path() const {
        return [this](const auto& chname) {
            return model_->channel_grid_log(id_, chname).string();
        };
    }
    void set_grid_done(bool flag) {
        grid_done_ = flag;
    }
    void summary_channel_log() {
        grid_done_ = true;
        for(auto&& ch : *channel_log_) {
            grid_done_ = grid_done_ && ch.at("grid_done").get<bool>();
        }

        grid_bad_ = false;
        for(auto&& ch : *channel_log_) {
            grid_bad_ = grid_bad_ || ch.at("grid_bad").get<bool>();
        }

        for(auto&& ch : *channel_log_) {
            warn_ = warn_ || ch.at("warning").get<bool>();
        }
    }
    void write_log() {
        grid_log_["proc_time"] = proc_time_;
        grid_log_["grid_done"] = grid_done_;
        std::ofstream fout(grid_log_path());
        fout << grid_log_.dump(2);
        fout << std::flush;
    }
    auto fov_image(
        const std::string& tag,
        int r, int c, const std::string& ch
    ) const {
        return model_->fov_image(id_,tag, r, c, ch);
    }
    auto stitch_image(
        const std::string& tag,
        const std::string& ch 
    ) const {
        return model_->stitch_image(id_, tag, ch);
    }
    auto gridline(const std::string& ch_name) const {
        return model_->gridline(id_, ch_name);
    }
    auto stitch_gridline(
        const std::string& tag,
        const std::string& ch_name
    ) const {
        return model_->stitch_gridline(id_, tag, ch_name);
    }
    void set_proc_time(float time) {
        proc_time_ = time;
    }
    auto& channel_in_grid_log(int ch_i) const {
        return model_->in_grid_log().at("channels").at(ch_i);
    }
    void create_complete_file() const {
        model_->create_complete_file(id_);
    }
    void create_warning_file() const {
        model_->create_warning_file(id_);
    }
    void copy_chip_log() const {
        std::ofstream grid_cl(model_->grid_chip_log(id_).string());
        grid_cl << chip_log_;
        grid_cl.close();
        if(!model_->secure_output_enabled()) return ;
        std::ofstream fout(model_->sc_chip_log().string());
        fout << chip_log_;
        fout.close();
    }
    bool support_aruco() const {
        return origin_infer_algo_ == "aruco_detection";
    }
    void collect_fovs_warnings(Utils::FOVMap<bool>& fovs_warnings, bool& warn) {
        bool flag = false;
        for(auto&& [fov_id, warning] : fovs_warnings) {
            flag = flag || warning;
        }
        warn = warn || flag;
    }
    void collect_fovs_mk_append(Utils::FOVMap<cv::Mat>& fov_mk_append) {
        auto wh_mk_append_mat = Utils::make_fovs_mk_append(
            fov_mk_append, 
            fov_rows(), fov_cols(),
            [](auto&& mat){ return mat; }
        );
        auto path = model().marker_append_path(
            id_, ""
        );
        cv::imwrite(path.string(), wh_mk_append_mat);
    }
    void check_fovs_mk_append(
        Utils::FOVMap<cv::Mat>& fov_mk_append_dn, 
        const double& thresh,
        bool& warn
        ) const {
        auto& bm_mk_a_dn = fov_mk_append_dn.at(cv::Point(fov_rows()/2, fov_cols()/2));
        auto  bads       = Utils::count_bad_fov_mk_append(
            fov_mk_append_dn, bm_mk_a_dn,
            fov_rows(),       fov_cols(),
            thresh
        );
        warn = warn || bads;
        // if(bads){
        //     create_warning_file();
        // }
    }
    boost::filesystem::path debug_img(
        const std::string& ch_name, 
        int r, int c, 
        const std::string& tag
    ) const {
        return model().debug_img(id(), ch_name, r, c, tag);
    }
    template<class Str>
    std::function<void(const cv::Mat&)> debug_img_view(
        Str&& ch_name, 
        int r, int c, 
        const std::string& tag, 
        bool viewable = false
    ) const {
        if(model().debug() >= 5 ) {
            if(viewable) {
                return [
                    chn = FWD(ch_name), 
                    r, c, tag, 
                    this
                ](const cv::Mat& view) {
                    auto path = debug_img(chn, r, c, tag);
                    cv::imwrite(path.string(), chipimgproc::viewable(view));
                };
            } else {
                return [
                    chn = FWD(ch_name), 
                    r, c, tag, 
                    this
                ](const cv::Mat& view) {
                    auto path = debug_img(chn, r, c, tag);
                    cv::imwrite(path.string(), view);
                };
            }
        } else {
            return nullptr;
        }
    }
    auto aruco_ch_mk_seg_view(int r, int c) const {
        return debug_img_view("aruco", r, c, "aruco_mkseg", false);
    }
    void set_multi_tiled_mat(const std::string& chname, MTMat mat) {
        multi_tiled_mat_[chname] = std::move(mat);
    }
    void set_multi_warped_mat(const std::string& chname, MWMat mat) {
        multi_warped_mat_[chname] = std::move(mat);
    }
    void set_stitched_img(const std::string& chname, GLRawImg glraw_img) {
        stitched_img_[chname] = std::move(glraw_img);
    }
    auto debug_stitch(const std::string& tag = "") {
        return model().debug_stitch(id(), tag);
    }
    double rum2px_r() const {
        return um2px_r_ / rescale_;
    }
    auto std2raw_warp() const {
        return cmw::rescale_warp_mat(rum2px_r());
    }
    const std::vector<MKRegion>& mk_regs_cl() const {
        if(mk_regs_cl_.empty()) {
            for(int i = 0; i < mk_row_cl_; i ++) {
                for(int j = 0; j < mk_col_cl_; j ++) {
                    MKRegion mkr;
                    mkr.x_i = j;
                    mkr.y_i = i;
                    mkr.x = (j * mk_wd_cl_) + mk_xi_cl_;
                    mkr.y = (i * mk_hd_cl_) + mk_yi_cl_;
                    mkr.width = mk_w_cl_;
                    mkr.height = mk_h_cl_;
                    const_cast<This*>(this)->mk_regs_cl_.emplace_back(std::move(mkr));
                }
            }
        }
        return mk_regs_cl_;
    }
    VAR_GET(nlohmann::json,                 chip_log            )
    VAR_GET(nlohmann::json,                 grid_log            )
    VAR_GET(std::vector<nlohmann::json>,    channels            )
    VAR_GET(std::vector<nlohmann::json>,    probe_channels      )
    VAR_GET(boost::filesystem::path,        chip_dir            )
    VAR_GET(bool,                           is_img_enc          )
    VAR_GET(double,                         um2px_r             )
    VAR_GET(double,                         ref_ch_theor_max_val)
    VAR_GET(std::string,                    scan_mode           )
    VAR_GET(std::string,                    chip_info_name      )
    VAR_GET(std::string,                    chip_spec_name      )
    VAR_GET(float,                          cell_h_um           )
    VAR_GET(float,                          cell_w_um           )
    VAR_GET(float,                          space_um            )
    VAR_GET(int,                            fov_rows            )
    VAR_GET(int,                            fov_cols            )
    VAR_GET(int,                            fov_w               )
    VAR_GET(int,                            fov_h               )
    VAR_GET(int,                            fov_wd              )
    VAR_GET(int,                            fov_hd              )
    VAR_GET(Utils::FOVImages<std::uint8_t>, white_channel_imgs  )
    VAR_GET(Utils::FOVImages<std::uint16_t>,probe_channel_imgs  )
    VAR_GET(double,                         wh_mk_append_eval   )
    VAR_GET(double,                         pb_mk_append_eval   )
    VAR_GET(std::int32_t,                   pyramid_level       )
    VAR_GET(std::int32_t,                   border_bits         )
    VAR_GET(std::int32_t,                   fringe_bits         )
    VAR_GET(double,                         bit_w               )
    VAR_GET(double,                         margin_size         )
    VAR_GET(boost::filesystem::path,        frame_template      )
    VAR_GET(boost::filesystem::path,        frame_mask          )
    VAR_GET(std::int32_t,                   nms_count           )
    VAR_GET(std::int32_t,                   nms_tolerance       )
    VAR_GET(std::int32_t,                   nms_radius          )
    VAR_GET(double,                         ext_width           )
    VAR_GET(std::int32_t,                   cell_size_px        )
    VAR_GET(std::string,                    db_key              )
    VAR_GET(std::optional<float>,           rot_degree          )
    VAR_GET(Utils::FOVMarkerRegionMap,      fov_mk_regs         )
    VAR_GET(bool,                           white_ch_proc_failed)
    VAR_GET(Utils::FOVMarkerNum,            fov_marker_num      )
    VAR_GET(std::uint32_t,                  mk_wd_cl            )
    VAR_GET(std::uint32_t,                  mk_hd_cl            )
    VAR_GET(std::uint32_t,                  mk_w_cl             )
    VAR_GET(std::uint32_t,                  mk_h_cl             )
    VAR_GET(std::uint32_t,                  mk_col_cl           )
    VAR_GET(std::uint32_t,                  mk_row_cl           )
    VAR_GET(std::uint32_t,                  mk_xi_cl            )
    VAR_GET(std::uint32_t,                  mk_yi_cl            )
    VAR_GET(std::uint32_t,                  spec_h_cl           )
    VAR_GET(std::uint32_t,                  spec_w_cl           )
    VAR_GET(float,                          proc_time           )
    VAR_GET(bool,                           grid_done           )
    VAR_GET(bool,                           grid_bad            )
    VAR_GET(std::string,                    origin_infer_algo   )
    VAR_GET(summit::app::grid::TaskID,      id                  )

    VAR_GET(double,                         rescale             )

    VAR_GET(std::uint32_t,                  aruco_width         )
    VAR_GET(std::uint32_t,                  tm_outer_width      )
    VAR_GET(std::uint32_t,                  tm_inner_width      )
    VAR_GET(std::uint32_t,                  tm_padding          )
    VAR_GET(std::uint32_t,                  tm_margin           )

    VAR_GET(double,                         bit_ms_wd_rum       )
    VAR_GET(double,                         bit_ms_hd_rum       )
    VAR_GET(double,                         cell_wd_rum         )
    VAR_GET(double,                         cell_hd_rum         )
    VAR_GET(double,                         cell_w_rum          )
    VAR_GET(double,                         cell_h_rum          )
    VAR_GET(double,                         space_rum           )
    VAR_GET(double,                         mk_wd_rum           )
    VAR_GET(double,                         mk_hd_rum           )
    VAR_GET(double,                         mk_w_rum            )
    VAR_GET(double,                         mk_h_rum            )
    VAR_GET(double,                         fov_w_rum           )
    VAR_GET(double,                         fov_h_rum           )
    VAR_GET(double,                         chip_w_rum          )
    VAR_GET(double,                         chip_h_rum          )
    VAR_GET(double,                         xi_rum              )
    VAR_GET(double,                         yi_rum              )
    VAR_GET(double,                         stat_window_size_r  )

    VAR_PTR_GET(Model,                      model               )
    VAR_PTR_GET(nlohmann::json,             chipinfo            )
    VAR_PTR_GET(nlohmann::json,             chipspec            )
    VAR_PTR_GET(nlohmann::json,             fov                 )
    VAR_PTR_GET(nlohmann::json,             aruco_marker        )
    VAR_PTR_GET(nlohmann::json,             id_map              )
    VAR_PTR_GET(nlohmann::json,             origin_infer        )
    VAR_PTR_GET(nlohmann::json,             location_marker     )
    VAR_PTR_GET(nlohmann::json,             shooting_marker     )
    VAR_PTR_GET(nlohmann::json,             sh_mk_pats_cl       )
    VAR_PTR_GET(nlohmann::json,             sh_mk_pos_cl        )
    VAR_PTR_GET(nlohmann::json,             sh_mk_pats          )
    VAR_PTR_GET(ChipSpecMarkerBase,         marker_patterns     )

    VAR_LOCAL_PTR_GET(nlohmann::json,       channel_log         )

    VAR_GET(ChnMap<OptMTMat>,               multi_tiled_mat     )
    VAR_GET(ChnMap<GLRawImg>,               stitched_img        )
    VAR_IO(Utils::FOVMap<bool>,             fov_ref_ch_successes)
    VAR_IO(Utils::FOVMap<cv::Mat>,          white_warp_mat      )
    VAR_IO(Utils::FOVMap<cv::Mat>,          ref_ch_warp_mat     )
    VAR_IO(Utils::FOVMap<
        std::vector<cv::Point2d>
    >,                                      fov_wh_mk_pos       )
    VAR_IO(Utils::FOVMap<
        std::vector<cv::Point2d>
    >,                                      fov_ref_ch_mk_pos   )
    VAR_IO(Utils::FOVMap<
        std::vector<cv::Point2d>
    >,                                      fov_mk_pos_spec     )
    VAR_GET(ChnMap<MWMat>,                  multi_warped_mat    )
    VAR_IO(Utils::FOVMap<cv::Point>,        stitched_points_cl  )
    VAR_IO(std::vector<cv::Point>,          stitched_points_rum )
    VAR_IO(std::vector<model::GLID>,        gl_x_rum            )
    VAR_IO(std::vector<model::GLID>,        gl_y_rum            )
    VAR_IO(std::vector<model::Float>,       gl_x_raw            )
    VAR_IO(std::vector<model::Float>,       gl_y_raw            )

    VAR_GET(cv::Size2d,                     basic_cover_size    )
    VAR_IO(double,                          highP_cover_extend_r)
    VAR_IO(double,                          regu_cover_extend_r )
    VAR_IO(double,                          ref_ch_rescue_r     )
    VAR_IO(bool,                            est_bias            )
    VAR_IO(bool,                            est_bias_regulation )
    VAR_IO(bool,                            global_search       )
    VAR_IO(bool,                            ref_from_white_ch   )
    VAR_IO(bool,                            ref_from_probe_ch   )
    VAR_IO(bool,                            warn                )
private:
    std::vector<MKRegion>                   mk_regs_cl_;
    
    void set_chip_dir(const boost::filesystem::path& path) {
        grid_log_["channels"] = nlohmann::json::array();
        channel_log_ = &grid_log_["channels"];
        chip_dir_ = path;
        {
            std::size_t i = 0;
            std::ifstream fin((path / "chip_log.json").string());
            fin >> chip_log_;
            for(auto&& cl : chip_log_["channels"]) {
                channels_.push_back(cl);
                auto filter_id = cl.at("filter");
                if(filter_id != 0) {
                    probe_channels_.push_back(cl);
                    probe_channels_.back()["id"] = i;
                    (*channel_log_)[i] = nlohmann::json::object();
                    i ++;
                }
            }
            // if probe channels has any invalid channel id, all channel assign to sequencial id.
            for(auto&& pch : probe_channels_) {
                if(pch.at("id") < 0) {
                    for(std::size_t i = 0; i < probe_channels_.size(); i++) {
                        auto& _pch = probe_channels_[i];
                        _pch.at("id") = i;
                    }
                    break;
                }
            }
        }

        // load chipinfo
        chipinfo_          = &chip_log_["chip"];
        is_img_enc_        = chip_log_.at("img_encrypted");
        um2px_r_           = chip_log_.value<double>("um_to_px_coef", -1);
        scan_mode_         = chip_log_.value<std::string>("scan_mode", "precise");
        chip_info_name_    = chipinfo_->at("name");
        chip_spec_name_    = chipinfo_->at("spec").at("name");

        // load chipinfo > origin_infer
        origin_infer_      = &chipinfo_->at("origin_infer");
        origin_infer_algo_ = origin_infer_->at("algo");

        // load cell_fov
        fov_               = &summit::config::cell_fov().get_fov_type(chip_info_name_);
        auto& tmp_fov      = fov_->at("fov");
        fov_rows_          = tmp_fov.at("rows");
        fov_cols_          = tmp_fov.at("cols");
        fov_wd_            = tmp_fov.at("w_d");
        fov_hd_            = tmp_fov.at("h_d");
        fov_w_             = tmp_fov.at("w");
        fov_h_             = tmp_fov.at("h");

        // load chipspec
        chipspec_          = &summit::config::chip().get_spec(chip_spec_name_);
        cell_h_um_         = chipspec_->at("cell_w_um");
        cell_w_um_         = chipspec_->at("cell_h_um");
        space_um_          = chipspec_->at("space_um");
        spec_h_cl_         = chipspec_->at("h_cl");
        spec_w_cl_         = chipspec_->at("w_cl");

        // load chipspec > location marker
        location_marker_   = &chipspec_->at("location_marker");
        

        // load chipspec > shooting marker
        shooting_marker_   = &chipspec_->at("shooting_marker");
        
        // load chipspec > shooting marker > mk_pats_cl
        sh_mk_pats_cl_     = &shooting_marker_->at("mk_pats_cl");
        sh_mk_pats_        = &shooting_marker_->at("mk_pats");
        
        // load chipspec > shooting marker > position_cl
        sh_mk_pos_cl_      = &shooting_marker_->at("position_cl");
        mk_wd_cl_          = sh_mk_pos_cl_->at("w_d");
        mk_hd_cl_          = sh_mk_pos_cl_->at("h_d");
        mk_w_cl_           = sh_mk_pos_cl_->at("w");
        mk_h_cl_           = sh_mk_pos_cl_->at("h");
        mk_xi_cl_          = sh_mk_pos_cl_->at("x_i");
        mk_yi_cl_          = sh_mk_pos_cl_->at("y_i");
        mk_col_cl_         = sh_mk_pos_cl_->at("col");
        mk_row_cl_         = sh_mk_pos_cl_->at("row");
        // load chipspec > aruco marker
        if(support_aruco()) {
            pyramid_level_     = origin_infer_->at("pyramid_level");

            // obtain ArUco patterns
            aruco_marker_   = &chipspec_->at("aruco_marker");
            if(auto iter = origin_infer_->find("db_key"); iter == origin_infer_->end()) {
                db_key_ =  aruco_marker_->value("db_key", "");
            } else {
                db_key_ = iter.value();
            }
            id_map_         = &aruco_marker_->at("id_map");
            aruco_width_    = aruco_marker_->at("aruco_width");
            tm_outer_width_ = aruco_marker_->at("outer_width");
            tm_inner_width_ = aruco_marker_->at("inner_width");
            tm_padding_     = aruco_marker_->at("padding");
            tm_margin_      = aruco_marker_->at("margin");
 
            // detection parameters
            nms_count_      = (fov_wd_ / mk_wd_cl_ + 1) * (fov_hd_ / mk_hd_cl_ + 1);
            nms_radius_     = aruco_marker_->at("nms_radius");
            nms_tolerance_  = aruco_marker_->at("nms_tolerance");
            ext_width_      = aruco_marker_->at("ext_width");

            // legacy
            cell_size_px_   = origin_infer_->at("cell_size_px");
            border_bits_    = aruco_marker_->at("border_bits");
            fringe_bits_    = aruco_marker_->at("fringe_bits");
            bit_w_          = aruco_marker_->at("bit_w");
            margin_size_    = aruco_marker_->at("margin_size");
            frame_template_ = (summit::install_path() / aruco_marker_->at("frame_template").get<std::string>()).make_preferred();
            frame_mask_     = (summit::install_path() / aruco_marker_->at("frame_mask").get<std::string>()).make_preferred();
        }

        // chipinfo_->erase("spec");
        stat_window_size_r_ = 0.6;


        // TODO: all use cache
        fov_marker_num_   = Utils::generate_fov_marker_num(
            chipspec(), fov()
        );

        marker_patterns_  = &MarkerBase::get().reg_mat_chip_mks(
            chip_spec_name_, sh_mk_pats_cl(), sh_mk_pats()
        );

        for(auto&& [fov_id, _num] : fov_marker_num_ ) {
            fov_mk_regs_[fov_id] = {};
        }

        rescale_ = 2;

        xi_rum_        = 0;
        yi_rum_        = 0;
        cell_wd_rum_   = (space_um_ + cell_w_um_) * rescale_;
        cell_hd_rum_   = (space_um_ + cell_h_um_) * rescale_;
        cell_w_rum_    = cell_w_um_ * rescale_;
        cell_h_rum_    = cell_h_um_ * rescale_;
        space_rum_     = space_um_ * rescale_;
        bit_ms_wd_rum_ =    6.0    * cell_wd_rum_ + space_rum_;
        bit_ms_hd_rum_ =    6.0    * cell_hd_rum_ + space_rum_;
        mk_wd_rum_     = mk_wd_cl_ * cell_wd_rum_;
        mk_hd_rum_     = mk_hd_cl_ * cell_hd_rum_;
        mk_w_rum_      = mk_w_cl_  * cell_wd_rum_;
        mk_h_rum_      = mk_h_cl_  * cell_hd_rum_;
        fov_w_rum_     = fov_w_    * cell_wd_rum_;
        fov_h_rum_     = fov_h_    * cell_hd_rum_;
        chip_w_rum_    = spec_w_cl_ * cell_wd_rum_;
        chip_h_rum_    = spec_h_cl_ * cell_hd_rum_;

        set_stitched_points_cl(Utils::generate_stitch_points(fov()));

        for(auto [fov_id, st_pts_cl] : stitched_points_cl()) {
            stitched_points_rum_.emplace_back(
                std::round(fov_id.x * (fov_wd() * cell_wd_rum())),
                std::round(fov_id.y * (fov_hd() * cell_hd_rum()))
            );
        }

        gl_x_rum_.resize(spec_w_cl() + 1);
        gl_y_rum_.resize(spec_h_cl() + 1);
        gl_x_raw_.resize(spec_w_cl() + 1);
        gl_y_raw_.resize(spec_h_cl() + 1);
        for(model::GLID i = 0; i <= spec_w_cl(); i ++) {
            gl_x_rum_[i] = i * cell_wd_rum();
            gl_x_raw_[i] = i * cell_wd_rum() * rum2px_r();
        }
        for(model::GLID i = 0; i <= spec_h_cl(); i ++) {
            gl_y_rum_[i] = i * cell_hd_rum();
            gl_y_raw_[i] = i * cell_hd_rum() * rum2px_r();
        }

        // Set the evaluation for the marker append image gridding result.
        wh_mk_append_eval_ = 0.98;
        pb_mk_append_eval_ = 0.94;
        // float thresh     = 0.98;  // for BF images
        // 0.95 for good fluor images
        // 0.85, 0.8 for bad fluo images.

        // Initialize the indicator for the source of the referenced image.
        ref_from_white_ch_ = false;
        ref_from_probe_ch_ = false;

        // Initialize the warning indicator.
        warn_              = false;

        // auto tmp = ScanMode::from_string(scan_mode_);
        // Generate scan mode related parameters for ChipImgProc marker detection estimate_bias.
        set_scan_mode_params(ScanMode::from_string(scan_mode_));
    }
    void set_scan_mode_params(const ScanMode::Modes& mode) {
        double basic_multiplying_factor = 14.3;
        double basic_sd = 7.74;
        basic_cover_size_.width = basic_multiplying_factor * basic_sd;
        basic_cover_size_.height = basic_multiplying_factor * basic_sd;
        switch(mode) {
            case ScanMode::precise:
                est_bias_ = true;
                global_search_ = false;
                est_bias_regulation_ = true;
                regu_cover_extend_r_ = 0.1;
                highP_cover_extend_r_ = 2.3;
                break;
            case ScanMode::regular:
                est_bias_ = true;
                global_search_ = false;
                est_bias_regulation_ = false;
                highP_cover_extend_r_ = 2.3;
                break;
            case ScanMode::quick:
                est_bias_ = true;
                global_search_ = false;
                est_bias_regulation_ = false;
                highP_cover_extend_r_ = 2.3;
                break;
            case ScanMode::unknown:
                est_bias_ = true;
                global_search_ = true;
                est_bias_regulation_ = false;
                break;
            case ScanMode::abandoned:
                est_bias_ = false;
                break;
        }
    }
};
using TaskMap = std::map<
    std::string,    // task id
    Task            // task
>;

}