#include "ota_server.h"

#include <FlaskCpp/FlaskCpp.h>
#include "lightota.h"
#include "pylike/logger.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <map>

namespace fs = std::filesystem;


std::string showSize(size_t sz)
{
    static const std::vector<std::string> sz_unit = {"B", "KB", "MB", "GB", "TB", "PB"};
    double sz_ = sz;
    int uidx = 0;
    while (sz_ > 1024)
    {
        sz_ /= 1024;
        uidx++;
    }
    return std::to_string(sz_).substr(0, 5) + sz_unit[uidx];
}

std::vector<std::string> list_files_with_extension(const std::string& path, const std::vector<std::string>& extensions) {
    std::vector<std::string> result;
    
    try {
        // directory_iterator 仅遍历当前目录，不递归子目录
        // recursive_directory_iterator 可遍历子目录
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) { // 确保是普通文件，排除目录
                std::string file_ext = entry.path().extension().string();
                
                // 转换为小写进行比对
                std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);
                
                for (auto& extension: extensions)
                {
                    // 统一后缀名为小写以便比较（可选，视需求而定）
                    std::string ext_lower = extension;
                    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);
                    if (file_ext == ext_lower) {
                        result.push_back(os::path::basename(entry.path().string()));
                        break;
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }

    return result;
}

struct Impl
{
    FlaskCpp* app{nullptr};
    ota::LightOTA* ota_handle{nullptr};
    std::string prefix="", ota_save_path="./ota_files", html_file_path="./source/html";
    int port = 8081;
    std::string title_name, footer_name;

    void set()
    {
        if (app == nullptr)
        {
            app = new FlaskCpp("ota_server", 2, 128);
            
        }
    }

