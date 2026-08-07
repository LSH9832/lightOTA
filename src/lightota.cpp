#include "lightota.h"
#include <stdint.h>
#include <fstream>
#include <pylike/os.h>
#include <zip.h>
#include "json.hpp"
#include <filesystem>
#include <ostream>
#include <fcntl.h>
#include <unistd.h>

#define IMPL ((Impl*)impl)

using namespace ota;
namespace fs = std::filesystem;


bool add_execute_permission(const std::string& filepath) {
    std::error_code ec;
    
    // 获取当前权限
    auto perms = fs::status(filepath, ec).permissions();
    if (ec) {
        std::cerr << "Error getting status: " << ec.message() << std::endl;
        return false;
    }

    // 添加所有者、组和其他人的可执行权限
    // 注意：std::filesystem::perms 是枚举类，需要使用 bitwise 操作
    perms |= fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;

    // 应用权限
    // fs::perm_options::add 表示在现有权限基础上添加
    fs::permissions(filepath, perms, fs::perm_options::add, ec);
    
    if (ec) {
        std::cerr << "Error setting permissions: " << ec.message() << std::endl;
        return false;
    } else {
        // std::cout << "Permissions updated for " << filepath << std::endl;
        return true;
    }
}

struct Impl
{
    std::string ota_save_path;
    std::string version_path = ".version";
    std::vector<std::string> path_list_white, path_list_black;

    zip_t* za=NULL;

    nlohmann::json version;


    bool isPermit(pystring& file_path)
    {
        for (auto& p: path_list_black) if (file_path.startswith(p)) return false;
        for (auto& p: path_list_white) if (file_path.startswith(p)) return true;
        return true;   
    }

    void loadVersionFile()
    {
        std::string fpath = os::path::join({ota_save_path, version_path});
        if (os::path::isfile(fpath))
        {
            std::ifstream ifs(fpath.c_str());
            std::stringstream buffer;
            buffer << ifs.rdbuf(); // 读取整个文件到字符串流
            ifs.close();
            try {
                version = nlohmann::json::parse(buffer.str());
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "解析错误: " << e.what() << std::endl;
            }
        }
    }

    LightOTA::OTA_RET saveVersionFile()
    {
        LightOTA::OTA_RET ret;
        std::string fpath = os::path::join({ota_save_path, version_path});
        // 2. 打开文件流
        std::ofstream ofs(fpath);
        if (!ofs.is_open()) {
            // std::cerr << "无法创建文件" << std::endl;
            ret.code = RET_FILE_ERROR;
            ret.message = "无法更新版本号文件";
            return ret;
        }

        // 3. 写入文件
        // dump(4) 表示缩进为 4 空格，使文件可读性更好
        ofs << version.dump(4) << std::endl; 
        
        ofs.close();
        return ret;
    }
};



LightOTA::LightOTA() {
    impl = new Impl();
    IMPL->ota_save_path = "./ota_files";
    IMPL->loadVersionFile();
}

LightOTA::LightOTA(const std::string& ota_file_save_path)
{
    impl = new Impl();
    IMPL->ota_save_path = ota_file_save_path;
    IMPL->loadVersionFile();
}

void LightOTA::setWhiteList(const std::vector<std::string>& whitelist)
{
    IMPL->path_list_white = whitelist;
}

void LightOTA::setBlackList(const std::vector<std::string>& blacklist)
{
    IMPL->path_list_black  = blacklist;
}


