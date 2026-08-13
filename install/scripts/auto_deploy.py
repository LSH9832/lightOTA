"""
从零部署程序至板端
"""
# import psutil
import paramiko # type: ignore
import warnings
# import chardet
import requests
import time
import os
import os.path as osp
from glob import glob
import io
import datetime
warnings.filterwarnings("ignore")

class AutoDeploy:

    def __init__(self, ip, username, password, ssh_port=22, app=None):
        self.dest_ip = ip
        self.username = username
        self.password = password
        self.ssh_port = ssh_port
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.WarningPolicy())
        self.app = app
        self.__opened = False

    def isOpened(self):
        return self.__opened

    def connect(self):
        if self.__opened:
            return True
        # (print if self.app is None else self.app.add_log)("尝试ssh连接")
        try:
            self.ssh.connect(
                hostname=self.dest_ip, 
                port=self.ssh_port, 
                username=self.username, 
                password=self.password,
                timeout=10
            )
            self.__opened = True
            # (print if self.app is None else self.app.add_log)("ssh连接成功")
            return True
        except paramiko.AuthenticationException:
            (print if self.app is None else self.app.add_log)("认证失败：用户名或密码错误")
            self.ssh.close()
        except paramiko.SSHException as e:
            (print if self.app is None else self.app.add_log)(f"SSH 连接错误: {e}")
            self.ssh.close()
        except Exception as e:
            (print if self.app is None else self.app.add_log)(f"发生未知错误: {e}")
            self.ssh.close()
        (print if self.app is None else self.app.add_log)("ssh连接失败")
        return False

    def disconnect(self):
        if self.__opened:
            self.ssh.close()
            self.__opened = False

    def send_file(self, src_: str, dest_: str, chmod="+777", silent=False):
        self.success = False
        if self.__opened:
            while True:
                try:
                    # (print if self.app is None else self.app.add_log)("打开文件传输通道")
                    sftp = self.ssh.open_sftp()
                    # (print if self.app is None else self.app.add_log)("文件传输通道已打开")
                    break
                except:
                    time.sleep(1)
                    print("尝试重连")
                    self.disconnect()
                    self.connect()

            multi = isinstance(src_, list)
            if not multi:
                src_ = [src_]
                dest_ = [dest_]
            

            for i, (src, dest) in enumerate(zip(src_, dest_)):
                self.send_file_show_last_time = time.time()
                def callback(cur, total):
                    self.success = cur == total
                    cur_time = time.time()
                    if self.success or cur_time - self.send_file_show_last_time > 0.1:
                        (print if self.app is None else self.app.add_log)(f"\r正在传输：{cur}/{total}, {cur/total*100:.2f}%", end="")
                        self.send_file_show_last_time = cur_time

                _, _, ret_code = self.exec(f"mkdir -p {osp.dirname(dest)}", True)

                if self.app is not None:
                    if len(src_) > 1:
                        self.app.add_log(f'（{i+1}/{len(src_)}）开始上传：{osp.basename(src)}')
                    self.app.add_log(f"正在传输：")
                sftp.put(src, dest, None if silent else callback)  # 上传
                # sftp.putfo()

                _, _, ret_code = self.exec(f"chmod {chmod} {dest}", True)

            
            sftp.close()
            print()
        else:
            (print if self.app is None else self.app.add_log)("未连接目标主机！无法传输文件！")
        return self.success

    def send_file_bytes(self, bytes, dest: str, chmod="+777", silent=False, callback=None):
        byte_stream = io.BytesIO(bytes)
        self.success = False
        if self.__opened:
            sftp = self.ssh.open_sftp()
            self.send_file_show_last_time = time.time()
            def callback_default(cur, total):
                self.success = cur == total
                cur_time = time.time()
                if self.success or cur_time - self.send_file_show_last_time > 0.1:
                    print(f"\r正在传输：{cur}/{total}, {cur/total*100:.2f}%", end="")
                    self.send_file_show_last_time = cur_time
            _, _, ret_code = self.exec(f"mkdir -p {osp.dirname(dest)}", True)
            if callback is None:
                callback = callback_default
            # sftp.put(src, dest, None if silent else callback)  # 上传
            # if self.app is not None:
            #     self.app.add_log(f"正在传输：")
            sftp.putfo(byte_stream, dest, len(bytes), None if silent else callback)
            _, _, ret_code = self.exec(f"chmod {chmod} {dest}", True)
            sftp.close()
            print()
        else:
            (print if self.app is None else self.app.add_log)("未连接目标主机！无法传输文件！")
        return self.success


    def request_ota_get(self, url="getOTAFileList", params=None, timeout=20):
        try:
            response = requests.get(f"http://{self.dest_ip}/ota/api/{url}", params=params, timeout=timeout)
            response.raise_for_status()
            data = response.json()
            return data
        except requests.exceptions.HTTPError as http_err:
            (print if self.app is None else self.app.add_log)(f"HTTP 错误发生: {http_err}")
        except requests.exceptions.ConnectionError:
            (print if self.app is None else self.app.add_log)("连接错误：无法连接到服务器")
        except requests.exceptions.Timeout:
            (print if self.app is None else self.app.add_log)("请求超时")
        except requests.exceptions.JSONDecodeError:
            (print if self.app is None else self.app.add_log)("JSON 解码错误：返回的内容不是有效的 JSON格式")
        except Exception as err:
            (print if self.app is None else self.app.add_log)(f"发生其他错误: {err}")
        return None

    def exec(self, command, silence=False, no_return=False):
        if self.__opened:
            if not silence:
                (print if self.app is None else self.app.add_log)(f"开始执行命令：{command}")
                print(f"开始执行命令：{command}")
            stdin, stdout, stderr = self.ssh.exec_command(command)
            if not no_return:
                output = stdout.read()  #.decode('utf-8')
                error = stderr.read()   #.decode('utf-8')
                exit_status = stdout.channel.recv_exit_status()
                if not silence:
                    if exit_status == 0:
                        (print if self.app is None else self.app.add_log)("命令执行成功")
                    else:
                        (print if self.app is None else self.app.add_log)(f"命令执行失败 (Exit Code: {exit_status}):\n{error}")
                return output, error, exit_status
            return None, None, None
        else:
            (print if self.app is None else self.app.add_log)("未连接目标主机！无法传输文件！")
            return None, None, None

    def getBash(self, install_path):
        number = datetime.datetime.now().strftime(".%Y%m%d%H%M%S")
        bash = f"""cd {install_path}
unzip auto_start.zip
cd tools
chmod +x screen
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)
./screen -dmS auto_deploy{number} bash
./screen -x -S auto_deploy{number} -X stuff 'sudo su\\n'
sleep 1
./screen -x -S auto_deploy{number} -X stuff '{self.password}\\n'
sleep 1
./screen -x -S auto_deploy{number} -X stuff 'mkdir -p /apps/lightOTA/ota_files\\n'
./screen -x -S auto_deploy{number} -X stuff 'chmod -R +777 /apps/*\\n'
./screen -x -S auto_deploy{number} -X stuff 'chmod -R +777 /apps/lightOTA/ota_files\\n'
./screen -x -S auto_deploy{number} -X stuff 'chmod -R +777 /apps/lightOTA/ota_files/*\\n'
./screen -x -S auto_deploy{number} -X stuff 'cd {install_path}\\n'
./screen -x -S auto_deploy{number} -X stuff 'bash setup.bash\\n'
sleep 1
./screen -x -S auto_deploy{number} -X stuff 'chmod -R +777 /apps/*\\n'
./screen -x -S auto_deploy{number} -X stuff 'chmod -R +777 /apps/lightOTA/ota_files\\n'
./screen -x -S auto_deploy{number} -X stuff 'chmod -R +777 /apps/lightOTA/ota_files/*\\n'
./screen -x -S auto_deploy{number} -X stuff 'sleep 1\\n'
./screen -x -S auto_deploy{number} -X stuff 'service nginx start\\n'
./screen -x -S auto_deploy{number} -X stuff 'service apps_auto_start start\\n'
./screen -x -S auto_deploy{number} -X stuff 'exit\\n'
sleep 10
./screen -x -S auto_deploy{number} -X stuff 'exit\\n'
sleep 10
./screen -x -S auto_deploy{number} -X stuff 'exit\\n'
sleep 10
./screen -x -S auto_deploy{number} -X stuff 'exit\\n'
echo done
"""
        return bash

