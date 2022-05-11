#pragma once
#include <boost/filesystem.hpp>
#include <summit/config/chip.hpp>
#include <summit/config/cell_fov.hpp>
#include <summit/utils.h>
#include <limits>
#include <ChipImgProc/marker/loader.hpp>
#include <Nucleona/range.hpp>
#include <optional>
#include <iostream>
#include <iomanip>
#include <type_traits>
#include <ChipImgProc/marker/txt_to_img.hpp>
#include <summit/crypto/scan_image.hpp>
#include <ChipImgProc/utils.h>
#include <summit/format/rfid.hpp>
#include <ChipImgProc/marker/detection/mk_region.hpp>
#include "paths.hpp"
#include "sup_improc_data.hpp"
#include <optional>
#include "task_id.hpp"
#include "is_chip_dir.hpp"
#include <opencv2/imgproc/types_c.h> 

namespace summit::app::grid {
struct Utils{
    static auto mean(const std::vector<float>& data) {
        float sum = 0;
        int n = 0;
        for(auto& v : data) {
            if(std::isnan(v)) {
                // std::cout << "warn: a nan value detected, ignored" << std::endl; 
            } else {
                sum += v;
                n ++;
            }
        }
        return sum/n;
    }
    static auto extract_rfid_from_path(
        const boost::filesystem::path& path
    ) {
        auto itr = path.rbegin();
        itr ++;
        auto rfid_str = itr->string();
        auto rfid = summit::format::RFID::parse(rfid_str);
        return rfid;
    }
    template<class T>
    static auto guess_um2px_r( 
        const cv::Mat_<T>& src, 
        const nlohmann::json& chip_spec 
    ) {
        auto u8_src = chipimgproc::norm_u8(src);
        auto& shooting_marker = chip_spec["shooting_marker"];
        auto& shooting_marker_type = shooting_marker["type"];
        // std::cout << "shooting_marker_type: " << shooting_marker_type << std::endl;
        if( shooting_marker_type.get<std::string>() != "regular_matrix" ) {
            throw std::runtime_error(
                "unsupported shooting marker type: " + 
                shooting_marker_type.get<std::string>()
            );
        }
        auto& marker_pats = shooting_marker["mk_pats"];

        float max_score = std::numeric_limits<float>::min();
        float max_score_um2px_r = 0;

        for(auto&& mk : marker_pats ) {
            std::string path = mk["path"];
            auto pat_img_path = (summit::install_path() / path).make_preferred();
            // std::cout << "load pattern: " << pat_img_path << std::endl;
            cv::Mat pat_img = cv::imread(pat_img_path.make_preferred().string(), cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
            // chipimgproc::info(std::cout, pat_img);
            cv::Mat_<float> score(
                u8_src.rows - pat_img.rows + 1,
                u8_src.cols - pat_img.cols + 1
            );
            cv::matchTemplate(u8_src, pat_img, score, CV_TM_CCORR_NORMED);
            double min, max;
            cv::Point min_loc, max_loc;
            cv::minMaxLoc(score, &min, &max, &min_loc, &max_loc);

            if(max_score < max ) {
                max_score_um2px_r = mk["um2px_r"].get<float>();
            }
        }
        return max_score_um2px_r;
    }
    static std::vector<std::string> filter_path(
        const nlohmann::json&       mk_pats_cl,
        const nlohmann::json&       channel
    ) {
        std::map<std::string, std::string> mk_type_to_ch_name {
            {"AM1", "CY3"},
            {"AM3", "CY5"}
        }; // For down compatibility
        std::basic_string<bool> filter(mk_pats_cl.size(), false);
        auto mk_itr = channel.find("marker_type");
        if(
            mk_itr == channel.end()
        ) {
            auto ch_name = channel["name"].get<std::string>(); 
            for(std::size_t i = 0; i < mk_pats_cl.size(); i ++ ) {
                auto&& obj = mk_pats_cl[i];
                filter[i] = (
                    mk_type_to_ch_name[
                        obj["marker_type"].get<std::string>()
                    ] != ch_name 
                );
            }
        } // For down compatibility
        else if(
            auto marker_type = *mk_itr; 
            marker_type != "" || marker_type != "none"
        ) {
            for(std::size_t i = 0; i < mk_pats_cl.size(); i ++ ) {
                auto&& obj = mk_pats_cl[i];
                auto obj_mk_type = obj["marker_type"].get<std::string>();
                // std::cout << VDUMP(marker_type) <<std::endl;
                // std::cout << VDUMP(obj_mk_type) <<std::endl;
                filter[i] = (
                    obj_mk_type 
                    != marker_type
                );
            }
        }
        std::vector<std::string> res;
        for(std::size_t i = 0; i < mk_pats_cl.size(); i ++ ) {
            if( !filter[i] ) {
                res.push_back(mk_pats_cl[i]["path"].get<std::string>());
            }
        }
        return res;
    }

    static auto get_single_marker_pattern( 
        float um2px_r,
        const nlohmann::json& shooting_marker,
        float cell_w_um, 
        float cell_h_um, 
        float space_um,
        const nlohmann::json& channel
    ) 
    {
        std::vector<cv::Mat_<std::uint8_t>> candi_pats_cl;
        std::vector<cv::Mat_<std::uint8_t>> candi_pats_cl_mask;
        std::vector<cv::Mat_<std::uint8_t>> candi_pats_px;
        std::vector<cv::Mat_<std::uint8_t>> candi_pats_px_mask;

        auto& mk_pats_cl        = shooting_marker["mk_pats_cl"];
        // std::cout << VDUMP(mk_pats_cl.size()) << '\n';
        auto mk_pats_cl_path = filter_path(
            mk_pats_cl, channel
        );
        // std::cout << VDUMP(mk_pats_cl_path.size()) << '\n';
        // collect candi_pats_cl;
        for( auto&& mk : mk_pats_cl_path ) {
            auto path = summit::install_path() / mk;
            std::ifstream fin(path.string());
            auto [mk_cl, mask_cl] = chipimgproc::marker::Loader::from_txt(
                fin, nucleona::stream::null_out
            );
            candi_pats_cl.push_back(mk_cl);
            candi_pats_cl_mask.push_back(mask_cl);
        } 

        chipimgproc::marker::TxtToImg txt_to_img;
        for( std::size_t i = 0; i < candi_pats_cl.size(); i ++ ) {
            auto& mk = candi_pats_cl.at(i);
            auto& mask = candi_pats_cl_mask.at(i);
            auto [mk_img, mask_img] = txt_to_img(
                mk, mask,
                cell_h_um * um2px_r, 
                cell_w_um * um2px_r,
                space_um * um2px_r
            );
            candi_pats_px.push_back(mk_img);
            candi_pats_px_mask.push_back(mask_img);
        }

        return nucleona::make_tuple(
            std::move(candi_pats_cl), 
            std::move(candi_pats_px),
            std::move(candi_pats_cl_mask),
            std::move(candi_pats_px_mask)
        );
    }
    struct EndP {
        enum Type {
            start, end
        };
        enum Tag {
            fov, marker
        };
        int     id;
        int     point;
        Type    type;
        Tag     tag;
        bool operator<( const EndP& e ) const {
            if(point == e.point ) return tag < e.tag;
            else return point < e.point;
        }
    };
    static void add_region( 
        std::vector<EndP>& points, 
        int id,
        int ini, 
        int len, 
        typename EndP::Tag tag
    ) {
        points.push_back(
            EndP {
                id,
                ini,
                EndP::start,
                tag
            }
        );
        points.push_back(
            EndP {
                id,
                ini + len,
                EndP::end,
                tag
            }
        );
    }
    static std::vector<int> fov_mk_overlap_detect( 
        const std::vector<EndP>& points
    ) {
        // 
        std::vector<int> res;
        std::set<int> curr_fov;
        int curr_mk_num = 0;
        for(auto&& p : points ) {
            if(p.tag == EndP::fov) {
                if( p.type == EndP::start ) {
                    res.push_back(0);
                    curr_fov.insert(p.id);
                } else if( p.type == EndP::end ) {
                    res[p.id] += curr_mk_num;
                    curr_fov.erase(p.id);
                } else {
                    debug_throw(std::runtime_error("BUG: should never be here"));
                }
            } else if( p.tag == EndP::marker ) {
                if( p.type == EndP::start ) {
                    curr_mk_num ++;
                } else if ( p.type == EndP::end ) {
                    for(auto& id : curr_fov) {
                        res[id] ++;
                    }
                    curr_mk_num --;
                } else {
                    debug_throw(std::runtime_error("BUG: should never be here"));
                }
            } else {
                debug_throw(std::runtime_error("BUG: should never be here"));
            }
        }
        return res;
    }
    template<class T>
    using FOVMap = std::map<
        cv::Point, T, chipimgproc::PointLess 
    >;
    using FOVMarkerNum = FOVMap<cv::Point>;
    static FOVMarkerNum generate_fov_marker_num(
        const nlohmann::json& chip_spec,
        const nlohmann::json& cell_fov
    ) {
        // TODO: all use cache
        FOVMarkerNum marker_num;
        auto& shooting_marker   = chip_spec["shooting_marker"];
        auto& position_cl       = shooting_marker["position_cl"];
        auto& fov_cl            = cell_fov["fov"];

        auto fov_rows   = fov_cl["rows"].get<int>();
        auto fov_cols   = fov_cl["cols"].get<int>();

        auto fov_cl_w_d = fov_cl["w_d"].get<int>();
        auto fov_cl_h_d = fov_cl["h_d"].get<int>();
        auto fov_cl_w   = fov_cl[ "w" ].get<int>();
        auto fov_cl_h   = fov_cl[ "h" ].get<int>();

        auto x_i = position_cl["x_i"].get<int>();
        auto y_i = position_cl["y_i"].get<int>();
        auto row = position_cl["row"].get<int>();
        auto col = position_cl["col"].get<int>();
        auto w_d = position_cl["w_d"].get<int>();
        auto h_d = position_cl["h_d"].get<int>();
        auto  w  = position_cl[ "w" ].get<int>();
        auto  h  = position_cl[ "h" ].get<int>();

        std::vector<EndP> points;
        for( int i = 0; i < fov_cols; i ++ ) {
            add_region(points, i, fov_cl_w_d * i, fov_cl_w, EndP::fov);
        }
        for( int i = 0; i < col; i ++ ) {
            add_region(points, i, x_i + (w_d * i), w, EndP::marker);
        }
        std::sort( points.begin(), points.end() );
        auto fov_mk_overlap_w = fov_mk_overlap_detect(points);

        points.clear();

        for( int i = 0; i < fov_rows; i ++ ) {
            add_region(points, i, fov_cl_h_d * i, fov_cl_h, EndP::fov);
        }
        for( int i = 0; i < row; i ++ ) {
            add_region(points, i, y_i + (h_d * i), h, EndP::marker);
        }
        std::sort( points.begin(), points.end() );
        auto fov_mk_overlap_h = fov_mk_overlap_detect(points);
        int fov_h_id = 0;
        for(auto&& h_fov : fov_mk_overlap_h ) {
            int fov_w_id = 0;
            for(auto&& w_fov : fov_mk_overlap_w ) {
                marker_num[cv::Point(fov_w_id, fov_h_id)] = cv::Point(w_fov, h_fov);
                fov_w_id ++;
            }
            fov_h_id ++;
        }
        return marker_num;
    }
    using FOVMarkerLayouts = FOVMap<chipimgproc::marker::Layout>;
    static FOVMarkerLayouts generate_sgl_pat_reg_mat_marker_layout(
        float um2px_r,
        const nlohmann::json& chip_spec,
        const nlohmann::json& cell_fov,
        const nlohmann::json& channel,
        const std::optional<int>& fov_ec_id = std::nullopt
    ) {
        // TODO: all use cache
        auto& shooting_marker = chip_spec["shooting_marker"];
        auto& position_cl     = shooting_marker["position_cl"];
        auto& position        = shooting_marker["position"];
        auto x_i              = position_cl["x_i"].get<int>();
        auto y_i              = position_cl["y_i"].get<int>();
        auto w_d              = position_cl["w_d"].get<int>();
        auto h_d              = position_cl["h_d"].get<int>();
        auto w_dpx            = position["w_d"].get<int>() * um2px_r;
        auto h_dpx            = position["h_d"].get<int>() * um2px_r;
        auto [pats_cl, pats_px, pats_cl_mask, pats_px_mask]= get_single_marker_pattern(
            um2px_r, shooting_marker, 
            chip_spec["cell_w_um"].get<float>(),
            chip_spec["cell_h_um"].get<float>(),
            chip_spec["space_um"].get<float>(),
            channel
        ); 
        std::uint32_t space_px = std::ceil(um2px_r * chip_spec.at("space_um").get<float>());
        auto marker_num = generate_fov_marker_num(
            chip_spec, cell_fov
        );

        std::map<
            cv::Point, // fov id 
            chipimgproc::marker::Layout, // marker layout
            chipimgproc::PointLess
        > res;
        auto add_marker_layout_ = [&](
            const auto& fov_id, 
            const auto& fov_mk_num, 
            auto& marker_layout 
        ) {
            marker_layout.set_reg_mat_dist(
                fov_mk_num.y, fov_mk_num.x,
                cv::Point(x_i, y_i),
                w_d, h_d,
                w_dpx, h_dpx,
                space_px
            );
            marker_layout.set_single_mk_pat(pats_cl, pats_px, 
                pats_cl_mask, pats_px_mask);
            res[fov_id] = marker_layout;
        };
        if(fov_ec_id) {
            cv::Point fov_id(0,0); // TODO: fov_ec_id to fov_id
            chipimgproc::marker::Layout marker_layout;
            auto fov_mk_num = marker_num[fov_id];
            add_marker_layout_ ( fov_id, fov_mk_num, marker_layout );
        } else {
            for(auto&& mkp : marker_num) {
                chipimgproc::marker::Layout marker_layout;
                auto& fov_mk_num = mkp.second;
                add_marker_layout_(mkp.first, fov_mk_num, marker_layout );     
            }
        }

        return res;
    }
    static auto generate_stitch_points(
        const nlohmann::json& cell_fov
    ) {
        std::map<
            cv::Point, // fov id
            cv::Point,  // stitch points
            chipimgproc::PointLess
        > res;
        auto& fov_cl = cell_fov["fov"];
        auto rows = fov_cl["rows"].get<int>();
        auto cols = fov_cl["cols"].get<int>();
        auto w_d  = fov_cl["w_d"].get<int>();
        auto h_d  = fov_cl["h_d"].get<int>();
        
        for(int r = 0; r < rows; r ++ ) {
            for( int c = 0; c < cols; c ++ ) {
                res[cv::Point(c, r)] = cv::Point(
                    w_d * c,
                    h_d * r
                );
            }
        }
        return res;
    }
    template<class GLID>
    static auto write_gl( std::ostream& gl_file, const chipimgproc::GridRawImg<GLID>& grid_image, const int& decimal = 3) {
        constexpr bool is_float{std::is_floating_point<GLID>::value};
        if constexpr (is_float) {
            gl_file << std::fixed << std::setprecision(decimal);
        } 
        gl_file << "x";
        for( auto& l : grid_image.gl_x() ) {
            gl_file << ',' << l;
        }
        gl_file << std::endl;

        gl_file << "y";
        for( auto& l : grid_image.gl_y() ) {
            gl_file << ',' << l;
        }
        gl_file << std::endl;
    }
    static auto write_background(
        std::ostream& bg_file, 
        const std::string& task_id,
        const FOVMap<float>& data
    ) {
        float sum = 0;
        for(auto& [fov_id, bgv] : data) {
            sum += bgv;
        }
        bg_file << task_id << ',';
        bg_file << sum / data.size();
        for(auto& [fov_id, bgv] : data) {
            bg_file << ',' << bgv;
        }
        bg_file << '\n';
        bg_file << std::flush;
    }
    static auto imread(const std::string& fname_no_ext, bool img_enc, const Paths& data_paths) {
        cv::Mat res;
        if(img_enc) {
            std::ifstream fin(fname_no_ext + ".srl", std::ios::binary);
            summit::crypto::EncryptedScanImage en_img;
            en_img.load(fin);
            res = summit::crypto::scan_image_de(en_img, "qsefthukkuhtfesq");
        } else {
            // #include <chrono>
            // std::chrono::duration<double, std::milli> d;
            // auto timer = std::chrono::steady_clock::now();
            res = chipimgproc::imread(fname_no_ext + ".tiff");
            // d = std::chrono::steady_clock::now() - timer;
            // std::cout << "fov_image read: " << d.count() << " ms\n";
        }
        if( data_paths.secure_output_enabled()) {
            boost::filesystem::path fname_path_no_ext(fname_no_ext);
            // cv::Mat small_image = res;
            // if( res.depth() == CV_8U ) {
            //     small_image = res;
            // } else if( res.depth() == CV_16U) {
            //     res.convertTo(small_image, CV_8U, 0.00390625);
            // }
            // cv::imwrite(
            //     (data_paths.sc_raw_img_dir() 
            //         / (fname_path_no_ext.stem().string() + ".png")).string(), 
            //     small_image
            // );
            // std::chrono::duration<double, std::milli> d;
            // auto timer = std::chrono::steady_clock::now();
            cv::imwrite(
                (data_paths.sc_raw_img_dir() 
                    / (fname_path_no_ext.stem().string() + ".png")).string(), 
                res
            );
            // d = std::chrono::steady_clock::now() - timer;
            // std::cout << "fov_image write: " << d.count() << " ms\n";
        }
        return res;
    }
    // seperated from imread
    static void imwrite(const std::string& fname_no_ext, const Paths& data_paths, cv::Mat res) {
        if( data_paths.secure_output_enabled()) {
            boost::filesystem::path fname_path_no_ext(fname_no_ext);
            // cv::Mat small_image = res;
            // if( res.depth() == CV_8U ) {
            //     small_image = res;
            // } else if( res.depth() == CV_16U) {
            //     res.convertTo(small_image, CV_8U, 0.00390625);
            // }
            // cv::imwrite(
            //     (data_paths.sc_raw_img_dir() 
            //         / (fname_path_no_ext.stem().string() + ".png")).string(), 
            //     small_image
            // );
            // std::chrono::duration<double, std::milli> d;
            // auto timer = std::chrono::steady_clock::now();
            cv::imwrite(
                (data_paths.sc_raw_img_dir() 
                    / (fname_path_no_ext.stem().string() + ".png")).string(), 
                res
            );
            // d = std::chrono::steady_clock::now() - timer;
            // std::cout << "fov_image write: " << d.count() << " ms\n";
        }
    }
    static auto imread(const std::string& fname) {
        boost::filesystem::path fpath(fname);
        std::string ext = fpath.filename().extension().string();
        if(ext == ".srl") {
            std::ifstream fin(fname, std::ios::binary);
            summit::crypto::EncryptedScanImage en_img;
            en_img.load(fin);
            return summit::crypto::scan_image_de(en_img, "qsefthukkuhtfesq");
        } else {
            return chipimgproc::imread(fname);
        }
    }
    template<class Channels>
    static std::string search_white_channel(const Channels& channels) {
        for(auto& ch : channels) {
            if(ch["filter"].template get<int>() == 0) {
                return ch["name"];
            }
        }
        return "";
    }
    template<class Channels>
    static std::string search_first_probe_channel(const Channels& channels) {
        for(auto& ch : channels){
            if(ch["filter"].template get<int>() != 0) {
                return ch["name"];
            }
        }
        return "";
    }
    template<class Int>
    using FOVImages= Utils::FOVMap<
        std::tuple<
            boost::filesystem::path,
            cv::Mat_<Int>
        >
    >;
    template<class Channels>
    static FOVImages<std::uint8_t> read_white_channel(
        const Channels&                 channels,
        const boost::filesystem::path&  src_path,
        int                             rows, 
        int                             cols,
        bool                            img_enc, 
        const Paths&                    data_paths
    ) {
        auto white_ch_name = search_white_channel(channels);
        if(white_ch_name.empty()) return {};
        FOVImages<std::uint8_t> res;
        for ( int r = 0; r < rows; r ++ ) {
            for ( int c = 0; c < cols; c ++ ) {
                std::stringstream ss;
                ss  << std::to_string(r) << '-' 
                    << std::to_string(c) << '-'
                    << white_ch_name
                ;
                auto img_path = src_path / ss.str();
                // std::cout << "read white channel image: " << img_path << std::endl;
                cv::Mat_<std::uint8_t> img = Utils::imread(
                    img_path.string(), img_enc, data_paths
                );
                // chipimgproc::info(std::cout, img);
                res[cv::Point(c, r)] = std::make_tuple(img_path, img);
            }
        }
        return res;
    }
    template<class Channels>
    static FOVImages<std::uint16_t> read_first_probe_channel(
        const Channels&                 channels,
        const boost::filesystem::path&  src_path,
        int                             rows,
        int                             cols,
        bool                            img_enc, 
        const Paths&                    data_paths
    ) {
        auto first_probe_ch_name = search_first_probe_channel(channels);
        if(first_probe_ch_name.empty()) return {};
        FOVImages<std::uint16_t> res;
        for ( int r = 0; r < rows; r ++ ) {
            for ( int c = 0; c < cols; c ++ ) {
                auto [img, img_path] = Utils::read_img<std::uint16_t>(src_path, r, c, 
                                                                      first_probe_ch_name, 
                                                                      img_enc, data_paths);
                res[cv::Point(c, r)] = std::make_tuple(img_path, img);
            }
        }
        return res;
    }
    template<class cvMatT>
    static cv::Mat_<cvMatT> get_fov_mat_from(
        const FOVImages<cvMatT>& all_fov_imgs,
        int                      row_id,
        int                      col_id
    ){
        return std::get<1>(all_fov_imgs.at(cv::Point(col_id, row_id)));
    }
    static bool is_chip_scan( const boost::filesystem::path& path ) {
        return summit::app::grid::is_chip_dir(path);
    }
    static std::vector<boost::filesystem::path> task_paths(
        const boost::filesystem::path& root
    ) {
        namespace bf = boost::filesystem;
        std::vector<bf::path> res;
        auto tmp = root;
        if(bf::is_directory(root)) {
            if( is_chip_scan(root)) {
                res.push_back(
                    tmp.make_preferred()
                );
            } else {
                for(
                    bf::directory_iterator itr(root); 
                    itr != bf::directory_iterator() ; 
                    ++itr 
                ) {
                    bf::path sub_path = itr->path();
                    auto sub_task_paths = task_paths(sub_path);
                    if( !sub_task_paths.empty() ) {
                        res.insert(
                            res.end(), 
                            sub_task_paths.begin(), 
                            sub_task_paths.end()
                        );
                    }
                }
            }
        }
        return res;

    }
    static TaskID to_task_id(const boost::filesystem::path& path) {
        // auto task_id = path.parent_path().filename().string();
        // task_id += "_";
        // task_id += path.filename().string();
        // return task_id;
        auto src = path.parent_path();
        auto rfid = src.filename().string();
        src = src.parent_path();
        auto cid  = path.filename().string();
        return TaskID(cid, rfid, src);
    }
    using FOVMarkerRegionMap = FOVMap<
        std::vector<
            chipimgproc::marker::detection::MKRegion
        >
    >;
    template<class Int>
    static auto read_img(
        const boost::filesystem::path& src_path,
        int row, int col,
        const std::string& postfix,
        bool img_enc, 
        const Paths&                    data_paths
    ) {
        std::stringstream ss;
        ss  << std::to_string(row) << '-' 
            << std::to_string(col) << '-'
            << postfix;
        auto img_path = src_path / ss.str();
        // std::cout << "read image: " << img_path << std::endl;
        cv::Mat_<Int> img = Utils::imread(
            img_path.string(), img_enc, data_paths
        );
        // chipimgproc::info(std::cout, img);
        return nucleona::make_tuple(std::move(img), std::move(img_path));
    }
    template<class Int>
    static auto read_imgs(
        const boost::filesystem::path& src_path,
        int rows, int cols,
        const std::string& postfix,
        bool img_enc, 
        const Paths&                    data_paths
    ) {
        Utils::FOVImages<Int> res;
        for ( int r = 0; r < rows; r ++ ) {
            for ( int c = 0; c < cols; c ++ ) {
                auto [img, img_path] = Utils::read_img<Int>(src_path, r, c, postfix, img_enc, data_paths);
                res[cv::Point(c, r)] = nucleona::make_tuple(
                    img_path.string(), std::move(img)
                );
            }
        }
        return res;
    }
    template <typename Task, typename Int>
    static auto get_future_write_imgs(
        const Task& task, 
        const std::string& ch_name, 
        const Utils::FOVImages<Int>& imgs)
    {
        auto lambda(
            [&task, &ch_name, &imgs](){
                for ( int r = 0; r < task.fov_rows(); r ++ ) {
                    for ( int c = 0; c < task.fov_cols(); c ++ ) {
                        std::stringstream ss;
                        ss  << std::to_string(r) << '-' 
                            << std::to_string(c) << '-'
                            << ch_name;
                        auto img_path = task.chip_dir() / ss.str();
                        Utils::imwrite(
                            img_path.string(), task.model(), 
                            std::get<1>(imgs.at(cv::Point(c, r)))
                        );
                    }
                }
            }
        );

        return std::future<void>(std::async(std::launch::async, lambda));
    }
    template <typename Task, typename FOV>
    static auto get_future_write_imgs(
        const Task& task, 
        const std::string& ch_name, 
        const Utils::FOVMap<FOV>& imgs)
    {
        auto lambda(
            [&task, &ch_name, &imgs](){
                for ( int r = 0; r < task.fov_rows(); r ++ ) {
                    for ( int c = 0; c < task.fov_cols(); c ++ ) {
                        std::stringstream ss;
                        ss  << std::to_string(r) << '-' 
                            << std::to_string(c) << '-'
                            << ch_name;
                        auto img_path = task.chip_dir() / ss.str();
                        Utils::imwrite(
                            img_path.string(), task.model(), 
                            imgs.at(cv::Point(c, r)).src()
                        );
                    }
                }
            }
        );

        return std::future<void>(std::async(std::launch::async, lambda));
    }
    static std::vector<cv::Point> fov_ids(
        int rows, int cols
    ) {
        std::vector<cv::Point> res;
        for( int r = 0; r < rows; r++) {
            for(int c = 0; c < cols; c++) {
                res.push_back(cv::Point(c, r));
            }
        }
        return res;
    }
    template<class Rng>
    static auto mean(Rng&& rng) {
        using Value = nucleona::range::ValueT<std::decay_t<Rng>>;
        Value sum(0);
        for(auto&& v : rng) {
            sum += v;
        }
        return sum / ranges::distance(rng);
    }
    static auto du_um(const std::vector<double>& gl, float um2px_r) {
        std::vector<double> res(gl.size() - 1);
        for(std::size_t i = 0; i < res.size(); i ++ ) {
            res.at(i) = static_cast<int>(std::round(
                (gl.at(i + 1) - gl.at(i)
            ) / um2px_r * 1000)) * 0.001;
        }
        return res;
    }
    static auto abs_px(
        double org,
        const std::vector<double>& gl_du, 
        float um2px_r
    ) {
        std::vector<double> res(gl_du.size() + 1);
        res.at(0) = org;
        for(std::size_t i = 0 ; i < gl_du.size(); i ++ ) {
            res.at(i + 1) = gl_du.at(i) * um2px_r + res.at(i);
        }
        return res;
    }
    template<class Func>
    static auto make_fovs_mk_append(
        Utils::FOVMap<cv::Mat>& fov_mk_append, 
        int fov_rows, int fov_cols,
        Func&& light_tf
    ) {
        Utils::FOVMap<cv::Rect> roi;
        int y_start = 0;
        int x_max(0), y_max(0);
        for(auto y : nucleona::range::irange_0(fov_rows)) {
            int row_max = 0;
            int x_start = 0;
            for(auto x : nucleona::range::irange_0(fov_cols)) {
                cv::Point fov_id(x, y);
                auto&& mk_a = fov_mk_append.at(fov_id);
                auto& fov_roi = roi[fov_id];
                fov_roi.x = x_start;
                fov_roi.y = y_start;
                fov_roi.width  = mk_a.cols;
                fov_roi.height = mk_a.rows;
                summit::grid::log.trace(
                    "fov_roi({},{},{},{})", 
                    fov_roi.x, fov_roi.y, 
                    fov_roi.width, fov_roi.height
                );
                row_max = std::max(row_max, mk_a.rows);
                x_start += mk_a.cols;
            }
            x_max = std::max(x_max, x_start);
            y_start += row_max;
        }
        y_max = std::max(y_max, y_start);
        cv::Point first(0,0);
        auto& first_ma = fov_mk_append.at(first);
        cv::Mat data(
            y_max, x_max, 
            first_ma.type()
        );
        summit::grid::log.trace("data size: ({},{})", data.cols, data.rows);
        for(auto y : nucleona::range::irange_0(fov_rows)) {
            for(auto x : nucleona::range::irange_0(fov_cols)) {
                cv::Point fov_id(x, y);
                auto& fov_roi = roi.at(fov_id);
                auto&& mk_a = fov_mk_append.at(fov_id);
                cv::Mat tmp = light_tf(mk_a);
                // cv::Mat tmp = (mk_a * 8.192) + 8192;
                summit::grid::log.trace("fov_roi: ({},{},{},{})", fov_roi.x, fov_roi.y, fov_roi.width, fov_roi.height);
                tmp.copyTo(data(fov_roi));
                auto white = chipimgproc::cmax(data.depth());
                auto gray = (int)std::round(white / 2);
                cv::rectangle(data, fov_roi, gray, 1);
            }
        }
        return data;
    }
    static auto count_bad_fov_mk_append(
        Utils::FOVMap<cv::Mat>& fov_mk_append,
        Utils::FOVMap<float>& fov_mk_append_correlation,
        cv::Mat& benchmark_img,
        const int& fov_rows,
        const int& fov_cols,
        const float& bad_threshold
    ) {
        int bads = 0;
        for(auto y : nucleona::range::irange_0(fov_rows)) {
            for(auto x : nucleona::range::irange_0(fov_cols)) {
                cv::Point fov_id(x, y);
                auto& mk_a_dn = fov_mk_append.at(fov_id);
                cv::Mat1f PCC = chipimgproc::match_template(mk_a_dn, benchmark_img, cv::TM_CCOEFF_NORMED);
                float R = PCC(0, 0);
                int bad = R < bad_threshold;
                bads += bad;
                if(bad) {
                    fov_mk_append_correlation.at(fov_id) = R;
                }
            }
        }
        return bads;
    }
    static auto compute_sharpness(
        const cv::Mat& img
    ) {
        cv::Mat tmp;
        cv::Mat dst;
        img.convertTo(tmp, CV_32F);
        cv::GaussianBlur(tmp, dst, cv::Size(5, 5), 1, 1, cv::BORDER_REFLECT_101);
        tmp.release();
        cv::Laplacian(dst, tmp, CV_32F);
        return cv::norm(tmp, cv::NORM_L1);
    }
    static auto append_grid_info(
        const boost::filesystem::path&  dst,
        const std::vector<std::string>& oneline_content
    ) {
        std::fstream log4gridding;

        if(!boost::filesystem::exists(dst)) {
            summit::grid::log.info("Create {} for collecting info from gridding program for each chip.", dst.string());
            log4gridding.open(dst.string(), std::fstream::in | std::fstream::out | std::fstream::trunc);
            log4gridding << oneline_content[0] << std::endl;
        } else {
            log4gridding.open(dst.string(), std::fstream::in | std::fstream::out | std::fstream::app);
            summit::grid::log.info("{} found. Append the info to the {}.", dst.string(), dst.string());
        }

        log4gridding << oneline_content[1] << std::endl;
        log4gridding.close();
    }
    template<typename Type>
    static auto table_writer(
        std::stringstream& table,
        Type               content,
        const char&        sep,
        const int&         width,
        const bool&        newline = false
    ) {
        table << std::left << std::setw(width) << content << sep;
        if(newline) {
            table << std::endl;
        }
    }
};

}
