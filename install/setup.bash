#! /bin/bash
if [ "$(id -u)" -ne 0 ]; then
    echo "本脚本必须在root用户下运行才能生效"
    exit 1
fi

sudo apt install screen -y   # 如果为离线环境，注释掉此行

cp -r ./apps /
chmod +x /apps/startup/*.py
chmod +x /apps/lightOTA/bin/*
chmod +x /apps/lightOTA/lib/*

cp ./apps_auto_start.service /lib/systemd/system
chmod +x /lib/systemd/system/apps_auto_start.service
systemctl enable apps_auto_start.service

chmod -R +777 /apps/*
sleep 1
systemctl start apps_auto_start

# 如果为离线环境，在本目录下创建tools文件夹并将screen及其依赖库
# libaudit.so.1 libcap-ng.so.0 libpam.so.0 libutempter.so.0
# 放进去
# 并取消以下两行注释
# cp ./tools/screen /usr/bin
# cp ./tools/*so* /lib/aarch64-linux-gnu
