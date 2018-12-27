#pragma once
#include <boost/filesystem.hpp>
namespace summit::app::grid {

struct Task {
    enum Type {
        single, chipscan
    };
    // enum Vendor {
    //     tirc, centr
    // };

    boost::filesystem::path  path       ;
    Type                     type       ;

    auto id() const {
        std::string task_id;
        switch(type) {
            case single:
                task_id = path.stem().string();
                break;
            case chipscan:
                task_id = path.parent_path().filename().string();
                task_id += "_";
                task_id += path.filename().string();
                break;
            default:
                throw std::runtime_error("task type not found");
        }
        return task_id;
    }
    static std::vector<Task> search( 
        const boost::filesystem::path& root 
    ) {
        namespace bf = boost::filesystem;
        std::vector<Task> res;
        auto tmp = root;
        if(bf::is_directory(root)) {
            if( is_chip_scan(root)) {
                res.push_back(Task{
                    tmp.make_preferred(),
                    Task::Type::chipscan
                });
            } else {
                for(
                    bf::directory_iterator itr(root); 
                    itr != bf::directory_iterator() ; 
                    ++itr 
                ) {
                    bf::path sub_path = itr->path();
                    auto chip_images = search(sub_path);
                    if( !chip_images.empty() ) {
                        res.insert(
                            res.end(), 
                            chip_images.begin(), 
                            chip_images.end()
                        );
                    }
                }
            }
        }
        else if( is_image( root ) ){
            res.push_back(Task{
                tmp.make_preferred(),
                Task::Type::single
            });
        }
        return res;

    }
private:
    static bool is_chip_log( const boost::filesystem::path& path ) {
        return path.filename().string() == "chip_log.json";
    }
    static bool is_chip_scan( const boost::filesystem::path& path ) {
        return boost::filesystem::exists( path / "chip_log.json" );
    }
    static bool is_image ( const boost::filesystem::path& path ) {
        std::string ext = path.filename().extension().string();
        return ext == ".tif" || ext == ".tiff" || ext == ".srl";
    }
};

}