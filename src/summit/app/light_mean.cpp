#include <string>
#include <iostream>
#include <fstream>
#include <summit/app/light_mean.hpp>
#include <Nucleona/algo/split.hpp>

int main(int argc, char** argv) {
    std::string line;
    std::ifstream fin(argv[1]);
    summit::app::LightMean light_mean;
    std::basic_string<double> means;
    std::size_t i = 0;
    while(std::getline(fin, line)) {
        if( i++ == 0 ) continue;
        auto fields = nucleona::algo::split(line, "\t");
        auto mean = std::stod(fields[4]);
        means.push_back(mean);
    }
    std::cout << light_mean(means, std::atoi(argv[2])) << std::endl;
}