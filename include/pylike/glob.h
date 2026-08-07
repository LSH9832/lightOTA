#pragma once
#ifndef PYLIKE_GLOB_H
#define PYLIKE_GLOB_H
#include <string>
#include <vector>
#include <iostream>
#include <dirent.h>
#include "./os.h"


namespace glob
{
    static std::vector<std::vector<std::string>> glob(const std::string& path_)
    {
        std::string path = path_;
        std::vector<std::string> dirs, files;
        DIR *dir;
        struct dirent *entry;

        if ((dir = opendir(path.c_str())) == NULL) {
            std::cerr << "can not open dir: " << path << std::endl;
            return {};
        }

        if (path[path.size()-1] != '/')
        {
            path += "/";
        }

        while ((entry = readdir(dir)) != NULL) {
            // 忽略 . 和 ..
            if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
                continue;
            }
            
            // std::cout << entry->d_name << std::endl;
            (os::path::isdir(path + entry->d_name)?dirs:files).push_back(entry->d_name);
        }

        closedir(dir);
        std::sort(dirs.begin(), dirs.end());
        std::sort(files.begin(), files.end());

        return {dirs, files};
    }


}
#endif