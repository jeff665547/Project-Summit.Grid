#include <opencv2/core/ocl.hpp>

#include <iostream>
#include <stdlib.h>

int main()
{
    char* libvar;
    _putenv_s("OPENCV_OPENCL_DEVICE", "");
    const char* env = getenv("OPENCV_OPENCL_DEVICE");
    if (env)
        std::cout << "OPENCV_OPENCL_DEVICE: " << *env << std::endl;

    cv::ocl::Context context;
    if (!context.create(cv::ocl::Device::TYPE_ALL))
        std::cout << "OpenCL not available...\n";

    std::cout << context.ndevices() << " devices are detected:\n";
    for (int i = 0; i < context.ndevices(); i++) {
        auto&& device = context.device(0);
        std::cout << "name:              " << device.name() << std::endl;
        std::cout << "available:         " << device.available() << std::endl;
        std::cout << "imageSupport:      " << device.imageSupport() << std::endl;
        std::cout << "OpenCL_C_Version:  " << device.OpenCL_C_Version() << std::endl;
        std::cout << std::endl;
    }
}