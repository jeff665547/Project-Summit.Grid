#include <Nucleona/app/main.hpp>
#include <summit/app/grid2/main.hpp>

int nucleona::app::main( int argc, char* argv[] ) {
    auto main = summit::app::grid2::make(
        summit::app::grid2::OptionParser(argc, argv)
    );
    return main();
}