LightOTA::OTAInfo LightOTA::get_ota_info(const std::string& file_name, const std::string& password, bool keep_open)
{
    OTAInfo info;
    if (IMPL->za)
    {
        zip_close(IMPL->za);
        IMPL->za = NULL;
    }
    pystring file_path = os::path::abspath(os::path::join({IMPL->ota_save_path, file_name}));
    int err{0};
    // 打开 ZIP 文件
    // ZIP_RDONLY: 只读模式
    // ZIP_CHECKCONS: 进行一致性检查
    IMPL->za = zip_open(file_path.c_str(), ZIP_RDONLY | ZIP_CHECKCONS, &err);
    if (!IMPL->za) {
        info.ret.code = RET_FILE_ERROR;
        info.ret.message = "无法打开'" + file_path.str()  + "'" + "，错误码:" + std::to_string(err);
        return info;
    }

    // 2. 设置默认密码
    // 如果 password 为空，libzip 会尝试无密码打开（针对未加密文件）
    // 如果文件加密但密码为空或错误，后续 zip_fopen_index 会失败
    // if (!password.empty()) 
    if (zip_set_default_password(IMPL->za, password.c_str()) != 0) {
        info.ret.code = RET_WRONG_PASSWORD;
        info.ret.message = "密码错误";
        zip_close(IMPL->za);
        IMPL->za = NULL;
        return info;
    }

    info.file_name = os::path::basename(file_name).str();
    info.file_size = os::path::getsize(file_path);

    zip_int64_t num_entries = zip_get_num_entries(IMPL->za, 0);

    bool find_config = true;
    for (zip_int64_t i = 0; i < num_entries; ++i)
    {
        const char* name = zip_get_name(IMPL->za, i, 0);
        if (!name) continue;

        pystring file_name(name);
        if (file_name.endswith(".otaconfig"))
        {
            info.config_index = i;
            zip_file_t* zf = zip_fopen_index(IMPL->za, i, 0);
            if (!zf) {
                info.ret.code = RET_CONTENT_ERROR;
                std::cerr << "Error opening file inside zip: " << file_name << std::endl;
                info.ret.message += "打开OTA配置时出现错误\n";
                break;
            }

            char buf[1024 * 32];
            zip_int64_t bytes_read;
            bytes_read = zip_fread(zf, buf, sizeof(buf));
            if (bytes_read == sizeof(buf))
            {
                info.ret.code = RET_CONFIG_TOO_LARGE;
                info.ret.message += "OTA配置文件过大！\n";
                zip_fclose(zf);
                break;
            }
            std::string content(buf, bytes_read);

            nlohmann::json cfg;
            try {
                // 尝试解析
                cfg= nlohmann::json::parse(content);
                zip_fclose(zf);
            } 
            catch (const nlohmann::json::parse_error& e) {
                // 专门捕获解析错误
                std::ostringstream oss;
                
                oss << "JSON Parse Error: " << e.what() << "\n";
                oss << "Exception ID: " << e.id << "\n";      // 错误代码，如 101, 103 等
                oss << "Byte Position: " << e.byte << "\n";   // 出错位置的字节索引

                info.ret.code = RET_CONFIG_ERROR;
                info.ret.message = oss.str();
                zip_fclose(zf);
                break;
            }
            catch (const std::exception& e) {
                // 捕获其他可能的异常（如 type_error, out_of_range 等）
                std::cerr << "Other Error: " << e.what() << std::endl;
                info.ret.code = RET_CONFIG_ERROR;
                info.ret.message = e.what();
                zip_fclose(zf);
                break;
            }
            for (auto key: {"version", "root_dir", "name"})
            if (cfg.find(key) == cfg.end())
            {
                info.ret.code = RET_CONFIG_ERROR;
                info.ret.message = "missing key '"  + std::string(key) + "'";
            }

            if (cfg.find("introduction") != cfg.end())
            {
                info.intro = cfg["introduction"];
            }

            if (cfg.find("userdata") != cfg.end())
            {
                info.userdata = cfg["userdata"];
            }

            if (cfg.find("timestamp") != cfg.end())
            {
                info.publish_timestamp = (std::vector<double>)cfg["timestamp"];
            }

            info.version = cfg["version"];
            info.root_path = cfg["root_dir"];
            info.software_names = cfg["name"];
            
            break;
        }
    }

    if (!keep_open)
    {
        zip_close(IMPL->za);
        IMPL->za = NULL;
    }
    return info;
}