    void run()
    {
        if (app == nullptr) set();
        if (ota_handle == nullptr)
        {
            os::makedirs(ota_save_path);
            ota_handle = new ota::LightOTA(ota_save_path);
        }

        if (getenv("OTA_PATH_WHITELIST"))
        {
            pystring ota_paths = getenv("OTA_PATH_WHITELIST");
            std::vector<std::string> wlist;
            for (pystring p: ota_paths.split(":"))
            {
                if (p.length()) wlist.push_back(p);
            }
            ota_handle->setWhiteList(wlist);
            // INFO << "ota mode: whitelist" << ENDL;
        }
        if (getenv("OTA_PATH_BLACKLIST"))
        {
            pystring ota_paths = getenv("OTA_PATH_BLACKLIST");
            std::vector<std::string> blist;
            for (pystring p: ota_paths.split(":"))
            {
                if (p.length()) blist.push_back(p);
            }
            ota_handle->setBlackList(blist);
            // INFO << "ota mode: blacklist" << ENDL;
        }
        // else INFO << "ota mode: default" << ENDL;

        if (getenv("OTA_TITLE"))
        {
            title_name = getenv("OTA_TITLE");
        }
        else
        {
            title_name = "OTA系统";
        }
        
        if (getenv("OTA_FOOTER"))
        {
            footer_name = getenv("OTA_FOOTER");
        }
        else
        {
            footer_name = "作者：<a href='https://github.com/LSH9832'>LSH9832</a>";
        }

        app->loadTemplatesFromDirectory("source/html");
        
        if (getenv("OTA_SERVER_SECRET_KEY"))
            app->setSecretKey(getenv("OTA_SERVER_SECRET_KEY"));

        app->route2(prefix + "/ota/api/getOTAFileList", [&](const RequestData& req) {
            if (req.isFromData)
            {
                req.acceptForm();
            }
            flaskcpp::JsonGenerator js;
            js.add("success", true);
            int ret_code = 200;

            std::vector<std::string> flists = list_files_with_extension(ota_save_path, {".zip", ".ota"});
            js.add("value", flists);

            return flaskcpp::send_error(js, ret_code, {FLASK_NO_CACHE});
        });

        app->route2(prefix + "/ota/api/getSoftwareList", [&](const RequestData& req) {
            if (req.isFromData)
            {
                req.acceptForm();
            }
            flaskcpp::JsonGenerator js, csf;
            js.add("success", true);
            int ret_code = 200;

            std::vector<std::string> flists = list_files_with_extension(ota_save_path, {".zip", ".ota"});
            auto cur_version = ota_handle->getVersion();
            std::map<std::string, std::vector<std::string>> version_info;
            for (auto f: flists)
            {
                auto info = ota_handle->get_ota_info(f);
                if (info.ret.code)
                {
                    continue;
                    // js.add("success", false);
                    // js.add("message", info.ret.message);
                    // return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_INTERNAL_SERVER_ERROR);
                }

                for (int i=0;i<info.version.size();++i)
                {
                    if (cur_version.find(info.software_names[i]) == cur_version.end()) continue;

                    if (version_info.find(info.software_names[i]) == version_info.end())
                    {
                        version_info[info.software_names[i]] = {};
                    }
                    version_info[info.software_names[i]].push_back(info.version[i]);
                    version_info[info.software_names[i]].push_back(f);
                }
            }
            std::vector<std::string> sfnames;
            for (auto& [name, version]: cur_version)
            {
                flaskcpp::JsonGenerator sf;
                sf.add("support_version", std::vector<std::string>{});
                if (version_info.find(name) != version_info.end())
                {
                    sf.add("support_version", version_info[name]);
                }
                sf.add("current_version", version);
                csf.add(name, sf);
            }
            js.add("value", csf);

            return flaskcpp::send_error(js, ret_code, {FLASK_NO_CACHE});
        });

        

        app->route2(prefix + "/ota/api/update", [&](const RequestData& req) {
            if (req.isFromData)
            {
                req.acceptForm();
            }
            flaskcpp::JsonGenerator js;
            js.add("success", true);
            int ret_code = 200;

            if (req.queryParams.find("name") == req.queryParams.end())
            {
                js.add("success", false);
                js.add("message", "请求缺少关键字name!");
                return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_BAD_REQUEST, {FLASK_NO_CACHE});
            }

            std::string fname = osp::join({ota_save_path, req.queryParams.at("name")});
            if (!osp::isfile(fname))
            {
                js.add("success", false);
                js.add("message", "服务器未找到文件!");
                return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_BAD_REQUEST, {FLASK_NO_CACHE});
            }
            auto ret = ota_handle->try_ota(req.queryParams.at("name"));
            if (ret.code)
            {
                js.add("success", false);
                js.add("message", pystring(ret.message).replace("\n", "<br>").str());
                return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_INTERNAL_SERVER_ERROR);
            }
            return flaskcpp::send_error(js, 200, {FLASK_NO_CACHE});
        });

        app->route2(prefix + "/ota/api/info", [&](const RequestData& req) {
            if (req.isFromData)
            {
                req.acceptForm();
            }
            flaskcpp::JsonGenerator js;
            js.add("success", true);
            int ret_code = 200;

            if (req.queryParams.find("name") == req.queryParams.end())
            {
                js.add("success", false);
                js.add("message", "请求缺少关键字name!");
                return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_BAD_REQUEST, {FLASK_NO_CACHE});
            }

            std::string fname = osp::join({ota_save_path, req.queryParams.at("name")});
            if (!osp::isfile(fname))
            {
                js.add("success", false);
                js.add("message", "服务器未找到文件!");
                return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_BAD_REQUEST, {FLASK_NO_CACHE});
            }
            auto ret = ota_handle->get_ota_info(req.queryParams.at("name"));
            if (ret.ret.code)
            {
                js.add("success", false);
                js.add("message", pystring(ret.ret.message).replace("\n", "<br>").str());
                return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_INTERNAL_SERVER_ERROR);
            }
            else
            {
                if (!ret.userdata.empty()) js.add("用户信息", ret.userdata);
                if (!ret.intro.empty()) 
                {
                    if(ret.intro.size() == ret.software_names.size())
                    {
                        pystring intro = "";
                        for (int i=0;i<ret.intro.size();i++) if(!ret.intro[i].empty())
                        {
                            if (i) intro += "<br>";
                            intro += "【" + ret.software_names[i] + "】<br>" + ret.intro[i];
                        }
                        js.add("更新说明", intro);
                    }
                }

                pystring pub_ts = "未知", version_pub="";
                auto cur_version = ota_handle->getVersion();

                if (ret.publish_timestamp.size() == 1 && ret.version.size() == 1 &&
                    ret.software_names.size() == 1)
                {
                    pub_ts = datetime::datetime::from_timestamp(ret.publish_timestamp[0]).strftime("%Y-%m-%d %H:%M:%S");
                    version_pub = ret.software_names[0] + " " + ret.version[0] + " ";
                    if (cur_version.find(ret.software_names[0]) != cur_version.end())
                    {
                        version_pub += " （当前版本 " + cur_version.at(ret.software_names[0]) + "）";
                    }
                    else version_pub += " （当前版本未记录）";
                }
                else if (ret.publish_timestamp.size() > 1 && 
                         ret.publish_timestamp.size() == ret.software_names.size() &&
                         ret.publish_timestamp.size() == ret.version.size())
                {
                    pub_ts = "";
                    for (int i=0;i<ret.software_names.size();i++)
                    {
                        if (i)
                        {
                            pub_ts += "<br>";
                            version_pub += "<br>";
                        }
                        pub_ts += ret.software_names[i] + " " + datetime::datetime::from_timestamp(ret.publish_timestamp[i]).strftime("%Y-%m-%d %H:%M:%S").str();
                        version_pub += ret.software_names[i] + " " + ret.version[i];
                        if (cur_version.find(ret.software_names[i]) != cur_version.end())
                        {
                            version_pub += " （当前版本 " + cur_version.at(ret.software_names[i]) + "）";
                        }
                        else version_pub += " （当前版本未记录）";
                    }
                    
                }

                js.add("发布时间", pub_ts);
                js.add("OTA包大小", showSize(ret.file_size));
                js.add("版本号", version_pub);
                js.add("文件名称", ret.file_name);
            }
            return flaskcpp::send_error(js, 200, {FLASK_NO_CACHE});
        });

        app->route2(prefix + "/ota/api/deleteFile", [&](const RequestData& req) {
            if (req.isFromData)
            {
                req.acceptForm();
            }
            flaskcpp::JsonGenerator js;
            js.add("success", true);
            int ret_code = 200;

            if (req.queryParams.find("name") == req.queryParams.end())
            {
                js.add("success", false);
                js.add("message", "请求缺少关键字name!");
                return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_BAD_REQUEST, {FLASK_NO_CACHE});
            }

            std::string fname = osp::join({ota_save_path, req.queryParams.at("name")});
            if (!osp::isfile(fname))
            {
                js.add("success", false);
                js.add("message", "服务器未找到文件!");
                return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_BAD_REQUEST, {FLASK_NO_CACHE});
            }
            if(!fs::remove(fname))
            {
                js.add("success", false);
                js.add("message", "删除失败！");
                ret_code = flaskcpp::RESP_TYPE_INTERNAL_SERVER_ERROR;
            }
            return flaskcpp::send_error(js, ret_code, {FLASK_NO_CACHE});
        });

        app->route2(prefix + "/ota/api/uploadFile", [&](const RequestData& req) {
            flaskcpp::JsonGenerator js;
            js.add("success", true);
            bool has_file=false;
            int ret_code = 200;
            if (req.isFromData)
            {
                req.acceptForm();
            }

            for (auto& [name, file]: req.files)
            {
                pystring fn = file.file_name;
                bool correct = fn.lower().endswith(".zip") || fn.lower().endswith(".ota");

                if (!correct)
                {
                    js.add("success", false);
                    js.add("message", "文件类型错误！");
                    return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_NOT_ACCEPTABLE, {FLASK_NO_CACHE});
                }
                
                // std::cout << fn << std::endl;
                if (fn.endswith(".zip"))
                {
                    auto ret = ota_handle->unzip(file.data);
                    if (ret.code)
                    {
                        js.add("success", false);
                        js.add("message", pystring(ret.message).replace("\n", "<br>").str());
                        return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_INTERNAL_SERVER_ERROR);
                    }
                    return flaskcpp::send_error(js, 200);
                }
                std::string ret = flaskcpp::save_file(file, ota_save_path);
                if (!ret.empty())
                {
                    js.add("success", false);
                    js.add("message", ret);
                    return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_CONFLICT, {FLASK_NO_CACHE});
                }
                if (req.queryParams.find("update") != req.queryParams.end())
                {
                    auto ret = ota_handle->try_ota(file.file_name);
                    if (ret.code)
                    {
                        js.add("success", false);
                        js.add("message", pystring(ret.message).replace("\n", "<br>").str());
                        return flaskcpp::send_error(js, flaskcpp::RESP_TYPE_INTERNAL_SERVER_ERROR);
                    }
                }
                has_file = true;
            }
            js.add("success", has_file);
            if (!has_file) {
                js.add("message", "服务器没有找到上传文件！");
                ret_code = flaskcpp::RESP_TYPE_BAD_REQUEST;
            }

            return flaskcpp::send_error(js, ret_code, {FLASK_NO_CACHE});
        });

        app->route2(prefix + "/ota", [&](const RequestData& req) {
            return flaskcpp::send_error(app->jump2("/ota/softwares.html"), 200);
        });

        app->route2(prefix + "/ota/api/get_html_path", [&](const RequestData& req) {
            std::string fpath = osp::abspath(html_file_path);
            return flaskcpp::send_error(fpath, 200);
        });

        app->route2(prefix + "/ota/<path:address>", [&](const RequestData& req) {
            pystring fpath = osp::join({html_file_path, req.routeParams.at("address")});
            if (os::path::isfile(fpath))
            {
                if (fpath.endswith(".html"))
                {
                    TemplateEngine::Context ctx;
                    ctx["name"] = title_name;
                    ctx["footer"] = footer_name;
                    return flaskcpp::send_text(app->renderTemplate(osp::basename(fpath), ctx), {FLASK_NO_CACHE});
                }
                return flaskcpp::send_file(fpath, osp::basename(fpath));
            }
            return flaskcpp::send_error("<h1>404 Not Found</h1>", 404);
        });

        


        app->runAsync(port, true);
    }

    void stop()
    {
        if (app != nullptr) app->stop();
        delete app;
        app = nullptr;
    }
};

#define IMPL ((Impl*)impl)

OTAServer::OTAServer(int port)
{
    impl = new Impl();
    IMPL->port = port;
}

void OTAServer::setRoutePrefix(const std::string& prefix)
{
    IMPL->prefix = prefix;
}

void OTAServer::setOTAPath(const std::string& path)
{
    IMPL->ota_save_path = path;
}

void OTAServer::setWebSourcePath(const std::string& path)
{
    IMPL->html_file_path = path;
}

void OTAServer::run()
{
    IMPL->run();
}

void OTAServer::stop()
{
    IMPL->stop();
}

OTAServer::~OTAServer()
{
    stop();
    delete IMPL;
}
