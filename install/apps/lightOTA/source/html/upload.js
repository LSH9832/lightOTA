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

    // console.log(route_prefix);

    let selectedFile = null;
    const allowedExtensions = ['.ota', '.zip'];

    // returnBtn.addEventListener('click', function () {
    //     // console.log(111);
    //     window.location.href = route_prefix + '/ota_files.html';
    // });

    // returnBtn.disabled = false;

    // 检查文件扩展名
    function isValidFile(file) {
        const name = file.name.toLowerCase();
        return allowedExtensions.some(ext => name.endsWith(ext));
    }

    // 设置选中的文件
    function setFile(file) {
        if (!isValidFile(file)) {
            appendMessage('错误：仅支持 .ota 或 .zip 格式的文件', 'error');
            return;
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

        fetch(route_prefix + '/api/uploadFile', {
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
                        window.location.href =  route_prefix + "/softwares.html";
                    }, 2000);
                }
                else
                {
                    appendMessage('上传成功！2秒后跳转到升级包信息页面', 'success');
                    setTimeout(() => {
                        window.location.href = route_prefix + "/info.html?name=" + selectedFile.name;
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
