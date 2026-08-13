import tkinter as tk
from tkinter import ttk, filedialog, messagebox
# import ttkbootstrap as ttkb
# import paramiko
import os
import os.path as osp
import json
import auto_deploy
from threading import Thread
# import icon_ico
# import auto_start_zip
import time

class SSHUploadTool:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("板端软件部署工具")
        self.root.geometry("550x700")
        self.root.resizable(True, True)

        # 存储选中的本地文件路径
        self.selected_files = []

        # 1. 连接信息区域
        conn_frame = ttk.LabelFrame(root, text="板端配置信息", padding=(15, 10))
        conn_frame.pack(fill=tk.X, padx=15, pady=5)

        # IP 行
        ttk.Label(conn_frame, text="IP").grid(row=0, column=0, padx=5, pady=5, sticky=tk.W)
        self.ip_entry = ttk.Entry(conn_frame, width=25)
        self.ip_entry.grid(row=0, column=1, padx=5, pady=5)

        ttk.Label(conn_frame, text="SSH端口").grid(row=0, column=2, padx=20, pady=5, sticky=tk.W)
        self.port_entry = ttk.Entry(conn_frame, width=18)
        self.port_entry.grid(row=0, column=3, padx=5, pady=5)

        # 用户名密码行
        ttk.Label(conn_frame, text="用户名").grid(row=1, column=0, padx=5, pady=5, sticky=tk.W)
        self.user_entry = ttk.Entry(conn_frame, width=25)
        # self.user_entry.insert(0, "h2hy")
        self.user_entry.grid(row=1, column=1, padx=5, pady=5)

        ttk.Label(conn_frame, text="密码").grid(row=1, column=2, padx=20, pady=5, sticky=tk.W)
        self.pwd_entry = ttk.Entry(conn_frame, width=18, show="*")
        self.pwd_entry.grid(row=1, column=3, padx=5, pady=5)

        # 2. 文件选择区域
        file_frame = ttk.LabelFrame(root, text="OTA文件列表", padding=(15, 5))
        file_frame.pack(fill=tk.X, padx=15, pady=5)

        btn_container = ttk.Frame(file_frame)
        btn_container.pack(fill=tk.X, pady=5)
        
        self.clear_btn = ttk.Button(btn_container, text="清空文件列表", command=self.empty_list)  # , bootstyle="primary"
        self.clear_btn.pack(side=tk.LEFT)

        self.browse_btn = ttk.Button(btn_container, text="浏览", command=self.select_files)  # , bootstyle="primary"
        self.browse_btn.pack(side=tk.RIGHT)

        # 文件列表表格
        columns = ("序号", "文件名")
        self.file_tree = ttk.Treeview(file_frame, columns=columns, show="headings", height=8)
        self.file_tree.heading("序号", text="序号")
        self.file_tree.heading("文件名", text="文件名")
        self.file_tree.column("序号", width=50, anchor=tk.CENTER)
        self.file_tree.column("文件名", width=500, anchor=tk.CENTER)
        self.file_tree.pack(fill=tk.X, pady=5)

        # 3. 上传按钮
        upload_frame = ttk.Frame(root)
        upload_frame.pack(fill=tk.X, padx=15, pady=0)

        self.check_var = tk.BooleanVar()
        self.check_var.set(True)
        self.deploy_ota_service_checkbox = ttk.Checkbutton(upload_frame, text="部署OTA服务（每台设备仅需部署一次）", variable=self.check_var)
        self.deploy_ota_service_checkbox.setvar(value="1")
        self.deploy_ota_service_checkbox.pack(anchor=tk.E)

        self.upload_btn = ttk.Button(upload_frame, text="开始部署", command=self.start_upload_thread, width=12)  # , bootstyle="success"
        self.upload_btn.pack(anchor=tk.E)

        # 4. 信息反馈区域
        log_frame = ttk.LabelFrame(root, text="信息", padding=(5, 5))
        log_frame.pack(fill=tk.BOTH, expand=True, padx=15, pady=10)

        # self.log_text = tk.Text(log_border_frame, height=10, font=("", 10), state=tk.NORMAL)
        # self.log_text.pack(fill=tk.BOTH, expand=True)

        self.log_text = tk.Text(
            log_frame,
            bg="white",  # 日志区域背景色按需自定义
            bd=3,  # 把Text原生边框设为0，避免和模拟边框冲突
        )
        # 通过padx/pady控制边框粗细，数值越大黑色边框越宽
        self.log_text.pack(padx=1, pady=1, fill=tk.BOTH, expand=True)

        self.deploy_thread = None

        self.config_file = "deploy.config"
        self.cfg = {}
        self.load_config()

    def load_config(self):
        if osp.isfile(self.config_file):
            with open(self.config_file) as f:
                self.cfg = json.load(f)

        if "IP" in self.cfg:
            self.ip_entry.insert(0, self.cfg["IP"])
        # if "port" in self.cfg:
        self.port_entry.insert(0, str(self.cfg.get("port", 22)))
        if "username" in self.cfg:
            self.user_entry.insert(0, self.cfg["username"])
        if "password" in self.cfg:
            self.pwd_entry.insert(0, self.cfg["password"])

        for f in self.cfg.get("upload_files", []):
            if f not in self.selected_files:
                self.selected_files.append(f)
                filename = os.path.basename(f)
                idx = len(self.selected_files)
                self.file_tree.insert("", tk.END, values=(idx, filename))

    def empty_list(self):
        self.selected_files = []
        for item in self.file_tree.get_children():
            self.file_tree.delete(item)
        self.cfg["upload_files"] = []
        with open(self.config_file, "w") as f:
            json.dump(self.cfg, f)


    def select_files(self):
        """打开文件选择对话框，添加文件到列表"""
        files = filedialog.askopenfilenames(title="选择ota文件", filetypes=[("升级包文件", "*.ota")])
        for f in files:
            if f not in self.selected_files:
                self.selected_files.append(f)
                filename = os.path.basename(f)
                idx = len(self.selected_files)
                self.file_tree.insert("", tk.END, values=(idx, filename))
        self.cfg["upload_files"] = self.selected_files
        if len(files):
            self.add_log(f"共添加 {len(files)} 个文件")

    def add_log(self, msg, end="\n"):
        """向反馈框追加日志"""
        # self.log_text.insert(tk.END, f"[系统] {msg}\n")
        if not len(end):
            # 定位到最后一行的起始位置
            last_line_start = self.log_text.index("end-2l linestart")
            # 删除最后一行原有内容
            self.log_text.delete(last_line_start, tk.END)
            # 写入新的日志内容
            self.log_text.insert(tk.END, f"\n[系统] {msg}\n")
        else:
            # 常规追加模式，直接在末尾写入新日志
            self.log_text.insert(tk.END, f"[系统] {msg}\n")
        self.log_text.see(tk.END)
        self.root.update()

    def start_upload_thread(self):
        self.deploy_thread = Thread(target=self.start_upload)
        self.deploy_thread.daemon = True
        self.deploy_thread.start()

    def start_upload(self):
        
        if not self.selected_files:
            messagebox.showwarning("注意", "请先选择需要上传的OTA文件！")
            return

        # 获取连接参数
        ip = self.ip_entry.get().strip()
        if not self.port_entry.get().strip().isdigit():
            messagebox.showerror("错误", "ssh端口不是一个有效数字！")
            return
        port = int(self.port_entry.get().strip())
        username = self.user_entry.get().strip()
        password = self.pwd_entry.get().strip()

        self.cfg["IP"] = ip
        self.cfg["port"] = port
        self.cfg["username"] = username
        self.cfg["password"] = password
        with open(self.config_file, "w") as f:
            json.dump(self.cfg, f)

        if (username != "root" and not all([ip, port, username, password])) or not all([ip, port]):
            messagebox.showerror("错误", "板端信息不能为空")
            return

        # ssh = None
        try:
            # self.add_log(f"connecting {ip}:{port} ...")
            # ssh = paramiko.SSHClient()
            # ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            # ssh.connect(ip, port=port, username=username, password=password, timeout=10)
            # self.add_log("SSH connect success!")

            # sftp = ssh.open_sftp()
            # for local_path in self.selected_files:
            #     filename = os.path.basename(local_path)
            #     remote_path = f"./{filename}"
            #     self.add_log(f"uploading: {filename}")
            #     sftp.put(local_path, remote_path)
            #     self.add_log(f"file {filename} upload success")

            # sftp.close()
            # self.add_log("all files uploaded!")
            # messagebox.showinfo("Success", "all files uploaded!")
            self.browse_btn.config(state="disabled")
            self.clear_btn.config(state="disabled")
            self.upload_btn.config(state="disabled")

            deploy = auto_deploy.AutoDeploy(ip, username, password, port, self)
            print(ip, username, password, port)
            if self.check_var.get():
                auto_deploy.install_base(deploy, self)
            
            self.add_log("开始上传ota文件")
            auto_deploy.upload_ota_files(deploy, self.selected_files, self)
            auto_deploy.install_uploaded_ota_files(deploy, self.selected_files, self)
            messagebox.showinfo("提示", "执行完毕")
            self.add_log("执行完毕")
        except Exception as e:
            self.add_log(f"上传错误: {str(e)}")
            messagebox.showerror("错误", f"操作失败: {str(e)}")
            raise
        finally:
            # 后续需要恢复可用时，再切换回normal状态
            self.browse_btn.config(state="normal")
            self.clear_btn.config(state="normal")
            self.upload_btn.config(state="normal")
            deploy.disconnect()
            self.add_log("断开连接")


if __name__ == "__main__":
    app = tk.Tk()
    tool = SSHUploadTool(app)
    if osp.isfile("icon.ico"):
        app.iconbitmap("icon.ico")
    app.mainloop()