LightOTA::OTA_RET LightOTA::unzip(const std::vector<char>& buffer, const std::string& password)
{
    OTA_RET ret;
    zip_error_t error;
    
    // 1. 从 buffer 创建 source
    // 参数说明:
    // data: 数据指针
    // len: 数据长度
    // free_data: 0 表示 libzip 不拥有数据内存（不要释放它），1 表示 libzip 接管并负责释放
    //            对于 std::vector，通常传 0，因为 vector 自己管理内存
    zip_source_t* source = zip_source_buffer_create(
        buffer.data(), 
        buffer.size(), 
        0, 
        &error
    );

    if (!source) {
        std::cerr << "Error creating source: " << zip_error_strerror(&error) << std::endl;
        ret.code = RET_FILE_ERROR;
        ret.message = "无法以zip格式打开文件！";
        return ret;
    }

    // 2. 从 source 打开 ZIP 归档
    // ZIP_RDONLY: 只读模式
    zip_t* archive = zip_open_from_source(source, ZIP_RDONLY, &error);

    if (!archive) {
        std::cerr << "Error opening archive: " << zip_error_strerror(&error) << std::endl;
        ret.code = RET_FILE_ERROR;
        ret.message = "文件打开失败";
        zip_source_free(source); // 如果打开失败，需要手动释放 source
        return ret;
    }

    if (zip_set_default_password(archive, password.c_str()) != 0) {
        ret.code = RET_WRONG_PASSWORD;
        ret.message = "密码错误";
        zip_close(archive);
        archive = NULL;
        return ret;
    }

    // 3. 正常读取 ZIP 内容
    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
    bool has_file{false};
    for (zip_int64_t i = 0; i < num_entries; ++i) {
        pystring name = zip_get_name(archive, i, 0);
        if (name.lower().endswith(".ota")) {
            // 读取文件内容示例
            has_file = true;
            zip_file_t* file = zip_fopen_index(archive, i, 0);
            if (file) {
                auto cur_file_path = os::path::join({IMPL->ota_save_path, os::path::basename(name)});
                auto cur_file_path_temp = cur_file_path + ".tmp";

                std::ofstream out_file(cur_file_path_temp.str(), std::ios::binary);
                if (!out_file.is_open()) {
                    std::cerr << "Failed to create output file: " << cur_file_path << std::endl;
                    if (ret.code && ret.code != RET_FILE_ERROR)
                    {
                        ret.code = RET_COMPLEX_ERROR;
                    }
                    else ret.code = RET_FILE_ERROR;
                    ret.message += "写临时文件'" + name.str() + ".tmp'时出现错误\n";
                    zip_fclose(file);
                    continue;
                }

                char buf[1024 * 32];
                zip_int64_t bytes_read;
                while ((bytes_read = zip_fread(file, buf, sizeof(buf))) > 0) {
                    out_file.write(buf, bytes_read);
                }
                
                out_file.close();
                zip_fclose(file);

                // 原子替换原文件（关键：绕过 ETXTBSY 运行程序占用限制）
                int rename_ret = ::rename(cur_file_path_temp.c_str(), cur_file_path.c_str());
                if (rename_ret != 0)
                {
                    std::cerr << "Failed to replace target file: " << cur_file_path
                            << " errno:" << errno << " msg:" << strerror(errno) << std::endl;
                    // 删除无用临时文件
                    ::unlink(cur_file_path_temp.c_str());

                    ret.code = RET_FILE_ERROR;
                    ret.message += "临时文件替换目标文件'" + name.str() + "'失败\\n";
                }
            }
            else
            {
                if (ret.code && ret.code != RET_CONTENT_ERROR)
                {
                    ret.code = RET_COMPLEX_ERROR;
                }
                else ret.code = RET_CONTENT_ERROR;
                std::cerr << "Error opening file inside zip: " << name << std::endl;
                ret.message += "打开文件'" + name.str() + "'时出现错误\n";
                continue;
            }
        }
    }
    if (!has_file)
    {
        ret.code = RET_FILE_ERROR;
        ret.message = "未在压缩包中找到任何ota文件！";
    }

    // 4. 关闭归档
    // 注意：zip_close 会自动释放关联的 source（如果 source 是由 libzip 内部管理的）
    // 但在某些版本或特定情况下，如果 zip_open_from_source 成功，source 的所有权转移给 archive
    zip_close(archive);
    return ret;
}

