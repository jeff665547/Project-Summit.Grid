#pragma once
#include <boost/filesystem.hpp>
#include <summit/config/chipinfo.hpp>
#include <ChipImgProc/comb/single_general.hpp>
#include <summit/grid/utils.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <summit/grid/image_qc_fail.hpp>
#include <summit/grid/empty_chip_type.hpp>
#include <summit/grid/output/cell_info.hpp>
#include <summit/grid/output/html_table_writer.hpp>
#include <summit/grid/output/tsv_writer.hpp>
#include <summit/grid/output/filter.hpp>
#include <summit/grid/output/data_paths.hpp>
#include <summit/grid/output/heatmap_writer.hpp>
namespace summit{ namespace grid{

struct Single {
    using Float         = float;
    using GridLineID    = std::uint16_t;
    void operator()( 
        const boost::filesystem::path&  src_path        ,
        const std::string&              chip_type       ,
        const int&                      fov_ec_id       ,
        float                           um2px_r         ,
        const std::string&              output          ,
        const std::vector<std::string>& output_formats  ,
        const std::string&              task_id         ,
        const std::string&              filter          ,
        int                             debug           ,
        const output::DataPaths&        output_paths    ,
        output::HeatmapWriter<
            Float, GridLineID
        >&                              heatmap_writer
    ) {
        std::cout << "single image process" << std::endl;
        std::cout << "read image: " << src_path << std::endl;
        cv::Mat src = cv::imread(src_path.string(), cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
        std::cout << "src info: " << std::endl;
        chipimgproc::info(std::cout, src);
        if( src.type() != CV_16UC1 ) {
            throw std::runtime_error("image should be 16bit gray scale image");
        }
        if( chip_type.empty() ) {
            throw EmptyChipType();
        }
        std::cout << "read chip info: " << chip_type << std::endl;
        auto chipinfo = config::chipinfo().get_chip_type(chip_type);

        if(um2px_r < 0 ) {
            um2px_r = Utils::guess_um2px_r(
                static_cast<cv::Mat_<std::uint16_t>>(src), chipinfo
            );
        }
        std::cout << "um to pixel rate: " << um2px_r << std::endl;

        chipimgproc::comb::SingleGeneral<Float, GridLineID> algo;
        algo.set_logger(std::cout);
        if( 1 == debug ) {
            algo.set_rot_cali_viewer([](const auto& img){
                cv::imwrite("rot_cali_res.tiff", img);
            });
            algo.set_grid_res_viewer([](const auto& img){
                cv::imwrite("grid_res.tiff", img);
            });
            algo.set_margin_res_viewer([](const auto& img){
                cv::imwrite("margin_res.tiff", img);
            });
        }
        algo.set_marker_layout(Utils::generate_sgl_pat_reg_mat_marker_layout(
            um2px_r, chipinfo, fov_ec_id
        )[cv::Point(0,0)]);

        auto [qc, tiled_mat, stat_mats, theta] = algo(src, src_path.string());
        if(!qc) throw ImageQCFail();
        std::vector<chipimgproc::TiledMat<GridLineID>>  mats;
        std::vector<chipimgproc::stat::Mats<FLOAT>>     stats;
        std::vector<cv::Point>                          cell_st_pts;
        mats        .emplace_back(std::move(tiled_mat));
        stats       .emplace_back(std::move(stat_mats));
        cell_st_pts .emplace_back(cv::Point(0,0));
        chipimgproc::MultiTiledMat<Float, GridLineID> multi_tiled_mat(
            std::move(mats          ),
            std::move(stats         ),
            std::move(cell_st_pts   )
        );

        // single image
        auto& raw_grid_img = multi_tiled_mat.mats().at(0);
        auto& raw_img = raw_grid_img.mat();
        cv::imwrite(
            output_paths.single_image(output, task_id, "norm").string(),
            chipimgproc::viewable(raw_img, 0.02, 0.02)
        );
        
        // heatmap
        heatmap_writer.write(task_id, "NO_CH", output, filter, multi_tiled_mat);

        // gridline
        std::ofstream gl_file(output_paths.gridline(output, task_id, "NO_CH").string());
        Utils::write_gl(gl_file, raw_grid_img);
    }
};

}}