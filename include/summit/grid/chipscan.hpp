#pragma once
#include <boost/filesystem.hpp>
#include <summit/config/chipinfo.hpp>
#include <ChipImgProc/comb/single_general.hpp>
#include <summit/grid/utils.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <summit/grid/image_qc_fail.hpp>
#include <summit/grid/empty_chip_type.hpp>
#include <summit/grid/unknown_chip_log.hpp>
#include <summit/grid/channel_not_found.hpp>
#include <summit/grid/output/cell_info.hpp>
#include <summit/grid/output/html_table_writer.hpp>
#include <summit/grid/output/tsv_writer.hpp>
#include <summit/grid/output/filter.hpp>
#include <summit/grid/output/data_paths.hpp>
#include <summit/grid/output/heatmap_writer.hpp>
#include <summit/grid/task.hpp>
#include <ChipImgProc/stitch/gridline_based.hpp>

namespace summit{ namespace grid{

struct ChipScan {
    using Float         = float         ;
    using GridLineID    = std::uint16_t ;
    auto read_imgs(
        const boost::filesystem::path&  src_path,
        int rows, int cols,
        const std::string& posfix
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
                    << posfix << ".tiff"
                ;
                auto img_path = src_path / ss.str();
                if(!boost::filesystem::exists(img_path)) throw ChannelNotFound();
                std::cout << "read image: " << img_path << std::endl;
                cv::Mat_<std::uint16_t> img = cv::imread(
                    img_path.string(), 
                    cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH
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
        //     chipinfo
        // );

    }
    chipimgproc::MultiTiledMat<Float, GridLineID> cen_chipscan(
        const nlohmann::json&               chip_log,
        const boost::filesystem::path&      src_path,
        const std::string&                  channel_name,
        float                               um2px_r,
        int                                 debug
    ) {
        auto chip_type = chip_log["chip"]["name"].get<std::string>();
        std::cout << "load chipinfo, type: " << chip_type << std::endl;
        auto& chipinfo  = summit::config::chipinfo().get_chip_type(
            chip_type
        );
        std::cout << chipinfo.dump(2) << std::endl;

        auto& fov       = chipinfo["fov"];
        auto fov_rows   = fov["rows"];
        auto fov_cols   = fov["cols"];
        auto imgs = read_imgs(
            src_path, fov_rows, fov_cols, channel_name
        );
        auto um2px_r_itr = chip_log.find("um_to_px_coef");
        if(um2px_r_itr == chip_log.end()) {
            std::cout << "um_to_px_coef not found in chip log" << std::endl;
            if( um2px_r < 0) {
                std::cout << "um2px_r parameter not specified, try auto detection" << std::endl;
                auto& [img_path, img] = imgs.at(cv::Point(0,0));
                um2px_r = Utils::guess_um2px_r(
                    static_cast<cv::Mat_<std::uint16_t>>(img),
                    chipinfo
                );
            }
        } else {
            std::cout << "um_to_px_coef found" << std::endl;
            um2px_r = um2px_r_itr->get<float>();
        }
        std::cout << "um_to_px_coef=" << um2px_r << std::endl;

        chipimgproc::comb::SingleGeneral<Float, GridLineID> algo;
        algo.set_logger(std::cout);
        auto mk_layouts = Utils::generate_sgl_pat_reg_mat_marker_layout(
            um2px_r, chipinfo
        );
        auto st_points = Utils::generate_stitch_points(chipinfo);

        std::vector<chipimgproc::TiledMat<GridLineID>>  mats;
        std::vector<chipimgproc::stat::Mats<FLOAT>>     stats;
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
    void operator()( 
        const boost::filesystem::path&   src_path       ,
        const std::string&               chip_type      ,
        const std::vector<std::string>&  channel_names  ,
        const std::vector<std::string>&  spectrum_names ,
        float                            um2px_r        ,
        const std::string&               output         ,
        const std::vector<std::string>&  output_formats ,
        const std::string&               task_id        ,
        const std::string&               filter         ,
        int                              debug          ,
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
        if( chip_log.find("spectrum") != chip_log.end() ) {
            if( chip_type.empty() ) {
                throw EmptyChipType();
            }
            tirc_chipscan(
                chip_log,  
                src_path, 
                chip_type, 
                spectrum_names.at(0)
            );
        } else if ( channels_itr != chip_log.end() ) {
            std::cout << "CentrillionTech chip log detected" << std::endl;
            auto fov_cols = chip_log["chip"]["fov"]["cols"].get<int>();
            auto fov_rows = chip_log["chip"]["fov"]["rows"].get<int>();
            auto& channels = *channels_itr;
            for( auto& ch : channels ) {
                std::string ch_name = ch["name"];
                if( ch["filter"].get<int>() != 0 ) {
                    auto multi_tiled_mat = cen_chipscan(
                        chip_log, src_path,
                        ch_name,
                        um2px_r, debug
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
                }
                else {
                    std::cout << "channel name: " << ch_name << "is white LED, pass the scan" << std::endl;
                }
            }
        } else {
            std::cout << "unknown chip_log format, skip" << std::endl;
            throw UnknownChipLog();
        }


    }
private:
    chipimgproc::stitch::GridlineBased image_stitcher_;
};
}}