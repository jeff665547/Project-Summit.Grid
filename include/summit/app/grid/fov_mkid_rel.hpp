#include "./utils.hpp"

namespace summit::app::grid {

constexpr struct FovMKIDRel {
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
    void add_region( 
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
    std::vector<std::vector<int>> fov_mk_overlap_detect( 
        const std::vector<EndP>& points
    ) const {
        // 
        std::vector<std::vector<int>> res;
        std::set<int> curr_fov;
        std::vector<int> curr_mk_ids;
        for(auto&& p : points ) {
            if(p.tag == EndP::fov) {
                if( p.type == EndP::start ) {
                    res.push_back({});
                    curr_fov.insert(p.id);
                } else if( p.type == EndP::end ) {
                    auto& fov = res[p.id]
                    fov.insert(fov.end(), curr_mk_ids.begin(), curr_mk_ids.end());
                    curr_fov.erase(p.id);
                } else {
                    debug_throw(std::runtime_error("BUG: should never be here"));
                }
            } else if( p.tag == EndP::marker ) {
                if( p.type == EndP::start ) {
                    curr_mk_ids.push_back(p.id);
                } else if ( p.type == EndP::end ) {
                    for(auto& id : curr_fov) {
                        res[id].push_back(p.id);
                    }
                    curr_mk_ids.pop_back();
                } else {
                    debug_throw(std::runtime_error("BUG: should never be here"));
                }
            } else {
                debug_throw(std::runtime_error("BUG: should never be here"));
            }
        }
        return res;
    }
    using FOVMarkerRel = FOVMap<std::set<cv::Point>>;
    FOVMarkerRel operator()(
        const nlohmann::json& chip_spec,
        const nlohmann::json& cell_fov
    ) const {
        // TODO: all use cache
        FOVMarkerRel marker_rel;
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
        for(size_t i = 0; i < fov_mk_overlap_h.size(); i ++) {
            auto& fov_contain_mk_id_h = fov_mk_overlap_h.at(i);
            for(size_t j = 0; j < fov_mk_overlap_w.size(); j ++) {
                auto& fov_contain_mk_id_w = fov_mk_overlap_w.at(j);
                std::set<cv::Point> fov_contain_mk_ids;
                for(auto mkid_h : fov_contain_mk_id_h) {
                    for(auto mkid_w : fov_contain_mk_id_w) {
                        fov_contain_mk_ids.emplace(cv::Point(mkid_w, mkid_h));
                    }
                }
                marker_rel[cv::Point(j, i)] = std::move(fov_contain_mk_ids);
            }
        }
        return marker_rel;
    }
    
} fov_mkid_rel;

}