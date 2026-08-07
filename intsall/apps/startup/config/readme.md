# 程序自启动配置路径说明

该目录用于存放软件自启动配置yaml文件

- 命名按照`{优先级}.{软件英文名}.yaml`的格式，优先级从0~9依次变低，假设飞控软件优先级为0，则可以命名为`0.fly_control.yaml`，数字越小越先启动，相同优先级软件将同时启动（误差毫秒级）。
- 执行每个优先级的程序前会先执行`before_start`文件夹下对应的bash文件（从优先级1开始默认为执行睡眠3秒，可按需修改，比如检查优先级更高的某个程序是否正常启动等），如果该优先级下没有需要启动的软件，则对应的bash脚本也不执行（优先级0除外，该脚本必定执行）。
- 如果`before_start`文件夹下缺失对应优先级bash文件会生成默认睡眠3秒的对应的bash文件再运行。

- 如配置文件未按上述规定指定优先级，将视为该软件优先级最低，最晚执行。

- yaml配置格式如下
```yaml
# ---------------------------------------------------------
# 以下为选填项
# ---------------------------------------------------------
# global_version字段，可不填，用于配置该软件环境变量
# 对该软件下的所有可执行程序生效
global_environ:
  APP_DIR: /apps
  ROOT_DIR: $APP_DIR/track_control
  WORK_DIR: $ROOT_DIR/bin
  IMAGE_TOPIC: /camera/image
  DETECT_TOPIC: $IMAGE_TOPIC/detection
  VIDEO_SAVE_PATH: $ROOT_DIR/videos
  LD_LIBRARY_PATH:  # 列表形式不会覆盖原有值，而是添加
    - /usr/local/ffmpeg/lib
    - $APP_DIR/envs/vsoa/lib
    - $WORK_DIR
    - $ROOT_DIR/lib

# 软件启动前运行的bash命令，可不填
global_command:
  # 只运行一次的指令，有可执行程序退出后重新拉起时不会运行，可不填
  run_only_once: 
    - echo [$(date)] start ir track subprogram >> $ROOT_DIR/log/run.log
  # 每次有可执行程序退出后重新拉起时都会重新运行，可不填
  run_every_time:
    - cmd=$($WORK_DIR/param2env --files $ROOT_DIR/config/web/*.yaml);$cmd

# 用于配置与global_command命令有关的的环境变量（大概率用不到）,比如下面这个例子，通过上面的global_command:run_every_time中示例的命令，从文件中加载了环境变量$task_choise_task_name，再配置与其相关的环境变量$MODEL_CONFIG_NAME时就可以用这个字段实现，这个字段与global_environ字段的区别就在于global_environ在global_command执行前配置，这个字段在global_command执行后配置
command_based_environ:
  MODEL_CONFIG_NAME: $(eval echo \$"$task_choise_task_name"_detect_model)

# ---------------------------------------------------------
# 以上为选填项
# ---------------------------------------------------------


# 以下为可执行文件启动配置，至少要填一个，按顺序执行，间隔1秒
# 1. 只包含必填项写法，大部分情况够用
mediamtx: # 软件名称，随便填，不要出现中文字符
  work_dir: $APP_DIR/mediamtx   # 运行目录
  command: ./mediamtx   # 运行指令
# 以上配置将执行指令：cd $APP_DIR/mediamtx && ./mediamtx

# 2. 包含选填项写法
rtsp_pusher:
  work_dir: $WORK_DIR
  command: ./rtsp_push
  # 是否只运行一次，程序退出后不再拉起
  run_once: false
  # 仅对该可执行文件生效的环境变量
  environ:
    PUSH_FPS: 30
  # 当程序命令行参数较多时，可以填在optArgs字段内
  posArgs:
    1: /camera/ir_image/compressed
    2: rtsp://192.168.0.105:8554/ir_track.264
  # 当程序命令行可选参数较多时，可以填在optArgs字段内
  optArgs:
    --fps: $PUSH_FPS
    --kbitrate: 4096
# 以上配置将执行指令：cd $WORK_DIR && ./rtsp_push /camera/ir_image/compressed rtsp://192.168.0.105:8554/ir_track.264 --fps $PUSH_FPS --kbitrate 4096
```

简单的示例如下（OTA服务软件配置）：

```yaml
ota_server:
  work_dir: /apps/lightOTA
  environ:
    SERVICE_PORT: 8088
    OTA_PATH_WHITELIST:
      - /apps
    LD_LIBRARY_PATH:
      - /apps/lightOTA/lib
  command: bin/ota_server $SERVICE_PORT
```