#pragma once
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <fstream>
#include <mutex>
#include "paths.hpp"
#include "utils.hpp"
#include "sup_improc_data.hpp"

namespace summit::app::grid2 {

struct BackgroundWriter {

    BackgroundWriter(
        const Paths& dp
    )
    : data_paths(dp)
    , map_mux_()
    , fid_map_()
    {}

    void write(
        const std::string& task_id,
        const std::string& ch,
        const Utils::FOVMap<float>& bg_data
    ) {
        auto opath = data_paths.background(task_id, ch);
        opath = opath.make_preferred();
        auto itr = fid_map_.find(opath.string());
        if(itr == fid_map_.end()) {
            std::lock_guard<std::mutex> lock(map_mux_);
            if(itr == fid_map_.end()) {
                auto* pfile = new std::ofstream(opath.string());

                fid_map_[opath.string()].reset(pfile);
                fid_mux_[opath.string()].reset(new std::mutex());

                *pfile << "task_id,mean";
                for(auto& [fov_id, bgv] : bg_data) {
                    *pfile << ",(" << fov_id.x << ';' << fov_id.y << ')';
                }
                *pfile << '\n';
            }
        }
        std::cout << "output: " << opath << std::endl;
        auto& fid = fid_map_[opath.string()];
        auto& mux = fid_mux_[opath.string()];
        {
        std::lock_guard<std::mutex> lock(*mux);
        summit::app::grid2::Utils::write_background(*fid, task_id, bg_data);
        }
    }
    void close() {
        for(auto& [task_id, writer] : fid_map_)  {
            writer->close();
        }
    }
    ~BackgroundWriter() {
        close();
    }

private:
    const Paths&                            data_paths          ;
    std::mutex                              map_mux_            ;
    std::map<
        std::string, 
        std::unique_ptr<std::ofstream>
    >                                       fid_map_            ;
    std::map<
        std::string,
        std::unique_ptr<std::mutex>
    >                                       fid_mux_            ;
};


}