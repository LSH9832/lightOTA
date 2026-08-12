(function () {
    'use strict';

    const uploadArea = document.getElementById('uploadArea');
    const fileInput = document.getElementById('fileInput');
    const selectedFileDiv = document.getElementById('selectedFile');
    const fileNameSpan = document.getElementById('fileName');
    const uploadBtn = document.getElementById('uploadBtn');
    // const returnBtn = document.getElementById('returnBtn');
    const retMessageArea = document.getElementById('ret_message_area');
    const route_prefix = document.getElementById('route_prefix_info').innerText;

    const domainDiv = document.getElementById("domains");
    let domains = [""];
    if (domainDiv){
        domains = domainDiv.innerText.split(":");
    }
    // console.log(route_prefix);

    let selectedFile = null;
    let fileDomain = null;
    const allowedExtensions = ['.ota', '.zip'];

    // returnBtn.addEventListener('click', function () {
    //     // console.log(111);
    //     window.location.href = route_prefix + (fileDomain?fileDomain:"") + '/ota_files.html';
    // });

    // returnBtn.disabled = false;

    // 检查文件扩展名
    function isValidFile(file) {
        const name = file.name.toLowerCase();
        return allowedExtensions.some(ext => name.endsWith(ext));
    }

    function readFileAsArrayBuffer(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = (e) => resolve(e.target.result);
            reader.onerror = (e) => reject(new Error('文件读取失败'));
            reader.readAsArrayBuffer(file);
        });
    }

    // 设置选中的文件
    async function setFile(file) {
        if (!isValidFile(file)) {
            appendMessage('错误：仅支持 .ota 或 .zip 格式的文件', 'error');
            return;
        }

        // 尝试读取域
        fileDomain = null;
        if (file.name.toLowerCase().endsWith(".ota")) {
            try {
                const arrayBuffer = await readFileAsArrayBuffer(file);
                const zip = await JSZip.loadAsync(arrayBuffer);

                // 4. 查找指定的文件
                // zip.file() 支持相对路径，如 "folder/file.txt"
                const targetName = ".otaconfig";
                const zipEntry = zip.file(targetName);

                // console.log(666);
                if (!zipEntry) {
                    appendMessage(`错误: 在 ZIP 文件中未找到名为 "${targetName}" 的文件。\n可用文件列表:\n` + 
                                    zip.map(entry => entry.name).join('\n'), 'error');
                    return;
                }

                // 5. 异步读取文件内容为字符串
                const content = await zipEntry.async("string");
                try {
                    const jsonObj = JSON.parse(content);
                    appendMessage(
                        "\n软件名称：" + jsonObj.name[0] + 
                        "\n版本号：" + jsonObj.version[0] + 
                        "\n说明：\n" + (jsonObj.introduction?String(jsonObj.introduction).replace("<br>", "\n"):"无"), 
                        'info'
                    );
                    if (jsonObj.domain) {
                        fileDomain = jsonObj.domain;
                        if (!domains.includes(fileDomain))
                        {
                            appendMessage("错误：无法找到该文件对应的域：" + fileDomain, 'error');
                            return;
                        }
                        if (!String(fileDomain).startsWith("/"))
                        {
                            fileDomain = "/" + fileDomain;
                        }
                    }
                    else fileDomain = null;
                } catch (e) {
                    appendMessage(e, 'error');
                    return;
                }
            } catch (error) {
                appendMessage(error, 'error');
            }
        }


        selectedFile = file;
        fileNameSpan.textContent = file.name;
        selectedFileDiv.classList.add('show');
        uploadBtn.disabled = false;
    }

    // 追加消息到输出区域
    function appendMessage(text, type) {
        const line = document.createElement('div');
        if (type) {
            line.className = type;
        }
        const time = new Date().toLocaleTimeString();
        line.textContent = `[${time}] ${text}`;
        retMessageArea.appendChild(line);
        retMessageArea.scrollTop = retMessageArea.scrollHeight;
    }

    // 点击上传区域触发文件选择
    uploadArea.addEventListener('click', function () {
        fileInput.click();
    });

    // 文件选择变化
    fileInput.addEventListener('change', function (e) {
        const files = e.target.files;
        if (files && files.length > 0) {
            setFile(files[0]);
        }
    });

    // 拖拽事件
    uploadArea.addEventListener('dragover', function (e) {
        e.preventDefault();
        uploadArea.classList.add('dragover');
    });

    uploadArea.addEventListener('dragleave', function (e) {
        e.preventDefault();
        uploadArea.classList.remove('dragover');
    });

    uploadArea.addEventListener('drop', function (e) {
        e.preventDefault();
        uploadArea.classList.remove('dragover');
        const files = e.dataTransfer.files;
        if (files && files.length > 0) {
            setFile(files[0]);
        }
    });

    // 上传按钮点击
    uploadBtn.addEventListener('click', function () {
        if (!selectedFile) {
            appendMessage('请先选择文件', 'error');
            return;
        }

        uploadBtn.disabled = true;
        uploadBtn.textContent = '上传中...';
        appendMessage(`开始上传文件: ${selectedFile.name}`, 'info');

        const formData = new FormData();
        formData.append('file', selectedFile);

        fetch(route_prefix + (fileDomain?fileDomain:"") + '/api/uploadFile', {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                
                if (data.message) {
                    appendMessage(data.message, 'success');
                }
                if (selectedFile.name.toLowerCase().endsWith(".zip"))
                {
                    appendMessage('上传成功！2秒后跳转到软件版本控制页面', 'success');
                    setTimeout(() => {
                        window.location.href = route_prefix + "/softwares.html";
                    }, 2000);
                }
                else
                {
                    appendMessage('上传成功！2秒后跳转到升级包信息页面', 'success');
                    setTimeout(() => {
                        window.location.href = route_prefix + "/info.html?name=" + selectedFile.name + (fileDomain?("&domain=" + String(fileDomain).substring(1)):"");
                    }, 2000);
                }
                
            } else {
                appendMessage(data.message || '上传失败', 'error');
            }
        })
        .catch(error => {
            appendMessage('上传请求失败: ' + error.message, 'error');
        })
        .finally(() => {
            uploadBtn.disabled = false;
            uploadBtn.textContent = '开始上传';
        });

        
    });
})();
