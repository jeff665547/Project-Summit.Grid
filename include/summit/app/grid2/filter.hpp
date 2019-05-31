#pragma once
#include "heatmap_writer/cell_info.hpp"
namespace summit::app::grid2 {

struct Filter {
    template<class FLOAT, class GLID>
    using Function = std::function<bool(
        const heatmap_writer::CellInfo<FLOAT, GLID>&
    )>;
};

template<class FLOAT, class GLID>
Filter::Function<FLOAT, GLID> make_filter(const std::string& name) {
    if( name == "marker_only") {
        return []( const heatmap_writer::CellInfo<FLOAT, GLID>& ci ) {
            return ci.marker_info.is_marker;
        };
    } else if ( name == "all" ) {
        return []( const heatmap_writer::CellInfo<FLOAT, GLID>& ci ) {
            return true;
        };
    } else {
        throw std::runtime_error("unsupport filter: " + name );
    }
}

}