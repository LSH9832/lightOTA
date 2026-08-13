# python脚本介绍

使用前请安装依赖

```bash
pip install -r requirements.txt
```

## 1. `deploy.py`

- 功能：通过ssh、sftp自动部署自启动系统、OTA软件以及其他需要自启动的程序

- 类型：GUI

- 使用方法：

在编译完成的情况下先选中[apps](../apps)文件夹和[setup.bash](../setup.bash)文件，一起打包成auto_start.zip文件与下面的`deploy.py`或`deploy.exe`放在同一目录下

1. 直接使用

```bash
python deploy.py
```

2. 打包为exe后使用

```bash
# 安装nuitka
pip install nuitka

# 打包
nuitka --standalone \
       --windows-console-mode=disable \
       --output-dir=./output \
       --mingw64 \         # 如果python版本≥3.13，去掉这一行
       --enable-plugin=tk-inter \
       --onefile \
       --windows-icon-from-ico=./icon.ico \  # 如果有图标文件的话
       deploy.py
```

在界面上填写下位机IP、端口、用户、密码等信息，再选中需要上传的软件ota包执行部署即可。

## 2. `ota`

- 功能及使用方法：见[部署说明](../readme.md)

- 类型：CLI

- 打包：

```bash
nuitka --standalone --onefile --output-dir=./output ota
```
