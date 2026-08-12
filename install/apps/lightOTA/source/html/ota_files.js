(function () {
    'use strict';

    const tableContainer = document.getElementById('tableContainer');
    const errorModal = document.getElementById('errorModal');
    const errorMessage = document.getElementById('errorMessage');
    const closeModalBtn = document.getElementById('closeModalBtn');
    const route_prefix = document.getElementById('route_prefix_info').innerText;

    const domainDiv = document.getElementById("domains");
    let domains = [""];
    if (domainDiv){
        domains = domainDiv.innerText.split(":");
    }

    // console.log(route_prefix);
    // const upload_button = document.getElementById("upload");
    // const softlist_btn = document.getElementById("softlist");
    

    // softlist_btn.addEventListener('click', function () {
    //     window.location.href = route_prefix + '/softwares.html';
    // });
    // upload_button.addEventListener('click', function () {
    //     window.location.href = route_prefix + '/upload.html';
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
    function renderTable(fileList, domain="", idx=0) {
        if (!fileList || fileList.length === 0 && idx == 0) {
            tableContainer.innerHTML = '<div class="loading">暂无文件</div>';
            return 0;
        }

        let html = '';
        let count = 0;
        if (idx == 0) {
            html += '<table><thead><tr>';
            html += '<th class="col-index">序号</th>';
            html += '<th>文件名称</th>';
            html += '<th class="col-action">操作</th>';
            html += '</tr></thead><tbody>';
        }
        else {
            html = tableContainer.innerHTML.split("</tbody></table>")[0];
        }
        
        fileList.forEach(function (fileName, index) {
            html += '<tr>';
            html += '<td class="col-index">' + (index + idx + 1) + '</td>';
            html += '<td>' + escapeHtml(fileName) + '</td>';
            html += '<td class="col-action">';
            html += '<button class="btn btn-primary btn-small" data-name="' + 
                        encodeURIComponent(fileName) + '"' +
                        (domain.length?('domain-name="' + domain + '"'):'') + '>查看详细信息</button>';
            html += '</td>';
            html += '</tr>';
            count += 1;
        });
        idx += count;
        html += '</tbody></table>';
        tableContainer.innerHTML = html;

        // 绑定按钮事件
        const buttons = tableContainer.querySelectorAll('button[data-name]');
        buttons.forEach(function (btn) {
            btn.addEventListener('click', function () {
                const name = decodeURIComponent(btn.getAttribute('data-name'));
                const domain = btn.getAttribute('domain-name')?decodeURIComponent(btn.getAttribute('domain-name')):null;
                window.location.href = route_prefix + '/info.html?name=' + encodeURIComponent(name) + (domain?('&domain='+encodeURIComponent(domain)):'');
            });
        });

        return idx;
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
        let cur_idx = 0;
        for (const domain of domains) {
            fetch(route_prefix + ((domain.startsWith("/") || domain.length==0)?"":"/") + domain + '/api/getOTAFileList')
                .then(function (response) {
                    return response.json();
                })
                .then(function (data) {
                    if (data.success) {
                        cur_idx = renderTable(data.value || [], domain, cur_idx);
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
        
    }

    // 页面加载时获取列表
    fetchFileList();
})();