LightOTA::OTA_RET LightOTA::try_ota(const std::string& file_name, const std::string& password)
{
    OTA_RET ret;

    pystring file_path = os::path::join({IMPL->ota_save_path, file_name});

    auto info = get_ota_info(file_name, password, true);
    if (info.ret.code != RET_NO_ERROR)
    {
        return info.ret;
    }

    zip_int64_t num_entries = zip_get_num_entries(IMPL->za, 0);

    for (zip_int64_t i = 0; i < num_entries; ++i) {
        if (i == info.config_index) continue;  // 跳过OTA配置文件

        // 获取文件名
        const char* name = zip_get_name(IMPL->za, i, 0);
        if (!name) continue;

        pystring file_name(name);
        pystring cur_file_path = os::path::abspath(os::path::join({info.root_path, file_name}));

        if (!IMPL->isPermit(cur_file_path))
        {
            if (ret.code && ret.code != RET_PERMISSION_DENIED)
            {
                ret.code = RET_COMPLEX_ERROR;
            }
            else ret.code = RET_PERMISSION_DENIED;
            ret.message += "文件'" + file_name.str() + "'不在允许的OTA更新路径下\n";
            continue;
        }
        
        // 跳过目录条目（通常以 / 结尾）
        if (file_name.endswith("/")) {
            os::makedirs(cur_file_path);
            continue;
        }

        // 打开 ZIP 内的文件进行读取
        zip_file_t* zf = zip_fopen_index(IMPL->za, i, 0);
        if (!zf) {
            if (ret.code && ret.code != RET_CONTENT_ERROR)
            {
                ret.code = RET_COMPLEX_ERROR;
            }
            else ret.code = RET_CONTENT_ERROR;
            std::cerr << "Error opening file inside zip: " << file_name << std::endl;
            ret.message += "打开文件'" + file_name.str() + "'时出现错误\n";
            continue;
        }

        // 计算目标文件路径
        // std::string target_path = safe_join_path(dest_dir, file_name);
        
        // 确保目标文件的父目录存在
        os::makedirs(os::path::dirname(cur_file_path));
        // fs::create_directories(fs::path(target_path).parent_path());
        auto cur_file_path_temp = cur_file_path + ".tmp";
        // 读取数据并写入磁盘
        std::ofstream out_file(cur_file_path_temp.str(), std::ios::binary);
        if (!out_file.is_open()) {
            std::cerr << "Failed to create output file: " << cur_file_path_temp << std::endl;
            if (ret.code && ret.code != RET_FILE_ERROR)
            {
                ret.code = RET_COMPLEX_ERROR;
            }
            else ret.code = RET_FILE_ERROR;
            ret.message += "写临时文件'" + file_name.str() + ".tmp'时出现错误\n";
            zip_fclose(zf);
            continue;
        }

        char buf[1024 * 32];
        zip_int64_t bytes_read;
        while ((bytes_read = zip_fread(zf, buf, sizeof(buf))) > 0) {
            out_file.write(buf, bytes_read);
        }
        
        out_file.close();
        zip_fclose(zf);

        // 原子替换原文件（关键：绕过 ETXTBSY 运行程序占用限制）
        int rename_ret = ::rename(cur_file_path_temp.c_str(), cur_file_path.c_str());
        if (rename_ret != 0)
        {
            std::cerr << "Failed to replace target file: " << cur_file_path
                    << " errno:" << errno << " msg:" << strerror(errno) << std::endl;
            // 删除无用临时文件
            ::unlink(cur_file_path_temp.c_str());

            ret.code = RET_FILE_ERROR;
            ret.message += "临时文件替换目标文件'" + file_name.str() + "'失败\\n";
            continue;
        }


        if(!add_execute_permission(cur_file_path))
        {
            ret.code = RET_FILE_PERMISSION_NOT_CHANGED;
            ret.message += "未能添加可执行权限：" + os::path::basename(cur_file_path).str();
        }
        
    }

    // 更新version
    if (ret.code == RET_NO_ERROR)
    {
        if (info.version.size() == info.software_names.size())
        {
            for (int i=0;i<info.software_names.size();i++)
            {
                IMPL->version[info.software_names[i]] = info.version[i];
            }
            auto save_ret = IMPL->saveVersionFile();
            if (save_ret.code != RET_NO_ERROR)
            {
                if (ret.code != RET_NO_ERROR && ret.code != save_ret.code)
                {
                    ret.code = RET_COMPLEX_ERROR;
                    ret.message += save_ret.message;
                }
                else
                {
                    ret = save_ret;
                }
            }
        }
        else
        {
            ret.code = RET_CONFIG_ERROR;
            ret.message = "配置文件信息有误！版本号与软件名未对齐，版本号未更新！";
        }
    }
    


    zip_close(IMPL->za);
    IMPL->za = NULL;
    return ret;
}

std::map<std::string, std::string> LightOTA::getVersion()
{
    std::map<std::string, std::string> ret;
    for(auto& [k, v]: IMPL->version.items())
    {
        ret[k] = v;
    }
    return ret;
}

LightOTA::~LightOTA()
{
    if(impl != nullptr)
    {
        delete impl;
        impl = nullptr;
    }
}