#include <opencv2/core/ocl.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

int main()
{
    const char* c_var(std::getenv("OPENCV_OPENCL_DEVICE"));
    std::string var;
    if (c_var != nullptr)
        var = c_var;
    std::cout << "OPENCV_OPENCL_DEVICE=" << c_var << std::endl;
    
    cv::ocl::Context context;
    if (!context.create(cv::ocl::Device::TYPE_ALL))
        std::cout << "OpenCL not available...\n";
    else {
        std::cout << context.ndevices() << " devices are detected:\n";
        for (int i = 0; i < context.ndevices(); i++) {
            auto&& device = context.device(i);
            std::cout << "name:              " << device.name() << std::endl;
            std::cout << "available:         " << device.available() << std::endl;
            std::cout << "imageSupport:      " << device.imageSupport() << std::endl;
            std::cout << "OpenCL_C_Version:  " << device.OpenCL_C_Version() << std::endl;
            std::cout << std::endl;
        }
    }
}