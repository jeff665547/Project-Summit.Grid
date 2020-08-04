/**
 * @file heatmap_writer.hpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief @copybrief summit::app::grid::HeatmapWriter
 * 
 */
#pragma once
#include <summit/grid/reg_mat_mk_index.hpp>
#include "heatmap_writer/cell_info.hpp"
#include "heatmap_writer/html_table_writer.hpp"
#include "heatmap_writer/tsv_writer.hpp"
#include "heatmap_writer/cen_writer.hpp"
#include "heatmap_writer/mat_tiff_writer.hpp"
#include "heatmap_writer/cell_info_writer.hpp"
#include "output_format.hpp"
#include "paths.hpp"
#include "filter.hpp"
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <fstream>
#include <map>
#include <string>
#include <memory>
#include <Nucleona/range.hpp>
#include <summit/grid/logger.hpp>
#include "model/type.hpp"
namespace summit::app::grid {
namespace model {
    struct Task;
}
/**
 * @brief The Integrated heatmap writer.
 * 
 * @tparam Float The float point type used in HeatmapWriter, 
 *      may effect intensity or other cell info representation.
 * @tparam GLID The grid lines position integer type.
 */
struct HeatmapWriter {
    using CellInfoWriterType = heatmap_writer::CellInfoWriter;
    /**
     * @brief Construct a new Heatmap Writer object
     * 
     * @param dp Current path specification.
     * @param ofs User identified output formats.
     */
    HeatmapWriter(
        const Paths&                                dp, 
        const std::vector<OutputFormat::Labels>&    ofs
    )
    : data_paths(dp)
    , output_formats_(ofs)
    {}
private:
    /**
     * @brief Create the polymorphic output writer.
     * 
     * @param format The output format.
     * @param out The output path.
     * @param task The task parameter model.
     * @return auto The polymorphic output writer.
     */
    auto create_output_writer(const OutputFormat::Labels& format, const std::string& out, const model::Task& task) {
        std::unique_ptr<CellInfoWriterType> res(nullptr);
        switch(format) {
            case OutputFormat::csv_probe_list:
                res.reset(new heatmap_writer::TsvFileWriter(out, ","));
                break;
            case OutputFormat::tsv_probe_list:
                res.reset(new heatmap_writer::TsvFileWriter(out, "\t"));
                break;
            case OutputFormat::html_probe_list:
                res.reset(new heatmap_writer::HtmlTableFileWriter(out));
                break;
            case OutputFormat::cen_file:
                res.reset(new heatmap_writer::CENWriter(out, task));
                break;
            case OutputFormat::mat_tiff:
                res.reset(new heatmap_writer::MatTiffWriter(out));
                break;
            default:
                throw std::runtime_error("not support output format: " + OutputFormat::to_string(format));

        }
        return res;
    }
    /**
     * @deprecated This function is no function any more.
     * @brief Normalize all FOV using same scaling parameters.
     * 
     * @param mat The input multi-tiled matrix.
     */
    // void raw_image_norm( 
    //     chipimgproc::MultiTiledMat<
    //         Float, GLID
    //     >& mat
    // ) {
    //     auto& raw_imgs = mat.mats();
    //     // int im_r = raw_imgs.at(0).mat().rows;
    //     // int im_c = raw_imgs.at(0).mat().cols;
    //     int w = 0; // raw_imgs.size() * im_c;
    //     int h = 0; // im_r;
    //     for( auto& img : raw_imgs ) {
    //         w += img.mat().cols; 
    //         if( h < img.mat().rows ) h = img.mat().rows;
    //     }
    //     cv::Mat all(h, w, raw_imgs.at(0).mat().type());
    //     // std::cout << "all image append info: " << std::endl;
    //     // chipimgproc::info(std::cout, all);
    //     // int i = 0;
    //     // for(int j = 0; j < w; j += im_c ) {
    //     //     cv::Rect roi(
    //     //         j,
    //     //         0,
    //     //         im_c,
    //     //         im_r
    //     //     );
    //     //     std::cout << "copy to append image roi: " << roi << std::endl;
    //     //     raw_imgs.at(i).mat().copyTo(all(roi));
    //     //     i++;
    //     // }
    //     {
    //         int x = 0;
    //         for(auto& img : raw_imgs) {
    //             cv::Rect roi(
    //                 x, 0, img.mat().cols, img.mat().rows
    //             );
    //             // std::cout << "copy to append image roi: " << roi << std::endl;
    //             img.mat().copyTo(all(roi));
    //             x += img.mat().cols;
    //         }
    //     }
    //     // all = chipimgproc::viewable(all);

