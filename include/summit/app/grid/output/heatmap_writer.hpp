#pragma once
#include <summit/app/grid/reg_mat_mk_index.hpp>
#include <summit/app/grid/output/cell_info.hpp>
#include <summit/app/grid/output/html_table_writer.hpp>
#include <summit/app/grid/output/tsv_writer.hpp>
#include <summit/app/grid/output/cell_info_writer.hpp>
#include <summit/app/grid/output/data_paths.hpp>
#include <summit/app/grid/output/filter.hpp>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <fstream>
#include <map>
#include <string>
#include <memory>
namespace summit::app::grid::output {

// using FLOAT = float;
// using GLID = int;
template<class FLOAT, class GLID>
struct HeatmapWriter {
    using CellInfoWriterType = CellInfoWriter<FLOAT, GLID>;
    HeatmapWriter(
        const output::DataPaths&              dp, 
        const std::vector<std::string>&       ofs
    )
    : data_paths(dp)
    , output_formats_(ofs)
    {}
private:
    auto create_output_writer(const std::string& format, std::ostream& out) {
        std::unique_ptr<output::CellInfoWriter<FLOAT, GLID>> res(nullptr);
        if( format == "tsv" ) {
            res.reset(new output::TsvWriter<FLOAT, GLID>(out, "\t"));
        } else if ( format == "csv" ) {
            res.reset(new output::TsvWriter<FLOAT, GLID>(out, ","));
        } else if ( format == "html" ) {
            res.reset(new output::HtmlTableWriter<FLOAT, GLID>(out));
        }
        return res;

    }
    void raw_image_norm( 
        chipimgproc::MultiTiledMat<
            FLOAT, GLID
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
        all = chipimgproc::viewable(all, 0.02, 0.02);

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
            FLOAT, GLID
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

        RegMatMkIndex mk_index(mat.markers());

        raw_image_norm(mat);
        
        auto filter = output::make_filter<FLOAT, GLID>(filter_type);
        for( auto r = 0; r < mat.rows(); r ++ ) {
            for( auto c = 0; c < mat.cols(); c ++ ) {
                auto full_cellinfo = mat.at(r, c, mat.min_cv_all_data());
                cv::Rect mk_reg;
                int mk_id_x;
                int mk_id_y;
                bool is_marker = mk_index.search(c, r, mk_reg, mk_id_x, mk_id_y);
                output::CellInfo<FLOAT, GLID> o_cell_info(
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
        chipimgproc::MultiTiledMat<FLOAT, GLID>& mat 
    ) {
        for( auto&& ofm : output_formats_ ) {
            auto heatmap_opath = data_paths.heatmap(
                output, task_id, ch, ofm
            );
            heatmap_opath = heatmap_opath.make_preferred();
            auto itr = fid_map_.find(heatmap_opath.string());
            if( itr == fid_map_.end() ) {
                auto* pfile = new std::ofstream(heatmap_opath.string());
                fid_map_[heatmap_opath.string()].reset(pfile);
                writer_map_[heatmap_opath.string()] = create_output_writer(
                    ofm, *pfile
                );
            }
            std::cout << "heatmap output: " << heatmap_opath << std::endl;
            auto& writer = writer_map_[heatmap_opath.string()];
            write_output(*writer, task_id, mat, filter);
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
    std::map<
        std::string, 
        std::unique_ptr<std::ofstream>
    >                                       fid_map_;
    std::map<
        std::string, 
        std::unique_ptr<CellInfoWriterType>
    >                                       writer_map_;
    const output::DataPaths&                data_paths;
    std::vector<std::string>                output_formats_;
};


}