def install_base(deploy: AutoDeploy, app=None):
    if not deploy.connect():
        return
    install_path = "/tmp/autostart"
    (print if app is None else app.add_log)("开始上传ota服务软件包")
    deploy.send_file("./auto_start.zip", osp.join(install_path, "auto_start.zip").replace("\\", "/"))
    (print if app is None else app.add_log)("正在安装并启动ota服务软件包")
    deploy.send_file_bytes(
        deploy.getBash(install_path).encode("utf8"), 
        osp.join(install_path, "install.sh").replace("\\", "/"), silent=True
    )

    deploy.exec(f"cd {install_path};bash install.sh", silence=True, no_return=True)

    (print if app is None else app.add_log)("请稍等")
    time.sleep(5)
    deploy.disconnect()


def upload_ota_files(deploy: AutoDeploy, ota_files, app=None):
    if not deploy.connect():
        (print if app is None else app.add_log)("连接失败！")
        return
    ota_dests = []
    for i, ota_file in enumerate(ota_files):
        ota_dest = osp.join("/apps/lightOTA/ota_files", osp.basename(ota_file)).replace("\\", "/")
        ota_dests.append(ota_dest)

    while True:
        try:
            # (print if app is None else app.add_log)(f"（{i+1}/{len(ota_files)}）: 开始上传OTA包：{osp.basename(ota_file).replace("\\", "/")}")
            deploy.send_file(ota_files, ota_dests)
            # (print if app is None else app.add_log)(f"上传成功")
            break
        except FileNotFoundError:
            (print if app is None else app.add_log)("等待授权")
            time.sleep(5)
    deploy.disconnect()


