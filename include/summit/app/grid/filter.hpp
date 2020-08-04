#pragma once
#include "heatmap_writer/cell_info.hpp"
namespace summit::app::grid {

struct Filter {
    using Function = std::function<bool(
        const heatmap_writer::CellInfo&
    )>;
};

Filter::Function make_filter(const std::string& name) {
    if( name == "marker_only") {
        return []( const heatmap_writer::CellInfo& ci ) {
            return ci.marker_info.is_marker;
        };
    } else if ( name == "all" ) {
        return []( const heatmap_writer::CellInfo& ci ) {
            return true;
        };
    } else {
        throw std::runtime_error("unsupport filter: " + name );
    }
}

}