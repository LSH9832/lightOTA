# 软件自启动服务说明

## 1.软件自启动服务安装

编译完成后将本目录复制到板端，在板端切换至root账户，在本目录执行以下命令

```bash
bash setup.bash
systemctl start apps_auto_start
```

如无反应重启即可

## 2.软件自启动结构说明

对于每个需要部署的软件，都在目录`/apps`下单独创建一个文件夹去存放软件（可执行文件、动态依赖库、配置文件等）。如预设的OTA服务软件就在`/apps/lightOTA`目录下

对于每个需要部署的软件，要在目录`/apps/startup/config`下编写一个单独的自启动配置文件，如预设的OTA服务软件配置文件路径为`/apps/startup/config/0.ota_server.yaml`，具体配置文件写法及命名规则见[install/apps/startup/config/readme.md](install/apps/startup/config/readme.md)

由上述可知每个软件部署目录如下

```
/apps
    ├── startup
    │      └── config
    │             └── 该软件的自启动配置文件.yaml
    │
    └── 该软件本体所在的文件夹
                 └── 该软件的所有文件
```

## 3. 软件部署方法

使用脚本[scripts/ota](scripts/ota)进行软件打包即可快速部署

### 3.1 便捷使用
为了方便使用，使用以下命令

```bash
sudo chmod -R +777 /usr/local/bin
cp scripts/ota /usr/local/bin
chmod +x /usr/local/bin/ota
```
将ota文件放至运行目录下

为方便使用，请安装python3-argcomplete命令补全工具

```bash
pip install argcomplete --break-system-packages
activate-global-python-argcomplete
# 如果上面安装不成功则用下面这个
sudo apt install python3-argcomplete
sudo activate-global-python-argcomplete3
```

并在`~/.bashrc`中添加

```
# ~/.bashrc
eval "$(register-python-argcomplete ota)"
# 或以下这句，主要看上面到底是哪个安装成功了
eval "$(register-python-argcomplete3 ota)"
```

后执行

```
source ~/.bashrc
```

即可

### 3.2 使用步骤

#### 3.2.1 创建系统根目录信息：`ota sysroot create`

打包时涉及到查找依赖库的问题，因此需要先填写系统根目录信息用于查找

```bash
ota sysroot create <名称> <系统根目录路径> --ld-library-path <添加额外的动态库查找目录，多个路径以空格隔开>
# 基本的动态库查找目录如下，如果没有其他地方有需要用的动态库则无需填写--ld-library-path字段
# /lib64
# /usr/lib64
# /lib
# /usr/lib
# /lib/aarch64-linux-gnu
# /lib/x86_64-linux-gnu
# /usr/local/lib
# /usr/lib/aarch64-linux-gnu
# /usr/lib/x86_64-linux-gnu

# 对于交叉编译环境，比如rk3576编译环境，根目录为/opt/cross-compile/rk3576/sysroot，可如下填写
ota sysroot rk3576 /opt/cross-compile/rk3576/sysroot --ld-library-path /opt/cross-compile/rk3576/sysroot/opt/my_lib

# 如果就是本机编译环境，则可填写如下
ota sysroot origin / --ld-library-path /opt/my_lib
```

创建成功后使用`ota sysroot list`查看所有已创建的系统根目录名称列表，使用`ota sysroot info rk3576`查看对应系统信息

如果发现漏加了额外的动态库，通过`ota sysroot modify rk3576`进行补充

#### 3.2.2 初始化工程目录打包环境并添加打包软件：`ota software XXX`

切换到需要打包的工程目录，执行

```
ota software init
```

便会在目录下生成`.ota`文件夹及相应配置文件，生成的打包文件也在其中

执行

```
ota software add <软件名称> <软件纯英文名称(即板端存放此软件所有文件的目录名称)> \
             -e <软件包含的所有可执行文件，多个文件用空格隔开> \
             -l <所有必须打包的动态库文件，没有则不填> \
             -p <其他额外需要一起打包的文件/文件夹，比如配置文件等，格式为“<文件/文件夹路径>:<打包后相对于程序运行目录的路径>”（即用冒号隔开），没有则不填>
             -r <软件自启动优先级（0~9）,如果不在此范围内或不填则视为最低优先级>
# 举例
ota software add 测试软件 test \
              -e build/test \
              -l build/libload_by_dl.so \   # 通过dlfcn.h动态载入的动态库无法以查找依赖库的方式被找到，应添加至此
              -p install/configs:configs /home/user/models:models
              -r 8
```

此时需要根据命令行提示进行对应根目录选择等操作。操作完成后会在`.ota`文件夹下生成`8.test.yaml`的自启动配置文件，参照[install/apps/startup/config/readme.md](install/apps/startup/config/readme.md)的说明进行修改

一个工程目录下可能存在多个软件，重复执行本节所述操作即可

#### 3.2.3 生成打包文件：`ota software pack`

修改完成后就可以打包了

```
ota software pack <软件纯英文名称> <软件版本号> \ # 版本号格式为：v{大版本号}.{次版本号}.{小版本号}
                  -i <版本说明> \
                  -if <版本说明文件路径（当版本说明文字较多需要写在文件里时使用，与-i最多只需要填一个）> \
                  --no-extra-pack \   # 不打包ota software add命令中通过-p添加的文件
                  --only-pack-new     # 只打包从上次打包起更新过的文件
# 举例
ota software pack test v0.2.4 -if ./log/updates.txt
```

即可等待打包完成，如果找不到所有所需的动态链接库则会打包失败

生成的ota包不会包含系统目录下的依赖库，这些依赖库需要对sysroot作为通用依赖库进行打包，稍后请看后续章节3.2.4

#### 3.2.4 打包通用依赖库：`ota sysroot pack`

当所有需要再板端运行的程序打包完成后，`/home/用户名/.ota/sysroot/名称/lib`路径下将会拥有所有程序所需的依赖库，此时可对其进行打包

```
ota sysroot pack <名称> <版本号> \
                  -i <版本说明> \
                  -if <版本说明文件路径（当版本说明文字较多需要写在文件里时使用，与-i最多只需要填一个）> \
                  --only-pack-new     # 只打包从上次打包起更新过的文件
# 举例
ota sysroot pack rk3576 v0.1.0 -i 增加了一些软件更新后所需的新依赖库 --only-pack-new
```

则会在`/home/用户名/.ota/sysroot/rk3576`路径下生成ota文件。

#### 3.2.5 上传所有打包文件

通过网页打开`http://板端IP:8088/ota/upload.html`即可上传文件，上传成功后自动跳转到ota包详情页，点击左下角`执行更新`按钮即可完成部署。

访问`http://板端IP:8088/ota`即可修改每一个软件的版本，只要老的ota包还在就可以换回来。
