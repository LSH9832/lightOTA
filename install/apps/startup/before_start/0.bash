#! /bin/bash

# 本脚本运行于所有软件之前，用于完成网络连通性检测、时间同步等一次性操作，以下为一些示例

# 循环，直到ping通某个IP地址
ping_until_success() {
    local ip=$1
    local timeout=1  # 默认超时时间
    local interval=1 # ping 间隔时间

    while true; do
        if ping -c 1 -W $timeout $ip &> /dev/null; then
            echo "Ping to $ip successful!"
            break
        else
            echo "Ping to $ip failed. Retrying in $interval seconds..."
            sleep $interval
        fi
    done
}
# 在下方可以添加多个IP地址
# ping_until_success 192.168.X.XXX


