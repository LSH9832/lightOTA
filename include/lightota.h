#ifndef LIGHTOTA_H
#define LIGHTOTA_H

#include <iostream>
#include <vector>
#include <string>
#include <map>

namespace ota
{

enum ErrorCode
{
    RET_NO_ERROR,
    RET_FILE_ERROR,
    RET_LACK_VERSION,
    RET_LACK_ROOT_PATH,
    RET_PERMISSION_DENIED,
    RET_FILE_PERMISSION_NOT_CHANGED,
    RET_WRONG_PASSWORD,
    RET_CONTENT_ERROR,
    RET_COMPLEX_ERROR,
    RET_CONFIG_ERROR,
    RET_CONFIG_TOO_LARGE
};

class LightOTA
{
public:
    struct OTA_RET
    {
        ErrorCode code=RET_NO_ERROR;
        std::string message;
    };

    struct OTAInfo
    {
        OTA_RET ret;
        std::vector<std::string> software_names;
        std::string file_name;
        size_t file_size;
        std::vector<std::string> version;       // 版本号
        std::vector<double> publish_timestamp;  // 最后修改时间
        std::vector<std::string> intro;         // 更新说明
        std::string userdata;
        int config_index{-1};
        std::string root_path; // 更新根目录
    };

    LightOTA();

    /**
     * @brief 构造函数
     * @param ota_file_save_path OTA升级包存放位置
     */
    LightOTA(const std::string& ota_file_save_path);


    /**
     * @brief 获取OTA升级包信息
     * @param file_name OTA升级包文件名，不要带路径
     * @param password 升级包密码，没有则不填
     * @param keep_open 获取信息后是否保持文件打开状态
     */
    OTAInfo get_ota_info(const std::string& file_name, const std::string& password="", bool keep_open=false);

    /**
     * @brief 尝试OTA升级    
     * @param file_name OTA升级包文件名，不要带路径
     * @param password 升级包密码，没有则不填
     */
    OTA_RET try_ota(const std::string& file_name, const std::string& password="");


    /**
     * @brief 将zip包中的OTA包全部解压到存放路径
     * @param file_name OTA升级包文件名，不要带路径
     * @param password 升级包密码，没有则不填
     */
    OTA_RET unzip(const std::vector<char>& zip_data, const std::string& password="");

    /**
     * @brief 将OTA设置为白名单模式，并设置可升级路径白名单
     * @param whitelist 所有允许进行OTA升级的路径，在该名单外的路径中的文件不允许进行OTA升级
     */
    void setWhiteList(const std::vector<std::string>& whitelist);

    /**
     * @brief 将OTA设置为黑名单模式，并设置可升级路径黑名单
     * @param blacklist 所有不允许进行OTA升级的路径，在该名单内的路径中的文件不允许进行OTA升级
     */
    void setBlackList(const std::vector<std::string>& blacklist);

    /**
     * @brief 获取当前版本信息
     * @return 软件名称->版本信息
     */
    std::map<std::string, std::string> getVersion();

    ~LightOTA();
private:
    void* impl{nullptr};

};
}


#endif