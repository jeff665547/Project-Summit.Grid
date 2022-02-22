#pragma once
#include <string>
#include <ChipImgProc/utils.h>
#include <ChipImgProc/warped_mat/patch.hpp>
namespace summit::app::grid::heatmap_writer{
struct MarkerInfo {
    bool        is_marker   ;
    int         mk_id_x     ;
    int         mk_id_y     ;
    int         sub_x       ;
    int         sub_y       ;
};
struct CellInfo {
    using MatElem = chipimgproc::warped_mat::Patch;
    CellInfo(
        int row, int col, 
        const MatElem& ele,
        bool           is_marker,
        int            mk_id_x,
        int            mk_id_y,
        cv::Rect&      mk
    )
    : cl_x        ( col            )
    , cl_y        ( row            )
    , width       ( ele.patch.cols )
    , height      ( ele.patch.rows )
    , mean        ( ele.mean       )
    , stddev      ( ele.stddev     )
    , cv          ( ele.cv         )
    , bg          ( ele.bg         )
    , num         ( ele.num        )
    , img_x       ( ele.img_p.x    )
    , img_y       ( ele.img_p.y    )
    , probe       ( ele.patch      )
    , marker_info {
        is_marker,
        mk_id_x, mk_id_y,
        col - mk.x,
        row - mk.y
    }
    { }

    CellInfo()
    : cl_x        ()
    , cl_y        ()
    , width       ()
    , height      ()
    , mean        ()
    , stddev      ()
    , cv          ()
    , bg          ()
    , num         ()
    , img_x       ()
    , img_y       ()
    , probe       ()
    , marker_info ()
    {}

    int         cl_x        ;
    int         cl_y        ;
    int         width       ;
    int         height      ;
    int         num         ;
    double      mean        ;
    double      stddev      ;
    double      cv          ;
    double      bg          ;
    double      img_x       ;
    double      img_y       ;
    cv::Mat     probe       ;

    MarkerInfo  marker_info ;
};

}
