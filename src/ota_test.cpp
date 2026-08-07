#include <iostream>
#include "ota_server.h"
#include <thread>

int main(int argc, char** argv)
{
    // ota::LightOTA ota_handle("./ota_files");
    // ota_handle.setWhiteList({
    //     "/mnt/e/packages/lightOTA/test2"
    // });

    // std::cout << "ret msg: " << ota_handle.try_ota("test.zip").message << std::endl;
    if(argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <PORT>" << std::endl;
        return 1;
    }

    OTAServer server(std::atoi(argv[1]));

    server.run();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}