    //     // i = 0;
    //     // for(int j = 0; j < w; j += im_c ) {
    //     //     cv::Rect roi(
    //     //         j,
    //     //         0,
    //     //         im_c,
    //     //         im_r
    //     //     );
    //     //     std::cout << "copy from append image roi: " << roi << std::endl;
    //     //     all(roi).copyTo(raw_imgs.at(i).mat());
    //     //     i++;
    //     // }
    //     {
    //         int x = 0;
    //         for(auto& img : raw_imgs) {
    //             cv::Rect roi(
    //                 x, 0, img.mat().cols, img.mat().rows
    //             );
    //             // std::cout << "copy from append image roi: " << roi << std::endl;
    //             all(roi).copyTo(img.mat());
    //             x += img.mat().cols;
    //         }
    //     }
    // }
    /**
     * @brief Write multi-tiled matrix to specific format file.
     * 
     * @param writer The heatmap writer.
     * @param task_id The task ID generate by summit::app::grid::TaskID.
     * @param mat The multi-tiled matrix generate from each FOV level image process.
     * @param ch_name The channel name.
     * @param filter_type The filter type, usually "all".
     */
    template<class Task>
    void write_output( 
        const Task&                 task,
        CellInfoWriterType&         writer,
        const std::string&          task_id, 
        const model::MWMat&         mat,
        const std::string&          ch_name,
        const std::string&          filter_type
    ) {

        summit::grid::RegMatMkIndex mk_index(task.mk_regs_cl());

        // raw_image_norm(mat);
        
        auto filter = make_filter(filter_type);
        for( auto r = 0; r < mat.rows(); r ++ ) {
            for( auto c = 0; c < mat.cols(); c ++ ) {
                auto full_cellinfo = mat.at_cell(r, c);
                cv::Rect mk_reg;
                int mk_id_x;
                int mk_id_y;
                bool is_marker = mk_index.search(c, r, mk_reg, mk_id_x, mk_id_y);
                heatmap_writer::CellInfo o_cell_info(
                    r, c, full_cellinfo,
                    is_marker, mk_id_x, mk_id_y,
                    mk_reg
                );
                if( filter(o_cell_info) ) {
                    writer.write(o_cell_info, task_id);
                }
            }
        }
    }
public:
    /**
     * @brief Write multi-tiled matrix into all formats request by client code.
     * 
     * @tparam Task This is a workaround of recursive dependency.
     * 
     * @param task The task parameter model.
     * @param ch The channel name.
     * @param ch_id The channel sequencial ID in chip log.
     * @param filter The filter string, usually "all".
     * @param mat The multi-tiled matrix.
     */
    template<class Task>
    void write(
        const Task& task,
        const std::string& ch,
        int ch_id,
        const std::string& filter,
        const model::MWMat& mat 
    ) {
        auto&& task_id = task.id().string();
        for( auto&& ofm : output_formats_ ) {
            auto heatmap_opath = data_paths.heatmap(
                task_id, ch, ofm
            );
            heatmap_opath = heatmap_opath.make_preferred();
            auto itr = writer_map_.find(heatmap_opath.string());
            if( itr == writer_map_.end() ) {
                std::lock_guard<std::mutex> lock(map_mux_);
                if( itr == writer_map_.end() ) {
                    writer_map_[heatmap_opath.string()] = create_output_writer(
                        ofm, heatmap_opath.string(), task
                    );
                    writer_mux_[heatmap_opath.string()].reset(new std::mutex());
                }
            }
            summit::grid::log.info("heatmap output: {}", heatmap_opath.string());
            auto& writer = writer_map_[heatmap_opath.string()];
            auto& mux = writer_mux_[heatmap_opath.string()];
            {
                std::lock_guard<std::mutex> lock(*mux);
                if(writer->is_write_entire_mat()) {
                    writer->write(mat, task_id, ch, ch_id, filter);
                } else {
                    write_output(task, *writer, task_id, mat, ch, filter);
                }
            }
        }
        
    }
    /**
     * @brief Flush the data to filesystem.
     * 
     */
    void flush() {
        for( auto& [task_id, writer] : writer_map_) {
            writer->flush();
        }
    }
    /**
     * @brief Close the writer.
     * 
     */
    void close() {
        for( auto& [task_id, writer] : writer_map_) {
            writer->close();
        }
    }
    /**
     * @brief Destroy the Heatmap Writer object
     * 
     */
    ~HeatmapWriter() {
        close();
    }
private:
    std::mutex                              map_mux_            ;
    // std::map<
    //     std::string, 
    //     std::unique_ptr<std::ofstream>
    // >                                       fid_map_            ;
    std::map<
        std::string, 
        std::unique_ptr<CellInfoWriterType>
    >                                       writer_map_         ;
    std::map<
        std::string,
        std::unique_ptr<std::mutex>
    >                                       writer_mux_         ;
    const Paths&                            data_paths          ;
    std::vector<OutputFormat::Labels>       output_formats_     ;
};


}