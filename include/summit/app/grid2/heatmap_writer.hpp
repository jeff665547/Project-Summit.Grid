#pragma once
#include <summit/grid/reg_mat_mk_index.hpp>
#include "heatmap_writer/cell_info.hpp"
#include "heatmap_writer/html_table_writer.hpp"
#include "heatmap_writer/tsv_writer.hpp"
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
namespace summit::app::grid2 {

// using Float = float;
// using GLID = int;
template<class Float, class GLID>
struct HeatmapWriter {
    using CellInfoWriterType = heatmap_writer::CellInfoWriter<Float, GLID>;
    HeatmapWriter(
        const Paths&                          dp, 
        const std::vector<std::string>&       ofs
    )
    : data_paths(dp)
    , output_formats_(ranges::view::transform(ofs, [](auto&& str){
        return OutputFormat::from_string(str);
    }))
    {}
private:
    auto create_output_writer(const OutputFormat::Labels& format, std::ostream& out) {
        std::unique_ptr<CellInfoWriterType> res(nullptr);
        switch(format) {
            case OutputFormat::csv_probe_list:
                res.reset(new heatmap_writer::TsvWriter<Float, GLID>(out, ","));
                break;
            case OutputFormat::tsv_probe_list:
                res.reset(new heatmap_writer::TsvWriter<Float, GLID>(out, "\t"));
                break;
            case OutputFormat::html_probe_list:
                res.reset(new heatmap_writer::HtmlTableWriter<Float, GLID>(out));
                break;
            default:
                throw std::runtime_error("not support output format: " + OutputFormat::to_string(format));

        }
        return res;

    }
    void raw_image_norm( 
        chipimgproc::MultiTiledMat<
            Float, GLID
        >& mat
    ) {
        auto& raw_imgs = mat.mats();
        // int im_r = raw_imgs.at(0).mat().rows;
        // int im_c = raw_imgs.at(0).mat().cols;
        int w = 0; // raw_imgs.size() * im_c;
        int h = 0; // im_r;
        for( auto& img : raw_imgs ) {
            w += img.mat().cols; 
            if( h < img.mat().rows ) h = img.mat().rows;
        }
        cv::Mat all(h, w, raw_imgs.at(0).mat().type());
        std::cout << "all image append info: " << std::endl;
        chipimgproc::info(std::cout, all);
        // int i = 0;
        // for(int j = 0; j < w; j += im_c ) {
        //     cv::Rect roi(
        //         j,
        //         0,
        //         im_c,
        //         im_r
        //     );
        //     std::cout << "copy to append image roi: " << roi << std::endl;
        //     raw_imgs.at(i).mat().copyTo(all(roi));
        //     i++;
        // }
        {
            int x = 0;
            for(auto& img : raw_imgs) {
                cv::Rect roi(
                    x, 0, img.mat().cols, img.mat().rows
                );
                std::cout << "copy to append image roi: " << roi << std::endl;
                img.mat().copyTo(all(roi));
                x += img.mat().cols;
            }
        }
        // all = chipimgproc::viewable(all);

        // i = 0;
        // for(int j = 0; j < w; j += im_c ) {
        //     cv::Rect roi(
        //         j,
        //         0,
        //         im_c,
        //         im_r
        //     );
        //     std::cout << "copy from append image roi: " << roi << std::endl;
        //     all(roi).copyTo(raw_imgs.at(i).mat());
        //     i++;
        // }
        {
            int x = 0;
            for(auto& img : raw_imgs) {
                cv::Rect roi(
                    x, 0, img.mat().cols, img.mat().rows
                );
                std::cout << "copy from append image roi: " << roi << std::endl;
                all(roi).copyTo(img.mat());
                x += img.mat().cols;
            }
        }
    }
    void write_output( 
        CellInfoWriterType&         writer,
        const std::string&          task_id, 
        chipimgproc::MultiTiledMat<
            Float, GLID
        >&                          mat,
        const std::string&          filter_type
    ) {
        // auto mean_dump = mat.dump();
        // std::cout << "mean dump info: " << std::endl;
        // chipimgproc::info(std::cout, mean_dump);
        // cv::Mat_<std::uint16_t> heatmap;
        // mean_dump.convertTo(heatmap, CV_16U, 1);
        // heatmap = chipimgproc::viewable(heatmap, 0.02, 0.02);
        // cv::imwrite(task_id + ".png", heatmap);

        summit::grid::RegMatMkIndex mk_index(mat.markers());

        raw_image_norm(mat);
        
        auto filter = make_filter<Float, GLID>(filter_type);
        for( auto r = 0; r < mat.rows(); r ++ ) {
            for( auto c = 0; c < mat.cols(); c ++ ) {
                auto full_cellinfo = mat.at(r, c, mat.min_cv_all_data());
                cv::Rect mk_reg;
                int mk_id_x;
                int mk_id_y;
                bool is_marker = mk_index.search(c, r, mk_reg, mk_id_x, mk_id_y);
                heatmap_writer::CellInfo<Float, GLID> o_cell_info(
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
    void write(
        const std::string& task_id, 
        const std::string& ch,
        const std::string& output, 
        const std::string& filter,
        chipimgproc::MultiTiledMat<Float, GLID>& mat 
    ) {
        for( auto&& ofm : output_formats_ ) {
            auto heatmap_opath = data_paths.heatmap(
                ch, OutputFormat::to_file_postfix(ofm)
            );
            heatmap_opath = heatmap_opath.make_preferred();
            auto itr = fid_map_.find(heatmap_opath.string());
            if( itr == fid_map_.end() ) {
                std::lock_guard<std::mutex> lock(map_mux_);
                if( itr == fid_map_.end() ) {
                    auto* pfile = new std::ofstream(heatmap_opath.string());
                    fid_map_[heatmap_opath.string()].reset(pfile);
                    writer_map_[heatmap_opath.string()] = create_output_writer(
                        ofm, *pfile
                    );
                    writer_mux_[heatmap_opath.string()].reset(new std::mutex());
                }
            }
            std::cout << "heatmap output: " << heatmap_opath << std::endl;
            auto& writer = writer_map_[heatmap_opath.string()];
            auto& mux = writer_mux_[heatmap_opath.string()];
            {
            std::lock_guard<std::mutex> lock(*mux);
            write_output(*writer, task_id, mat, filter);
            }
        }
        
    }
    void close() {
        for( auto& [task_id, writer] : fid_map_) {
            writer->close();
        }
    }
    ~HeatmapWriter() {
        close();
    }
private:
    std::mutex                              map_mux_            ;
    std::map<
        std::string, 
        std::unique_ptr<std::ofstream>
    >                                       fid_map_            ;
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