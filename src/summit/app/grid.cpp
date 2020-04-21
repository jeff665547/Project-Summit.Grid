/**
 * @file grid.cpp
 * @author Chia-Hua Chang (johnidfet@centrilliontech.com.tw)
 * @brief The main of Summit.Grid
 * 
 */

#include <Nucleona/app/main.hpp>
#include <summit/app/grid/main.hpp>
/**
 * @brief The main function, replaced by Nucleona's app framework
 * 
 * @param argc argc
 * @param argv argv
 * @return int exit code
 */
int nucleona::app::main( int argc, char* argv[] ) {
    auto main = summit::app::grid::make(
        summit::app::grid::OptionParser(argc, argv)
    );
    return main();
}