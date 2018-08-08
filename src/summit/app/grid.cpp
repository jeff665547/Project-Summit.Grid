#include <Nucleona/app/main.hpp>
#include <summit/app/grid/main.hpp>

int nucleona::app::main( int argc, char* argv[] ) {
    auto main = summit::app::grid::make(
        summit::app::grid::OptionParser(argc, argv)
    );
    return main();
}