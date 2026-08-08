(function () {
    'use strict';

    const infoShowArea = document.getElementById('info_show_area');
    const retMessageArea = document.getElementById('ret_message_area');
    const updateBtn = document.getElementById('updateBtn');
    const deleteBtn = document.getElementById('deleteBtn');
    const confirmModal = document.getElementById('confirmModal');
    const cancelBtn = document.getElementById('cancelBtn');
    const confirmBtn = document.getElementById('confirmBtn');
    const route_prefix = document.getElementById('route_prefix_info').innerText;

    // console.log(route_prefix);
    // const returnBtn = document.getElementById('returnBtn');

    // returnBtn.addEventListener('click', function () {
    //     console.log(111);
    //     window.location.href = route_prefix + '/ota_files.html';
    // });

    // 从 URL 获取文件名参数
    function getFileNameFromUrl() {
        const params = new URLSearchParams(window.location.search);
        return params.get('name') || '';
    }

    const fileName = getFileNameFromUrl();

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

    // HTML 转义
    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    // 渲染详细信息
    function renderInfo(data) {
        if (!data || typeof data !== 'object') {
            infoShowArea.innerHTML = '<div class="loading">暂无信息</div>';
            return;
        }

        let html = '';
        console.log(data);
        for (const key in data) {
            if (key === 'success') continue; // 跳过 success 字段
            const value = data[key];
            const displayValue = typeof value === 'object' ? JSON.stringify(value, null, 2) : String(value);
            html += '<div class="info-item">';
            html += '<div class="info-label">' + escapeHtml(key) + '</div>';
            html += '<div class="info-value">' + displayValue + '</div>';
            html += '</div>';
        }

        if (html === '') {
            html = '<div class="loading">暂无信息</div>';
        }

        infoShowArea.innerHTML = html;
    }

    // 获取文件详细信息
    function fetchInfo() {
        if (!fileName) {
            appendMessage('错误：未指定文件名', 'error');
            infoShowArea.innerHTML = '<div class="loading">未指定文件名</div>';
            return;
        }

        infoShowArea.innerHTML = '<div class="loading">加载中...</div>';
        appendMessage(`正在获取文件信息: ${fileName}`, 'info');

        fetch(route_prefix + '/api/info?name=' + encodeURIComponent(fileName))
            .then(function (response) {
                return response.json();
            })
            .then(function (data) {
                if (data.success) {
                    renderInfo(data);
                    appendMessage('获取文件信息成功', 'success');
                } else {
                    appendMessage(data.message || '获取文件信息失败', 'error');
                    infoShowArea.innerHTML = '<div class="loading">加载失败</div>';
                }
            })
            .catch(function (error) {
                appendMessage('请求失败: ' + error.message, 'error');
                infoShowArea.innerHTML = '<div class="loading">加载失败</div>';
            });
    }

    // 更新操作
    updateBtn.addEventListener('click', function () {
        if (!fileName) {
            appendMessage('错误：未指定文件名', 'error');
            return;
        }

        updateBtn.disabled = true;
        appendMessage(`正在更新文件: ${fileName}`, 'info');

        fetch(route_prefix + '/api/update?name=' + encodeURIComponent(fileName), {
            method: 'GET'
        })
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            if (data.success) {
                appendMessage('更新成功！', 'success');
                if (data.message) {
                    appendMessage(data.message, 'success');
                }
                // 更新成功后刷新信息
                fetchInfo();
            } else {
                appendMessage(data.message || '更新失败', 'error');
            }
        })
        .catch(function (error) {
            appendMessage('更新请求失败: ' + error.message, 'error');
        })
        .finally(function () {
            updateBtn.disabled = false;
        });
    });

    // 删除确认弹窗
    deleteBtn.addEventListener('click', function () {
        confirmModal.classList.add('show');
    });

    cancelBtn.addEventListener('click', function () {
        confirmModal.classList.remove('show');
    });

    confirmModal.addEventListener('click', function (e) {
        if (e.target === confirmModal) {
            confirmModal.classList.remove('show');
        }
    });

    // 确认删除
    confirmBtn.addEventListener('click', function () {
        if (!fileName) {
            appendMessage('错误：未指定文件名', 'error');
            confirmModal.classList.remove('show');
            return;
        }

        confirmModal.classList.remove('show');
        deleteBtn.disabled = true;
        appendMessage(`正在删除文件: ${fileName}`, 'info');

        fetch(route_prefix + '/api/deleteFile?name=' + encodeURIComponent(fileName), {
            method: 'GET'
        })
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            if (data.success) {
                appendMessage('删除成功！2秒后自动返回列表页面', 'success');
                if (data.message) {
                    appendMessage(data.message, 'success');
                }
                setTimeout(() => {window.location.href = route_prefix + "/ota_files.html";}, 2000);
                
            } else {
                appendMessage(data.message || '删除失败', 'error');
            }
        })
        .catch(function (error) {
            appendMessage('删除请求失败: ' + error.message, 'error');
        })
        .finally(function () {
            deleteBtn.disabled = false;
        });
    });

    // 页面加载时获取信息
    fetchInfo();
})();
