#pragma once
#include <string>
#include <ChipImgProc/multi_tiled_mat.hpp>
#include <ChipImgProc/utils.h>
namespace summit::app::grid::heatmap_writer{
struct MarkerInfo {
    bool        is_marker   ;
    int         mk_id_x     ;
    int         mk_id_y     ;
    int         sub_x       ;
    int         sub_y       ;
};
template<class FLOAT = float, class GLID = std::uint16_t>
struct CellInfo {
    using Mat = chipimgproc::MultiTiledMat<FLOAT, GLID>;
    using MatElem = typename Mat::MinCVAllData::Result;
    CellInfo(
        int row, int col, 
        const MatElem& ele,
        bool is_marker,
        int mk_id_x,
        int mk_id_y,
        cv::Rect& mk
    )
    : cl_x        ( col )
    , cl_y        ( row )
    , width       ( ele.cell_info.width  )
    , height      ( ele.cell_info.height )
    , mean        ( ele.cell_info.mean   )
    , stddev      ( ele.cell_info.stddev )
    , cv          ( ele.cell_info.cv     )
    , bg          ( ele.cell_info.bg     )
    , probe       ( ele.pixels           )
    , marker_info {
        is_marker,
        mk_id_x, mk_id_y,
        col - mk.x,
        row - mk.y
    }
    { }


    int         cl_x        ;
    int         cl_y        ;
    int         width       ;
    int         height      ;
    FLOAT       mean        ;
    FLOAT       stddev      ;
    FLOAT       cv          ;
    FLOAT       bg          ;
    cv::Mat     probe       ;

    MarkerInfo  marker_info ;
};

}
