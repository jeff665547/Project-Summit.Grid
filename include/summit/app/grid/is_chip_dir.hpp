#pragma once
namespace summit::app::grid {

constexpr auto is_chip_dir = []( const boost::filesystem::path& path ) -> bool{
    return boost::filesystem::exists( path / "chip_log.json" );
};

}