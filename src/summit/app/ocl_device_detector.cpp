#include <opencv2/core/ocl.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    cv::ocl::Context context;
    if (!context.create(cv::ocl::Device::TYPE_ALL)) {
        std::cout << "OpenCL not available...\n";
        return 0;
    }
    
    std::vector<cv::ocl::PlatformInfo> platform_info;
    cv::ocl::getPlatfomsInfo(platform_info);
    for (int i = 0; i < platform_info.size(); i++) {
		cv::ocl::PlatformInfo sdk = platform_info.at(i);
		for (int j = 0; j < sdk.deviceNumber(); j++) {
			cv::ocl::Device device;
			sdk.getDevice(device, j);

            std::cout << "Vendor Name:" << sdk.vendor() << std::endl;
            std::cout << "Name:" << device.name() << std::endl;
            std::cout << "available:" << device.available() << std::endl;
            std::cout << "imageSupport:" << device.imageSupport() << std::endl;
            std::cout << "OpenCL_C_Version:" << device.OpenCL_C_Version() << std::endl;
            std::cout << std::endl;
		}
	}
}