def install_uploaded_ota_files(deploy: AutoDeploy, files=None, app=None):
    data: dict = None
    while True:
        (print if app is None else app.add_log)("获取ota文件列表中")
        data = deploy.request_ota_get("getOTAFileList")
        if data is not None:
            if data.get("value", None) is not None:
                break
        (print if app is None else app.add_log)("获取ota文件列表失败，1秒后再试")
        time.sleep(1)
    value = sorted(data["value"])
    if files is None:
        files = value

    files = [osp.basename(file) for file in sorted(files)]

    files2update = []
    for file in files:
        if file in value:
            files2update.append(file)
        else:
            (print if app is None else app.add_log)(f"文件 {file} 未上传成功")
    
    (print if app is None else app.add_log)(f"共{len(files2update)}个ota文件，开始安装")
    for i, fn in enumerate(files2update):
        (print if app is None else app.add_log)(f"（{i+1}/{len(files2update)}）：开始安装{fn}")
        data = deploy.request_ota_get("update", params={"name": fn})
        if data is not None:
            if data.get("success", False):
                # (print if app is None else app.add_log)(f"{fn}安装成功")
                continue
        (print if app is None else app.add_log)("安装失败！")

def main():
    deploy = AutoDeploy("192.168.0.105", "hzhy", "1")
    # deploy.exec("ls /apps")

    # # 1. install
    # install_base(deploy)

    # # 2.upload otas
    # ota_files = glob("../ota_files/*.ota")
    # upload_ota_files(deploy, ota_files)

    # 3.install ota files
    install_uploaded_ota_files(deploy)
    


# 使用示例
if __name__ == "__main__":
    main()
    