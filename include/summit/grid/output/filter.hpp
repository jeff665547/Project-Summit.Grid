#pragma once
#include <summit/grid/output/cell_info.hpp>
namespace summit::grid::output {

struct Filter {
    template<class FLOAT, class GLID>
    using Function = std::function<bool(
        const CellInfo<FLOAT, GLID>&
    )>;


};

template<class FLOAT, class GLID>
Filter::Function<FLOAT, GLID> make_filter(const std::string& name) {
    if( name == "marker_only") {
        return []( const CellInfo<FLOAT, GLID>& ci ) {
            return ci.marker_info.is_marker;
        };
    } else if ( name == "all" ) {
        return []( const CellInfo<FLOAT, GLID>& ci ) {
            return true;
        };
    } else {
        throw std::runtime_error("unsupport filter: " + name );
    }
}

}