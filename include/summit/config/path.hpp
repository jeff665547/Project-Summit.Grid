#pragma once
#include <summit/utils.h>
#include <boost/filesystem.hpp>
namespace summit::config{

boost::filesystem::path private_dir();
boost::filesystem::path public_dir();

}