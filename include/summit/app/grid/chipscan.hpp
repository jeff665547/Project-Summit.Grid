#pragma once
#include <boost/filesystem.hpp>
#include <summit/config/chip.hpp>
#include <ChipImgProc/comb/single_general.hpp>
#include <summit/app/grid/utils.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <summit/app/grid/image_qc_fail.hpp>
#include <summit/app/grid/empty_chip_type.hpp>
#include <summit/app/grid/unknown_chip_log.hpp>
#include <summit/app/grid/channel_not_found.hpp>
#include <summit/app/grid/output/cell_info.hpp>
#include <summit/app/grid/output/html_table_writer.hpp>
#include <summit/app/grid/output/tsv_writer.hpp>
#include <summit/app/grid/output/filter.hpp>
#include <summit/app/grid/output/data_paths.hpp>
#include <summit/app/grid/output/heatmap_writer.hpp>
#include <summit/app/grid/task.hpp>
#include <ChipImgProc/stitch/gridline_based.hpp>
#include <summit/format/cfu_array.hpp>
#include <CFU/format/cen/file.hpp>
#include <summit/app/grid/output/format_decoder.hpp>
namespace summit::app::grid{

struct ChipScan {
    using Float         = float         ;
    using GridLineID    = std::uint16_t ;
    auto read_imgs(
        const boost::filesystem::path&  src_path,
        int rows, int cols,
        const std::string& posfix,
        bool img_enc
    ) {
        std::map<
            cv::Point, // fov id 
            std::tuple<
                std::string, // path
                cv::Mat_<std::uint16_t> // image
            >,
            chipimgproc::PointLess
        > res;
        for ( int r = 0; r < rows; r ++ ) {
            for ( int c = 0; c < cols; c ++ ) {
                std::stringstream ss;
                ss  << std::to_string(r) << '-' 
                    << std::to_string(c) << '-'
                    << posfix
                ;
                auto img_path = src_path / ss.str();
                std::cout << "read image: " << img_path << std::endl;
                cv::Mat_<std::uint16_t> img = Utils::imread(
                    img_path.string(), img_enc
                );
                chipimgproc::info(std::cout, img);
                res[cv::Point(c, r)] = nucleona::make_tuple(
                    img_path.string(), std::move(img)
                );
            }
        }
        return res;
    }
    chipimgproc::MultiTiledMat<Float, GridLineID> tirc_chipscan(
        const nlohmann::json&           chip_log,
        const boost::filesystem::path&  src_path,
        const std::string&              chip_type,
        const std::string& spectrum_names
    ) {
        throw std::runtime_error("TIRC scanning result process currently not support");
        // auto& shooting_info = chip_log["shooting_info"];
        // auto rows = shooting_info["rows"].get<int>();
        // auto cols = shooting_info["cols"].get<int>();
        // auto imgs = read_imgs(src_path, rows, cols, spectrum_names);

        // auto um2px_r = Utils::guess_um2px_r(
        //     static_cast<cv::Mat_<std::uint16_t>>(),
        //     chip_spec
        // );

    }
    chipimgproc::MultiTiledMat<Float, GridLineID> cen_chipscan(
        const nlohmann::json&               chip_log,
        const boost::filesystem::path&      src_path,
        const nlohmann::json&               channel,
        float                               um2px_r,
        const std::string&                  chip_type,
        const nlohmann::json&               cell_fov,
        const nlohmann::json&               chip_spec,
        int                                 debug,
        bool                                no_bgp
    ) {
        std::cout << cell_fov.dump(2) << std::endl;
        auto& channel_name = channel["name"];

        auto& fov           = cell_fov["fov"];
        auto fov_rows       = fov["rows"];
        auto fov_cols       = fov["cols"];
        auto is_img_enc_itr = chip_log.find("img_encrypted");
        bool is_img_enc = false;
        if(is_img_enc_itr != chip_log.end()) {
            is_img_enc = is_img_enc_itr->get<bool>();
        }
        auto imgs = read_imgs(
            src_path, fov_rows, fov_cols, channel_name, is_img_enc
        );
        auto um2px_r_itr = chip_log.find("um_to_px_coef");
        if(um2px_r_itr == chip_log.end()) {
            std::cout << "um_to_px_coef not found in chip log" << std::endl;
            if( um2px_r < 0) {
                std::cout << "um2px_r parameter not specified, try auto detection" << std::endl;
                auto& [img_path, img] = imgs.at(cv::Point(0,0));
                um2px_r = Utils::guess_um2px_r(
                    static_cast<cv::Mat_<std::uint16_t>>(img),
                    chip_spec
                );
            }
        } else {
            std::cout << "um_to_px_coef found" << std::endl;
            um2px_r = um2px_r_itr->get<float>();
        }
        std::cout << "um_to_px_coef=" << um2px_r << std::endl;

        chipimgproc::comb::SingleGeneral<Float, GridLineID> algo;
        algo.set_margin_method("auto_min_cv");
        algo.set_logger(std::cout);
        algo.disable_background_fix(no_bgp);
        auto mk_layouts = Utils::generate_sgl_pat_reg_mat_marker_layout(
            um2px_r, chip_spec, cell_fov, channel
        );
        auto st_points = Utils::generate_stitch_points(cell_fov);

        std::vector<chipimgproc::TiledMat<GridLineID>>  mats;
        std::vector<chipimgproc::stat::Mats<Float>>     stats;
        std::vector<cv::Point>                          cell_st_pts;
        std::vector<cv::Point>                          fov_ids;
        int i = 0;
        for(auto& [fov_id, mkly] : mk_layouts) {
            if( 1 == debug ) {
                algo.set_rot_cali_viewer([i](const auto& img){
                    cv::imwrite("rot_cali_res" + std::to_string(i) + ".tiff", img);
                });
                algo.set_grid_res_viewer([i](const auto& img){
                    cv::imwrite("grid_res" + std::to_string(i) + ".tiff", img);
                });
                algo.set_margin_res_viewer([i](const auto& img){
                    cv::imwrite("margin_res" + std::to_string(i) + ".tiff", img);
                });
                algo.set_marker_seg_viewer([i](const auto& img){
                    cv::imwrite("marker_seg" + std::to_string(i) + ".tiff", img);
                });
            }
            algo.set_marker_layout(mkly);
            auto& [img_path, img] = imgs[fov_id];
            auto [qc, tiled_mat, stat_mats, theta]
                = algo(img, img_path)
            ;
            mats.emplace_back(std::move(tiled_mat));
            stats.emplace_back(std::move(stat_mats));
            cell_st_pts.emplace_back(std::move(st_points[fov_id]));
            fov_ids.push_back(fov_id);
            i++;
        }
        chipimgproc::MultiTiledMat<Float, GridLineID> res(
            mats, stats, cell_st_pts, fov_ids
        );
        return res;
        
    }
    auto create_array( 
        const output::FormatDecoder& fmt_decoder,
        const nlohmann::json& chip_spec
    ) {
        std::unique_ptr<cfu::format::chip_sample::Array> array(nullptr);
        if( fmt_decoder.enable_array() ) {
            array.reset(new cfu::format::chip_sample::Array(
                summit::format::init_array(chip_spec)
            ));
        }
        return array;
    }
    auto create_cenfile(
        const output::FormatDecoder& fmt_decoder,
        const std::string& cen_path 
    ) {
        std::unique_ptr<cfu::format::cen::File> file(nullptr);
        if( fmt_decoder.enable_cen() ) {
            file.reset(new cfu::format::cen::File(cen_path, H5F_ACC_TRUNC));
        } 
        return file;
    }
    void operator()( 
        const boost::filesystem::path&   src_path       ,
        const std::string&               arg_chip_type  ,
        const std::vector<std::string>&  channel_names  ,
        const std::vector<std::string>&  spectrum_names ,
        float                            um2px_r        ,
        const std::string&               output         ,
        const output::FormatDecoder&     fmt_decoder    ,
        const std::string&               task_id        ,
        const std::string&               filter         ,
        int                              debug          ,
        bool                             no_bgp         ,
        const output::DataPaths&         output_paths   ,
        output::HeatmapWriter<
            Float, GridLineID
        >&                               heatmap_writer
    ) {
        std::cout << "chipscan images procss" << std::endl;
        std::cout << "src path: " << src_path << std::endl;
        std::cout << "load chip_log.json" << std::endl;
        auto chip_log_path = src_path / "chip_log.json";
        std::ifstream chip_log_fin(chip_log_path.string());
        nlohmann::json chip_log;
        chip_log << chip_log_fin;
        std::cout << chip_log.dump(2) << std::endl;
        auto channels_itr = chip_log.find("channels");
        // auto array = summit::format::init_array(summit::config::);
        if( chip_log.find("spectrum") != chip_log.end() ) {
            if( arg_chip_type.empty() ) {
                throw EmptyChipType();
            }
            tirc_chipscan(
                chip_log,  
                src_path, 
                arg_chip_type, 
                spectrum_names.at(0)
            );
        } else if ( channels_itr != chip_log.end() ) {
            std::cout << "CentrillionTech chip log detected" << std::endl;
            auto log_chip_type = chip_log["chip"]["name"].get<std::string>();
            std::cout << "load chip type: " << log_chip_type << std::endl;
            auto& cell_fov  = summit::config::cell_fov()
                .get_fov_type(log_chip_type);
            auto& chip_spec = summit::config::chip()
                .get_spec(cell_fov["spec"].get<std::string>());
            // auto array = summit::format::init_array(chip_spec);
            // cfu::format::cen::File cenfile(output_paths.array_cen(
            //     output, task_id
            // ).string(), H5F_ACC_TRUNC);
            auto array = create_array(fmt_decoder, chip_spec);
            auto cenfile = create_cenfile( 
                fmt_decoder, 
                output_paths.array_cen(output, task_id)
                    .string()
            );
            auto fov_cols = chip_log["chip"]["fov"]["cols"].get<int>();
            auto fov_rows = chip_log["chip"]["fov"]["rows"].get<int>();
            auto& channels = *channels_itr;
            for( auto& ch : channels ) {
                std::string ch_name = ch["name"];
                if( ch["filter"].get<int>() != 0 ) {
                    auto multi_tiled_mat = cen_chipscan(
                        chip_log, src_path, ch,
                        um2px_r, log_chip_type,
                        cell_fov, chip_spec,
                        debug, no_bgp
                    );

                    // sperate image output
                    for( int r = 0; r < fov_rows; r ++ ) {
                        for( int c = 0; c < fov_cols; c ++ ) {
                            auto fov_path = output_paths.fov_image(
                                output, task_id, "norm", r, c, ch_name
                             );
                            auto& raw_img = multi_tiled_mat.get_fov_img(c, r).mat();
                            std::cout << "FOV output: " << fov_path << std::endl;
                            cv::imwrite(fov_path.string(), chipimgproc::viewable(raw_img, 0.02, 0.02));
                        }
                    }

                    // heatmap
                    heatmap_writer.write(
                        task_id, ch_name, output, 
                        filter, multi_tiled_mat
                    );

                    // stitch image 
                    auto grid_image = image_stitcher_( multi_tiled_mat );
                    auto viewable_stitch_image = chipimgproc::viewable( grid_image.mat(), 0.02, 0.02 );
                    auto stitch_image_path = output_paths.stitch_image(output, task_id, "norm", ch_name);
                    cv::imwrite(stitch_image_path.string(), viewable_stitch_image);

                    // gridline
                    std::ofstream gl_file(output_paths.gridline(output, task_id, ch_name).string());
                    Utils::write_gl(gl_file, grid_image);

                    // cfu array
                    if( fmt_decoder.enable_array() ) {
                        summit::format::push_to_cfu_array(
                            *array, multi_tiled_mat, ch_name
                        );
                    }

                    // complete file
                    auto complete_file_path = output_paths.complete_file(output, task_id);
                    std::ofstream fout(complete_file_path.string(), std::fstream::trunc | std::fstream::out);
                    fout.close();
                }
                else {
                    std::cout << "channel name: " << ch_name << " is white LED, pass the scan" << std::endl;
                }
            }
            if( fmt_decoder.enable_cen() ) {
                cenfile->fill_data(*array);
            }
        } else {
            std::cout << "unknown chip_log format, skip" << std::endl;
            throw UnknownChipLog();
        }


    }
private:
    chipimgproc::stitch::GridlineBased image_stitcher_;
};
}