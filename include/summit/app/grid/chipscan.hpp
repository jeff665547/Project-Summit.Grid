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
#include <summit/format/rfid.hpp>
#include <summit/exception/analysis_skip.hpp>
#include <Nucleona/proftool/timer.hpp>
#include <summit/app/grid/output/sup_improc_data.hpp>
#include <summit/utils.h>
#include "output/single_img_proc_res.hpp"
#include <Nucleona/parallel/asio_pool.hpp>
#include "output/background_writer.hpp"

namespace summit::app::grid{

struct ChipScan {
    using Float         = float         ;
    using GridLineID    = std::uint16_t ;
    auto read_imgs(
        const boost::filesystem::path&  src_path,
        int rows, int cols,
        const std::string& posfix,
        bool img_enc,
        const output::DataPaths& data_paths
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
                    img_path.string(), img_enc, data_paths
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
    bool is_image_encrypted(
        const nlohmann::json&               chip_log
    ) {
        auto is_img_enc_itr = chip_log.find("img_encrypted");
        bool is_img_enc = false;
        if(is_img_enc_itr != chip_log.end()) {
            is_img_enc = is_img_enc_itr->get<bool>();
        }
        return is_img_enc;
    }
    template<class Executor>
    auto cen_chipscan(
        const nlohmann::json&               chip_log,
        const boost::filesystem::path&      src_path,
        const nlohmann::json&               channel,
        float                               um2px_r,
        const std::string&                  chip_type,
        const nlohmann::json&               cell_fov,
        const nlohmann::json&               chip_spec,
        int                                 debug,
        bool                                no_bgp,
        bool                                marker_append,
        const output::DataPaths&            data_paths,
        const std::string&                  output_path,
        const std::string&                  task_id,
        Executor&                           tp
    ) {
        auto timer = nucleona::proftool::make_timer(
            [](auto du) {
                std::cout << "chipscan process time: " << 
                    std::chrono::duration_cast<
                        std::chrono::milliseconds
                    >(du).count() << std::endl;
            }
        );
        std::cout << cell_fov.dump(2) << std::endl;
        auto& channel_name = channel["name"];

        auto& fov           = cell_fov["fov"];
        auto fov_rows       = fov["rows"];
        auto fov_cols       = fov["cols"];
        auto is_img_enc     = is_image_encrypted(chip_log);
        auto imgs = read_imgs(
            src_path, fov_rows, fov_cols, channel_name, is_img_enc, data_paths
        );
        // check RFID if the third code is XXX then do nothing.
        auto rfid = Utils::extract_rfid_from_path(src_path);
        if(rfid.region_code == "XXX") {
            throw summit::exception::AnalysisSkip("The RFID region code is required to be ignored, " + rfid.region_code);
        }

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

        auto mk_layouts = Utils::generate_sgl_pat_reg_mat_marker_layout(
            um2px_r, chip_spec, cell_fov, channel
        );
        auto st_points = Utils::generate_stitch_points(cell_fov);

        std::vector<chipimgproc::TiledMat<GridLineID>>  mats;
        std::vector<chipimgproc::stat::Mats<Float>>     stats;
        std::vector<cv::Point>                          cell_st_pts;
        std::vector<cv::Point>                          fov_ids;
        output::SupImprocData                           sup_improc_data;
        std::vector<
            nucleona::parallel::asio_pool::Future<
                output::SingleImgProcRes
            >
        > fov_procs;
        for(auto& [fov_id, mkly] : mk_layouts) {
            fov_procs.emplace_back(tp.submit([
                fov_id, mkly, &chip_spec, um2px_r, &no_bgp, 
                debug, &imgs, &channel_name, this, &tp,
                &data_paths, &output_path, &task_id, &marker_append
            ](){
                chipimgproc::comb::SingleGeneral<Float, GridLineID> algo;
                algo.set_margin_method("auto_min_cv");
                // algo.set_logger(std::cout);
                algo.disable_background_fix(no_bgp);
                algo.set_marker_layout(mkly); // get first layout
                algo.set_chip_cell_info(
                    chip_spec["cell_h_um"].get<float>(),
                    chip_spec["cell_w_um"].get<float>(),
                    chip_spec["space_um"].get<float>()
                );
                bool own_cali_um2px_r_mux = false;
                if(cali_um2px_r_ < 0) {
                    if(cali_um2px_r_mux_.try_lock()) {
                        own_cali_um2px_r_mux = true;
                        algo.enable_um2px_r_auto_scale(um2px_r);
                    } else {
                        while(cali_um2px_r_ < 0) {
                            tp.poll();
                        }
                        algo.disable_um2px_r_auto_scale(cali_um2px_r_);
                    }
                } else {
                    algo.disable_um2px_r_auto_scale(cali_um2px_r_);
                }
                
                std::string channel_name_str = channel_name.get<std::string>();
                if( 1 == debug ) {
                    std::string fov_id_str = std::to_string(fov_id.x) + "-" + std::to_string(fov_id.y);
                    algo.set_rot_cali_viewer([fov_id_str, channel_name_str](const auto& img){
                        cv::imwrite("rot_cali_res" + fov_id_str + 
                            "-" + channel_name_str + ".tiff", img);
                    });
                    algo.set_grid_res_viewer([fov_id_str, channel_name_str](const auto& img){
                        cv::imwrite("grid_res" + fov_id_str + 
                            "-" + channel_name_str + ".tiff", img);
                    });
                    algo.set_margin_res_viewer([fov_id_str, channel_name_str](const auto& img){
                        cv::imwrite("margin_res" + fov_id_str + 
                            "-" + channel_name_str + ".tiff", img);
                    });
                    algo.set_marker_seg_viewer([fov_id_str, channel_name_str](const auto& img){
                        cv::imwrite("marker_seg" + fov_id_str + 
                            "-" + channel_name_str + ".tiff", img);
                    });
                }
                if(marker_append) {
                    algo.set_marker_seg_append_viewer(
                        [
                            fov_id, channel_name_str, &data_paths, 
                            &output_path, &task_id
                        ](const auto& img){
                            cv::Mat tmp = (img * 8.192) + 8192;
                            auto path = data_paths.marker_append(
                                output_path, task_id, 
                                fov_id.y, fov_id.x, 
                                channel_name_str
                            );
                            cv::imwrite(path.string(), tmp);
                        }
                    );

                }
                auto& [img_path, img] = imgs[fov_id];
                auto [qc, tiled_mat, stat_mats, theta, bg_value]
                    = algo(img, img_path)
                ;
                if(own_cali_um2px_r_mux) {
                    cali_um2px_r_ = algo.get_um2px_r();
                }
                return output::SingleImgProcRes {
                    tiled_mat, stat_mats, bg_value
                };
            }));
            cell_st_pts.emplace_back(std::move(st_points[fov_id]));
            fov_ids.push_back(fov_id);
        }
        for(auto&& [fov_id, fut] : ranges::view::zip(fov_ids, fov_procs)) {
            auto single_img_proc_res = fut.sync();
            mats.emplace_back(std::move(single_img_proc_res.tiled_mat));
            stats.emplace_back(std::move(single_img_proc_res.stat_mats));
            sup_improc_data.backgrounds[fov_id] = Utils::mean(
                single_img_proc_res.bg_means
            );

        }
        chipimgproc::MultiTiledMat<Float, GridLineID> res(
            mats, stats, cell_st_pts, fov_ids
        );
        return nucleona::make_tuple(
            std::move(res), std::move(sup_improc_data)
        );
        
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
    template<class Executor>
    void operator()( 
        const boost::filesystem::path&   src_path           ,
        const std::string&               arg_chip_type      ,
        const std::vector<std::string>&  channel_names      ,
        const std::vector<std::string>&  spectrum_names     ,
        float                            um2px_r            ,
        const std::string&               output             ,
        const output::FormatDecoder&     fmt_decoder        ,
        const std::string&               task_id            ,
        const std::string&               filter             ,
        int                              debug              ,
        bool                             no_bgp             ,
        const output::DataPaths&         output_paths       ,
        bool                             marker_append      ,
        output::HeatmapWriter<
            Float, GridLineID
        >&                               heatmap_writer     ,
        output::BackgroundWriter&        background_writer  ,
        Executor&                        tp
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
            auto array = create_array(fmt_decoder, chip_spec);
            auto cenfile = create_cenfile( 
                fmt_decoder, 
                output_paths.array_cen(output, task_id)
                    .string()
            );
            auto fov_cols = chip_log["chip"]["fov"]["cols"].get<int>();
            auto fov_rows = chip_log["chip"]["fov"]["rows"].get<int>();
            auto& channels = *channels_itr;
            std::vector<
                nucleona::parallel::asio_pool::Future<
                    void
                >
            > ch_procs;
            for( auto& ch : channels ) {
                ch_procs.emplace_back(tp.submit([&, ch](){
                    std::string ch_name = ch["name"];
                    if( ch["filter"].get<int>() != 0 ) {
                        auto [multi_tiled_mat, sup_improc_data] = cen_chipscan(
                            chip_log, src_path, ch,
                            um2px_r, log_chip_type,
                            cell_fov, chip_spec,
                            debug, no_bgp, marker_append, 
                            output_paths, output,
                            task_id, tp
                        );

                        // sperate image output
                        for( int r = 0; r < fov_rows; r ++ ) {
                            for( int c = 0; c < fov_cols; c ++ ) {
                                auto fov_path = output_paths.fov_image(
                                    output, task_id, "norm", r, c, ch_name
                                 );
                                auto& raw_img = multi_tiled_mat.get_fov_img(c, r).mat();
                                std::cout << "FOV output: " << fov_path << std::endl;
                                cv::imwrite(
                                    fov_path.string(), 
                                    summit::better_viewable16(raw_img)
                                );
                            }
                        }

                        // heatmap
                        heatmap_writer.write(
                            task_id, ch_name, output, 
                            filter, multi_tiled_mat
                        );
                        // auto mean_float = multi_tiled_mat.dump();
                        // cv::Mat mean_int;
                        // mean_float.convertTo(mean_int, CV_16U, 1);
                        // auto mean_path = output_paths.heatmap(output, task_id, ch_name, ".tiff");
                        // // cv::imwrite(mean_path.string(), chipimgproc::viewable(mean_int));
                        // cv::imwrite(mean_path.string(), mean_int);


                        // stitch image 
                        auto grid_image = image_stitcher_( multi_tiled_mat );
                        auto viewable_stitch_image = chipimgproc::viewable( grid_image.mat(), 0.02, 0.02 );
                        auto stitch_image_path = output_paths.stitch_image(output, task_id, "norm", ch_name);
                        cv::imwrite(stitch_image_path.string(), viewable_stitch_image);

                        // gridline
                        std::ofstream gl_file(output_paths.gridline(output, task_id, ch_name).string());
                        Utils::write_gl(gl_file, grid_image);

                        // background
                        background_writer.write(task_id, ch_name, output, sup_improc_data);
                        // std::ofstream bgv_file(
                        //     output_paths.background(
                        //         output, task_id, ch_name
                        //     ).string()
                        // );
                        // Utils::write_background(bgv_file, sup_improc_data);

                        // cfu array
                        if( fmt_decoder.enable_array() ) {
                            summit::format::push_to_cfu_array(
                                *array, multi_tiled_mat, ch_name
                            );
                        }

                        // complete file
                        output_paths.create_complete_file(
                            src_path.string(), output, task_id
                        );
                        // auto complete_file_path = output_paths.complete_file(output, task_id);
                        // std::ofstream fout(complete_file_path.string(), std::fstream::trunc | std::fstream::out);
                        // fout.close();
                    }
                    else {
                        std::cout << "channel name: " << ch_name << " is white LED, pass the scan" << std::endl;
                        auto& fov           = cell_fov["fov"];
                        auto fov_rows       = fov["rows"];
                        auto fov_cols       = fov["cols"];
                        auto is_img_enc     = is_image_encrypted(chip_log);
                        auto images = read_imgs(
                            src_path, fov_rows, fov_cols, ch_name, 
                            is_img_enc, output_paths
                        );
                    }
                }));
            }
            for(auto& ch_p : ch_procs) {
                ch_p.sync();
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
    std::mutex cali_um2px_r_mux_;
    float cali_um2px_r_ {-1};
    // std::exception_ptr
};
}