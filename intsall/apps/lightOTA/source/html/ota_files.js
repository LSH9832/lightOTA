(function () {
    'use strict';

    const tableContainer = document.getElementById('tableContainer');
    const errorModal = document.getElementById('errorModal');
    const errorMessage = document.getElementById('errorMessage');
    const closeModalBtn = document.getElementById('closeModalBtn');
    // const upload_button = document.getElementById("upload");
    // const softlist_btn = document.getElementById("softlist");
    

    // softlist_btn.addEventListener('click', function () {
    //     window.location.href = '/ota/softwares.html';
    // });
    // upload_button.addEventListener('click', function () {
    //     window.location.href = '/ota/upload.html';
    // });
    // 显示错误弹窗
    function showError(message) {
        errorMessage.textContent = message;
        errorModal.classList.add('show');
    }

    // 关闭弹窗
    closeModalBtn.addEventListener('click', function () {
        errorModal.classList.remove('show');
    });

    errorModal.addEventListener('click', function (e) {
        if (e.target === errorModal) {
            errorModal.classList.remove('show');
        }
    });

    // 渲染表格
    function renderTable(fileList) {
        if (!fileList || fileList.length === 0) {
            tableContainer.innerHTML = '<div class="loading">暂无文件</div>';
            return;
        }

        let html = '<table><thead><tr>';
        html += '<th class="col-index">序号</th>';
        html += '<th>文件名称</th>';
        html += '<th class="col-action">操作</th>';
        html += '</tr></thead><tbody>';

        fileList.forEach(function (fileName, index) {
            html += '<tr>';
            html += '<td class="col-index">' + (index + 1) + '</td>';
            html += '<td>' + escapeHtml(fileName) + '</td>';
            html += '<td class="col-action">';
            html += '<button class="btn btn-primary btn-small" data-name="' + encodeURIComponent(fileName) + '">查看详细信息</button>';
            html += '</td>';
            html += '</tr>';
        });

        html += '</tbody></table>';
        tableContainer.innerHTML = html;

        // 绑定按钮事件
        const buttons = tableContainer.querySelectorAll('button[data-name]');
        buttons.forEach(function (btn) {
            btn.addEventListener('click', function () {
                const name = decodeURIComponent(btn.getAttribute('data-name'));
                window.location.href = '/ota/info.html?name=' + encodeURIComponent(name);
            });
        });

        
    }

    // HTML 转义
    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    // 获取文件列表
    function fetchFileList() {
        tableContainer.innerHTML = '<div class="loading">加载中...</div>';

        fetch('/ota/api/getOTAFileList')
            .then(function (response) {
                return response.json();
            })
            .then(function (data) {
                if (data.success) {
                    renderTable(data.value || []);
                } else {
                    showError(data.message || '获取文件列表失败');
                    tableContainer.innerHTML = '<div class="loading">加载失败</div>';
                }
            })
            .catch(function (error) {
                showError('请求失败: ' + error.message);
                tableContainer.innerHTML = '<div class="loading">加载失败</div>';
            });
    }

    // 页面加载时获取列表
    fetchFileList();
})();
