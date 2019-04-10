#include <string>
#include <iostream>
#include <fstream>
#include <summit/app/light_mean/main.hpp>
#include <Nucleona/app/main.hpp>
int nucleona::app::main(int argc, char** argv) {
    summit::app::light_mean::Main __main(
        summit::app::light_mean::OptionParser(argc, argv)
    );
    return __main();
}
// int main(int argc, char** argv) {
//     std::string line;
//     std::ifstream fin(argv[1]);
//     std::ofstream fout(argv[3]);
//     summit::app::LightMean light_mean;
//     std::basic_string<double> means;
//     std::size_t i = 0;
//     std::string curr_task_id;
//     while(std::getline(fin, line)) {
//         if( i == 0 ) {
//             i++;
//             continue;
//         }
//         auto fields = nucleona::algo::split(line, "\t");
//         auto mean = std::stod(fields[4]);
//         auto curr_line_task_id = fields[0];
//         if(curr_task_id != curr_line_task_id) {
//             if(!curr_task_id.empty()) {
//                 fout
//                     << curr_task_id << '\t'
//                     << light_mean(means, std::atoi(argv[2])) 
//                     << '\n'
//                 ;
//             }
//             means.clear();
//             curr_task_id = curr_line_task_id;
//         }
//         means.push_back(mean);
//     }
//     if(!means.empty()) {
//         fout
//             << curr_task_id << '\t'
//             << light_mean(means, std::atoi(argv[2])) 
//             << '\n'
//         ;
//     }
// }