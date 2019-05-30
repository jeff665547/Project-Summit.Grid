#pragma once
#include <vector>
#include <map>
#include <ChipImgProc/utils.h>
#include <Nucleona/tuple.hpp>
#include <iostream>
namespace summit::app::grid {

struct RegMatMkIndex {
    RegMatMkIndex(const std::vector<cv::Rect>& mks) // column major
    : markers_(mks)
    {
        for( auto& mk : markers_ ) {
            cl_col_idx_[mk.x] = -1;
            cl_row_idx_[mk.y] = -1;
        }
        int i = 0; 
        for( auto& p : cl_col_idx_ ) {
            p.second = i;
            i ++;
        }
        i = 0; 
        for( auto& p : cl_row_idx_ ) {
            p.second = i;
            i ++;
        }
    }
    bool search(int cl_x, int cl_y, cv::Rect& mk_reg, int& mk_id_x, int& mk_id_y) const {
        for(auto& mk : markers_) {
            if( mk.contains(cv::Point(cl_x, cl_y)) ) {
                mk_reg = mk;
                mk_id_x = cl_col_idx_.at(mk_reg.x);
                mk_id_y = cl_row_idx_.at(mk_reg.y);
                return true;
            }
        }
        return false;


        // auto x_itr = cl_col_idx_.find(cl_x);
        // if(x_itr == cl_col_idx_.end()) return false;
        // const auto& x = x_itr->second;

        // auto y_itr = cl_row_idx_.find(cl_y);
        // if(y_itr == cl_row_idx_.end()) return false;
        // const auto& y = y_itr->second;

        // const auto& mk = markers_.at(cl_row_idx_.size() * x + y); // the marker is column major
        // std::cout << "index search marker: " << mk << std::endl;
        // mk_reg = mk;
        // return true;
    }
private:
    const std::vector<cv::Rect>&    markers_;
    std::map<int, int>              cl_col_idx_;
    std::map<int, int>              cl_row_idx_;
};

}