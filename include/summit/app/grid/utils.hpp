#pragma once
#include <boost/filesystem.hpp>
#include <summit/config/chip.hpp>
#include <summit/config/cell_fov.hpp>
#include <ChipImgProc/comb/single_general.hpp>
#include <summit/utils.h>
#include <limits>
#include <ChipImgProc/marker/loader.hpp>
#include <Nucleona/range.hpp>
#include <optional>
#include <iostream>
namespace summit::app::grid{
struct Utils{
    template<class T>
    static auto guess_um2px_r( 
        const cv::Mat_<T>& src, 
        const nlohmann::json& chip_spec 
    ) {
        auto u8_src = chipimgproc::norm_u8(src);
        auto& shooting_marker = chip_spec["shooting_marker"];
        auto& shooting_marker_type = shooting_marker["type"];
        std::cout << "shooting_marker_type: " << shooting_marker_type << std::endl;
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
            std::cout << "load pattern: " << pat_img_path << std::endl;
            cv::Mat pat_img = cv::imread(pat_img_path.make_preferred().string(), cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
            chipimgproc::info(std::cout, pat_img);
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

    static auto get_single_marker_pattern( 
        float um2px_r,
        const nlohmann::json& shooting_marker
    ) 
    {
        std::vector<cv::Mat_<std::uint8_t>> candi_pats_cl;
        std::vector<cv::Mat_<std::uint8_t>> candi_pats_px;

        auto& mk_pats           = shooting_marker["mk_pats"];
        auto& mk_pats_cl        = shooting_marker["mk_pats_cl"];

        // collect candi_pats_cl;
        for( auto&& mk : mk_pats_cl ) {
            auto path = summit::install_path() / mk.get<std::string>();
            std::ifstream fin(path.string());
            candi_pats_cl.push_back(
                chipimgproc::marker::Loader::from_txt(fin, std::cout)
            );
        } 

        // collect candi_pats_px;
        for( auto&& mk : mk_pats ) {
            if( std::abs(mk["um2px_r"].get<float>() - um2px_r) < 0.001 ) {
                auto path = summit::install_path() / mk["path"].get<std::string>();
                cv::Mat pat_img = cv::imread(path.make_preferred().string(), cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
                candi_pats_px.push_back(pat_img);
            }
        }

        return nucleona::make_tuple(std::move(candi_pats_cl), std::move(candi_pats_px));
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
    static auto generate_sgl_pat_reg_mat_marker_layout(
        float um2px_r,
        const nlohmann::json& chip_spec,
        const nlohmann::json& cell_fov,
        const std::optional<int>& fov_ec_id = std::nullopt

    ) {

        auto& shooting_marker   = chip_spec["shooting_marker"];
        auto& position_cl       = shooting_marker["position_cl"];
        auto& position          = shooting_marker["position"];
        auto& fov_cl            = cell_fov["fov"];

        auto fov_rows = fov_cl["rows"].get<int>();
        auto fov_cols = fov_cl["cols"].get<int>();

        auto x_i = position_cl["x_i"].get<int>();
        auto y_i = position_cl["y_i"].get<int>();
        auto row = position_cl["row"].get<int>();
        auto col = position_cl["col"].get<int>();
        auto w_d = position_cl["w_d"].get<int>();
        auto h_d = position_cl["h_d"].get<int>();
        auto  w  = position_cl[ "w" ].get<int>();
        auto  h  = position_cl[ "h" ].get<int>();

        auto fov_cl_w_d = fov_cl["w_d"].get<int>();
        auto fov_cl_h_d = fov_cl["h_d"].get<int>();
        auto fov_cl_w   = fov_cl[ "w" ].get<int>();
        auto fov_cl_h   = fov_cl[ "h" ].get<int>();

        auto w_dpx = position["w_d"].get<int>() * um2px_r;
        auto h_dpx = position["h_d"].get<int>() * um2px_r;


        auto [pats_cl, pats_px]= get_single_marker_pattern(um2px_r, shooting_marker); 

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

        std::map<
            cv::Point, // fov id
            cv::Point, // marker number
            chipimgproc::PointLess
        > marker_num;
        int fov_h_id = 0;
        for(auto&& h_fov : fov_mk_overlap_h ) {
            int fov_w_id = 0;
            for(auto&& w_fov : fov_mk_overlap_w ) {
                marker_num[cv::Point(fov_w_id, fov_h_id)] = cv::Point(w_fov, h_fov);
                fov_w_id ++;
            }
            fov_h_id ++;
        }

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
                w_dpx, h_dpx
            );
            marker_layout.set_single_mk_pat(pats_cl, pats_px);
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
    static auto write_gl( std::ostream& gl_file, const chipimgproc::GridRawImg<GLID>& grid_image) {
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
};


}