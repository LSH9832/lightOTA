#ifndef OTA_SERVER_H
#define OTA_SERVER_H

#include <string>

class OTAServer
{
public:
    OTAServer(int port=8081);

    void setRoutePrefix(const std::string& prefix);

    void setOTAPath(const std::string& path);

    void setWebSourcePath(const std::string& path);

    void run();

    void stop();

    ~OTAServer();

private:
    void* impl{nullptr};
};


#endif