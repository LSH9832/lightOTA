(function () {
    'use strict';
    const retMessageArea = document.getElementById('ret_message_area');
    const tableBody = document.getElementById('tableBody');
    const saveAllBtn = document.getElementById('saveAllBtn');
    // const uploadBtn = document.getElementById('uploadBtn');
    const route_prefix = document.getElementById('route_prefix_info').innerText;

    // console.log(route_prefix);

    let softwareListData = []; // 存储所有软件完整信息，保存时遍历使用

    // uploadBtn.addEventListener('click', function () {
    //     window.location.href = route_prefix + "/upload.html";
    // });

    /**
     * 追加日志消息（复用项目原有逻辑）
     * @param {string} text 内容
     * @param {string} type error / success / info
     */
    function appendMessage(text, type) {
        if (!retMessageArea) return;
        const line = document.createElement('div');
        if (type) line.className = type;
        const time = new Date().toLocaleTimeString();
        line.textContent = `[${time}] ${text}`;
        retMessageArea.appendChild(line);
        retMessageArea.scrollTop = retMessageArea.scrollHeight;
    }

    /** HTML转义防XSS */
    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    /**
     * 版本号解析：v1.2.3 / v1.2 / 1.2.3 / 1.2 => [major, minor, patch]
     */
    function parseVersion(verStr) {
        let clean = String(verStr).replace(/^v/i, "");
        const arr = clean.split(".").map(s => parseInt(s || 0, 10));
        while (arr.length < 3) arr.push(0);
        return arr;
    }

    /**
     * 版本对比
     * @return 1目标更高 | 0相等 | -1目标更低
     */
    function compareVersion(currVer, targetVer) {
        const curr = parseVersion(currVer);
        const target = parseVersion(targetVer);
        for (let i = 0; i < 3; i++) {
            if (target[i] > curr[i]) return 1;
            if (target[i] < curr[i]) return -1;
        }
        return 0;
    }

    /**
     * 渲染软件表格，第四列改为下拉选择框
     * @param {Object} softMap 接口返回value字典
     */
    function renderTable(softMap) {
        tableBody.innerHTML = "";
        softwareListData = [];
        let index = 1;
        for (const softName in softMap) {
            const item = softMap[softName];
            const currVer = item.current_version;
            const supportArr = item.support_version;
            softwareListData.push({
                name: softName,
                current_version: currVer,
                support_version: supportArr
            });

            const tr = document.createElement('tr');

            // 序号
            const tdIdx = document.createElement('td');
            tdIdx.className = "col-index";
            tdIdx.textContent = index++;

            // 软件名
            const tdName = document.createElement('td');
            tdName.innerHTML = escapeHtml(softName);

            // 当前版本
            const tdCurr = document.createElement('td');
            tdCurr.innerHTML = escapeHtml(currVer);

            // 版本号设置：下拉框
            const tdSelect = document.createElement('td');
            const selectEl = document.createElement('select');
            selectEl.style.minWidth = "90px";
            selectEl.dataset.softName = softName;
            const opt_no_update = document.createElement('option');
            opt_no_update.textContent = "<不更改>";
            opt_no_update.selected = true;
            selectEl.appendChild(opt_no_update);
            
            // 填充下拉选项
            for (let i = 0; i < supportArr.length; i += 2) {
                const verText = supportArr[i];
                const fileName = supportArr[i + 1];
                const opt = document.createElement('option');
                opt.value = fileName;
                opt.textContent = verText;
                // 默认选中当前版本
                // if (verText === currVer) opt.selected = true;
                selectEl.appendChild(opt);
            }
            tdSelect.appendChild(selectEl);

            tr.appendChild(tdIdx);
            tr.appendChild(tdName);
            tr.appendChild(tdCurr);
            tr.appendChild(tdSelect);
            tableBody.appendChild(tr);
        }
    }

    /**
     * 延时工具
     * @param {number} ms 毫秒
     */
    function sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    function check_update_finished() {
        return new Promise()
    }

    

    /**
     * 批量保存逻辑：遍历所有下拉框，变更则发起请求，请求间隔100ms(0.1s)，非阻塞
     */
    saveAllBtn.addEventListener('click', async () => {
        saveAllBtn.disabled = true;
        // appendMessage("开始校验并批量下发版本更新请求", "info");
        const selects = tableBody.querySelectorAll('select');
        // 存放所有更新请求Promise
        const requestPromiseList = [];

        for (const sel of selects) {
            if (sel.selectedIndex == 0) continue;
            const softName = sel.dataset.softName;
            const selectedFileName = sel.value;
            
            const softInfo = softwareListData.find(s => s.name === softName);
            let selectedVer = "";
            const supportArr = softInfo.support_version;
            for(let i=1;i<supportArr.length;i+=2){
                if(supportArr[i] === selectedFileName){
                    selectedVer = supportArr[i-1];
                    break;
                }
            }
            if(selectedVer === softInfo.current_version){
                appendMessage(`${softName}：选择版本与当前版本相同，将覆写`,"warning");
                // continue;
            }
            else
                appendMessage(`${softName}：版本由${softInfo.current_version}改为${selectedVer}，开始更新`,"info");
            
            // 封装单个请求Promise，存入数组
            const singleReqPromise = (async () => {
                try {
                    const res = await fetch(route_prefix + `/api/update?name=${encodeURIComponent(selectedFileName)}`);
                    const data = await res.json();
                    if(data.success){
                        appendMessage(`${softName}：更新成功`,"success");
                    }else{
                        appendMessage(`${softName}：更新失败：${data.message}`,"error");
                    }
                }catch(err){
                    appendMessage(`${softName}：请求网络异常：${err.message}`,"error");
                }
            })();
            requestPromiseList.push(singleReqPromise);

            // 间隔0.1s再处理下一个软件
            await sleep(100);
            // num2update += 1;
        }

        if (requestPromiseList.length > 0)
        {
             // 等待所有接口请求全部拿到返回结果
            await Promise.all(requestPromiseList);
            // appendMessage("所有更新请求全部处理完成", "success");
            loadSoftwareList(false);
        }
        else
        {
            appendMessage(`没有需要更新的软件`,"warning");
        }

       
        saveAllBtn.disabled = false;
    });


    /** 加载软件列表 */
    async function loadSoftwareList(first = true) {
        tableBody.innerHTML = '<tr><td colspan="4" class="loading">加载中...</td>';
        if (first) appendMessage("正在获取已安装软件列表", "info");
        else appendMessage("刷新软件列表", "info");
        try {
            const resp = await fetch(route_prefix + "/api/getSoftwareList");
            const data = await resp.json();
            if (!data.success) {
                appendMessage(`获取列表失败: ${data.message || "服务返回异常"}`, "error");
                tableBody.innerHTML = '<tr><td colspan="4" class="loading">加载失败</td>';
                return;
            }
            if (first) appendMessage("软件列表加载完成，可在下拉框选择目标版本后点击保存更改", "success");
            renderTable(data.value || {});
        } catch (err) {
            appendMessage(`网络请求失败: ${err.message}`, "error");
            tableBody.innerHTML = '<tr><td colspan="4" class="loading">网络异常</td>';
        }
    }

    // 页面加载执行
    loadSoftwareList();
})();
