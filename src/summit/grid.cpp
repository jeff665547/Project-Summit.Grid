#include <Nucleona/app/main.hpp>
#include <summit/grid/main.hpp>

int nucleona::app::main( int argc, char* argv[] ) {
    auto main = summit::grid::make(
        summit::grid::OptionParser(argc, argv)
    );
    return main();
}