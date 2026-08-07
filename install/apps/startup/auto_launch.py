import os
import os.path as osp
from glob import glob
import time

WORKDIR = osp.dirname(__file__)
LAUNCH = osp.join(WORKDIR, "launch.py")

def check_before_start_bash():
    for i in range(10):
        bash_name = osp.join(WORKDIR, f"before_start/{i}.bash")
        if not osp.isfile(bash_name):
            with open(bash_name, "w") as bf:
                bf.write("sleep 3")


def get_config():
    last = []
    names = {}
    for i in range(10):
        names[f"{i}"] = []
    for config_path in glob(osp.join(WORKDIR, "config/**/*.yaml"), recursive=True):
        prefix = osp.basename(config_path).split(".")[0]
        names.get(prefix, last).append(config_path)
    names["last"] = last
    return names


def exec_one_layer(paths, index):
    if len(paths) == 0:
        if index > -1:
            print(f"没有优先级为{index}的软件")
        else:
            print(f"没有最低优先级的软件")
        if index != 0:
            return

    if index != -1:
        print(f"执行优先级为{index}的启动前脚本")
        bash_name = osp.join(WORKDIR, f"before_start/{index}.bash")
        os.system(f"bash {bash_name}")
    else:
        time.sleep(3)
    if len(paths):
        print(f"启动优先级为{index}的软件，共{len(paths)}个")
    for i, p in enumerate(paths):
        sname = osp.basename(p)
        print(f"({i+1}) ---> {sname}")
        cmd = f"{LAUNCH} {p} -i 1 -t 1"
        os.system(f"screen -dmS {sname} bash")
        os.system(f"screen -x -S {sname} -X stuff '{cmd}\n'")


def main():
    check_before_start_bash()
    cfg = get_config()
    for i in range(10):
        exec_one_layer(cfg[f"{i}"], i)
    exec_one_layer(cfg["last"], -1)

if __name__ == "__main__":
    main()
    print("所有软件启动完毕")