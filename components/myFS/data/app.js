// ==================== 全局配置 ====================
/**
 * 应用全局配置
 * @property {number} MAX_DATA_POINTS - 图表最大数据点数
 * @property {number} WS_RECONNECT_DELAY - WebSocket重连延迟（毫秒）
 * @property {number} CHART_UPDATE_THROTTLE - 图表更新节流时间（毫秒）
 * @property {Object} API_ENDPOINTS - API端点配置
 */
const CONFIG = {
    MAX_DATA_POINTS: 100,
    WS_RECONNECT_DELAY: 2000,
    CHART_UPDATE_THROTTLE: 100, // ms
    API_ENDPOINTS: {
        POWER_CONTROL: '/api/power-control',
        PROTECTION_CONFIG: '/api/protection-config',
        SYSTEM_STATUS: '/api/system-status',
        PROTECTION_STATUS: '/api/protection-status',
        FLTTMR_STATUS: '/api/flttmr-status',
        FLTTMR_CONFIG: '/api/flttmr-config',
        SSSR_STATUS: '/api/sssr-status',
        SSSR_CONFIG: '/api/sssr-config'
    }
};

// ==================== 常量映射表 ====================
const OVP_OPTIONS = [
    {code:'00000', val:'16V'},
    {code:'00001', val:'18V'},
    {code:'00010', val:'20V'},
    {code:'00011', val:'22V'},
    {code:'00100', val:'24V'},
    {code:'00101', val:'26V'},
    {code:'00110', val:'28V'},
    {code:'00111', val:'30V'},
    {code:'01000', val:'32V'},
    {code:'01001', val:'34V'},
    {code:'01010', val:'36V'},
    {code:'01011', val:'38V'},
    {code:'01100', val:'40V'},
    {code:'01101', val:'42V (Default)'}
];

const FLTTMR_OPTIONS = [
    {code:'000', val:'0ms'},
    {code:'001', val:'0.2ms'},
    {code:'010', val:'0.4ms'},
    {code:'011', val:'1ms'},
    {code:'100', val:'2ms (Default)'},
    {code:'101', val:'8ms'},
    {code:'110', val:'32ms'},
    {code:'111', val:'128ms'}
];

const SSSR_OPTIONS = [
    {code:'000', val:'0.25V/ms（默认）'},
    {code:'001', val:'0.5V/ms'},
    {code:'010', val:'1V/ms'},
    {code:'011', val:'4V/ms'},
    {code:'100', val:'16V/ms'},
    {code:'101', val:'32V/ms'},
    {code:'110', val:'64V/ms'},
    {code:'111', val:'无软启动（直接开启）'}
];

const OVP_VALUES = [16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42];

// ==================== 工具函数 ====================
const Utils = {
    /**
     * 安全的JSON解析
     * @param {string} jsonString - 要解析的JSON字符串
     * @param {any} defaultValue - 解析失败时的默认值
     * @returns {any} 解析结果或默认值
     */
    safeJSONParse(jsonString, defaultValue = null) {
        try {
            return JSON.parse(jsonString);
        } catch (e) {
            console.error('JSON解析失败:', e);
            return defaultValue;
        }
    },

    /**
     * 防抖函数
     * @param {Function} func - 要防抖的函数
     * @param {number} wait - 等待时间（毫秒）
     * @returns {Function} 防抖后的函数
     */
    debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    },

    /**
     * 节流函数
     * @param {Function} func - 要节流的函数
     * @param {number} limit - 限制时间（毫秒）
     * @returns {Function} 节流后的函数
     */
    throttle(func, limit) {
        let inThrottle;
        return function executedFunction(...args) {
            if (!inThrottle) {
                func(...args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, limit);
            }
        };
    },

    /**
     * 安全的DOM元素选择器
     * @param {string} selector - CSS选择器或ID（以#开头）
     * @returns {Element|null} DOM元素或null
     */
    safeSelector(selector) {
        try {
            if (selector.startsWith('#')) {
                return document.getElementById(selector.slice(1));
            }
            return document.querySelector(selector);
        } catch (e) {
            console.error(`选择器错误 "${selector}":`, e);
            return null;
        }
    },

    /**
     * 安全的DOM元素ID选择器
     * @param {string} id - 元素ID
     * @returns {Element|null} DOM元素或null
     */
    safeGetElementById(id) {
        return this.safeSelector(`#${id}`);
    }
};

// ==================== 数据管理 ====================
class DataManager {
    constructor(maxPoints = CONFIG.MAX_DATA_POINTS) {
        this.maxPoints = maxPoints;
        this.data = new Array(maxPoints).fill(0);
        this.labels = new Array(maxPoints).fill(0).map((_, i) => i);
        this.currentIndex = 0;
        this.isFull = false;
    }

    /**
     * 添加新数据点
     * @param {number} value - 数据值
     * @param {number} timestamp - 时间戳
     */
    addPoint(value, timestamp) {
        this.data[this.currentIndex] = value;
        this.labels[this.currentIndex] = timestamp;
        
        this.currentIndex = (this.currentIndex + 1) % this.maxPoints;
        if (this.currentIndex === 0) {
            this.isFull = true;
        }
    }

    /**
     * 获取当前数据数组（按时间顺序）
     * @returns {Array} 数据数组
     */
    getData() {
        if (!this.isFull) {
            return this.data.slice(0, this.currentIndex);
        }
        
        const result = new Array(this.maxPoints);
        for (let i = 0; i < this.maxPoints; i++) {
            const index = (this.currentIndex + i) % this.maxPoints;
            result[i] = this.data[index];
        }
        return result;
    }

    /**
     * 获取当前标签数组（按时间顺序）
     * @returns {Array} 标签数组
     */
    getLabels() {
        if (!this.isFull) {
            return this.labels.slice(0, this.currentIndex);
        }
        
        const result = new Array(this.maxPoints);
        for (let i = 0; i < this.maxPoints; i++) {
            const index = (this.currentIndex + i) % this.maxPoints;
            result[i] = this.labels[index];
        }
        return result;
    }

    /**
     * 获取数据范围
     * @returns {Array} [min, max]
     */
    getRange() {
        const data = this.getData();
        if (!data.length) return [0, 10];
        
        let min = Math.min(...data);
        let max = Math.max(...data);

        // 下限不低于 0
        min = Math.max(0, min);

        // 量程不低于 10 W
        if (max - min < 10) {
            const center = (min + max) / 2;
            min = center - 5;
            max = center + 5;
        }

        // 保证 min 不飘到负数
        if (min < 0) {
            max += -min;
            min = 0;
        }

        // 5% 视觉边距
        const margin = (max - min) * 0.05 || 0.5;
        return [min - margin, max + margin];
    }
}

// ==================== 图表绘制 ====================
class PowerChart {
    constructor(canvasId) {
        this.canvas = Utils.safeGetElementById(canvasId);
        if (!this.canvas) {
            throw new Error(`无法找到画布元素: ${canvasId}`);
        }
        
        this.ctx = this.canvas.getContext('2d');
        this.padding = 40;
        this.dataManager = new DataManager();
        
        this.setupCanvas();
        this.bindEvents();
    }

    setupCanvas() {
        this.resizeCanvas();
        window.addEventListener('resize', Utils.debounce(() => this.resizeCanvas(), 250));
    }

    resizeCanvas() {
        const rect = this.canvas.getBoundingClientRect();
        this.canvas.width = rect.width;
        this.canvas.height = rect.height;
        this.chartWidth = this.canvas.width;
        this.chartHeight = this.canvas.height;
        this.plotWidth = this.chartWidth - 2 * this.padding;
        this.plotHeight = this.chartHeight - 2 * this.padding;
        this.draw();
    }

    bindEvents() {
        // 可以添加鼠标交互等事件
    }

    updateData(power, timestamp) {
        this.dataManager.addPoint(power, timestamp);
        this.drawThrottled();
    }

    drawThrottled = Utils.throttle(() => this.draw(), CONFIG.CHART_UPDATE_THROTTLE);

    draw() {
        if (!this.ctx) return;
        
        this.ctx.clearRect(0, 0, this.chartWidth, this.chartHeight);
        
        const [rangeMin, rangeMax] = this.dataManager.getRange();
        const range = rangeMax - rangeMin;
        
        // 防止除零错误
        if (range === 0) return;
        
        this.drawGrid();
        this.drawAxes();
        this.drawLabels(rangeMin, rangeMax);
        this.drawDataLine(rangeMin, rangeMax);
        this.drawCurrentValue();
    }

    drawGrid() {
        this.ctx.strokeStyle = 'rgba(255,255,255,0.1)';
        this.ctx.lineWidth = 1;
        
        // 横向网格 - 5条
        for (let i = 0; i <= 5; i++) {
            const y = this.padding + (this.plotHeight / 5) * i;
            this.ctx.beginPath();
            this.ctx.moveTo(this.padding, y);
            this.ctx.lineTo(this.chartWidth - this.padding, y);
            this.ctx.stroke();
        }
        
        // 垂直网格 - 10条
        for (let i = 0; i <= 10; i++) {
            const x = this.padding + (this.plotWidth / 10) * i;
            this.ctx.beginPath();
            this.ctx.moveTo(x, this.padding);
            this.ctx.lineTo(x, this.chartHeight - this.padding);
            this.ctx.stroke();
        }
    }

    drawAxes() {
        this.ctx.strokeStyle = 'rgba(255,255,255,0.3)';
        this.ctx.lineWidth = 2;
        
        this.ctx.beginPath();
        this.ctx.moveTo(this.padding, this.padding);
        this.ctx.lineTo(this.padding, this.chartHeight - this.padding);
        this.ctx.moveTo(this.padding, this.chartHeight - this.padding);
        this.ctx.lineTo(this.chartWidth - this.padding, this.chartHeight - this.padding);
        this.ctx.stroke();
    }

    drawLabels(rangeMin, rangeMax) {
        this.ctx.fillStyle = '#e2e8f0';
        this.ctx.font = '12px Arial';
        this.ctx.textAlign = 'right';
        
        const range = rangeMax - rangeMin;
        for (let i = 0; i <= 5; i++) {
            const value = rangeMax - (range / 5) * i;
            const y = this.padding + (this.plotHeight / 5) * i;
            this.ctx.fillText(value.toFixed(0) + 'W', this.padding - 10, y + 4);
        }
    }

    drawDataLine(rangeMin, rangeMax) {
        const data = this.dataManager.getData();
        if (!data.length) return;
        
        const range = rangeMax - rangeMin;
        this.ctx.strokeStyle = '#38bdf8';
        this.ctx.lineWidth = 3;
        this.ctx.shadowBlur = 15;
        this.ctx.shadowColor = 'rgba(56,189,248,0.5)';
        
        this.ctx.beginPath();
        const stepX = this.plotWidth / (CONFIG.MAX_DATA_POINTS - 1);
        
        for (let i = 0; i < data.length; i++) {
            const x = this.padding + i * stepX;
            const y = this.chartHeight - this.padding - ((data[i] - rangeMin) / range) * this.plotHeight;
            i === 0 ? this.ctx.moveTo(x, y) : this.ctx.lineTo(x, y);
        }
        
        this.ctx.stroke();
        this.ctx.shadowBlur = 0;
    }

    drawCurrentValue() {
        const data = this.dataManager.getData();
        if (!data.length) return;
        
        const current = data[data.length - 1];
        this.ctx.fillStyle = '#f8fafc';
        this.ctx.font = 'bold 16px Arial';
        this.ctx.textAlign = 'left';
        this.ctx.fillText(current.toFixed(1) + 'W', this.chartWidth - this.padding - 50, this.padding + 20);
    }
}

// ==================== WebSocket管理 ====================
class WebSocketManager {
    constructor(onMessageCallback, uiManager = null) {
        this.ws = null;
        this.onMessageCallback = onMessageCallback;
        this.uiManager = uiManager;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 10;
        this.connect();
    }

    connect() {
        try {
            this.updateConnectionStatus('connecting', '连接中...');
            this.ws = new WebSocket(`ws://${location.host}/ws`);
            this.setupEventHandlers();
        } catch (e) {
            console.error('WebSocket连接失败:', e);
            this.updateConnectionStatus('', '连接失败');
            this.scheduleReconnect();
        }
    }

    setupEventHandlers() {
        this.ws.onopen = () => {
            console.log('WS 已连接');
            this.reconnectAttempts = 0;
            this.updateConnectionStatus('connected', '已连接');
        };

        this.ws.onerror = (err) => {
            console.error('WebSocket错误:', err);
            this.updateConnectionStatus('', '连接错误');
        };

        this.ws.onmessage = (event) => {
            const data = Utils.safeJSONParse(event.data);
            if (data && this.onMessageCallback) {
                this.onMessageCallback(data);
            }
        };

        this.ws.onclose = () => {
            console.log('WS 连接关闭，尝试重连...');
            this.updateConnectionStatus('', '连接断开');
            this.scheduleReconnect();
        };
    }

    updateConnectionStatus(status, text) {
        if (this.uiManager && this.uiManager.updateConnectionStatus) {
            this.uiManager.updateConnectionStatus(status, text);
        }
    }

    scheduleReconnect() {
        if (this.reconnectAttempts >= this.maxReconnectAttempts) {
            console.error('达到最大重连次数，停止重连');
            this.updateConnectionStatus('', '连接失败');
            return;
        }
        
        this.reconnectAttempts++;
        const delay = CONFIG.WS_RECONNECT_DELAY * Math.min(this.reconnectAttempts, 5); // 指数退避
        console.log(`${delay/1000}秒后尝试重连...`);
        this.updateConnectionStatus('connecting', `${delay/1000}秒后重连...`);
        
        setTimeout(() => this.connect(), delay);
    }

    close() {
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
    }
}

// ==================== API通信管理 ====================
class APIManager {
    /**
     * 通用API请求方法
     * @param {string} url - API端点
     * @param {Object} options - 请求选项
     * @returns {Promise} 请求Promise
     */
    static async request(url, options = {}) {
        const defaultOptions = {
            method: 'GET',
            headers: { 'Content-Type': 'application/json' }
        };
        
        const finalOptions = { ...defaultOptions, ...options };
        
        try {
            const response = await fetch(url, finalOptions);
            if (!response.ok) {
                throw new Error(`HTTP错误: ${response.status}`);
            }
            return await response.json();
        } catch (error) {
            console.error(`API请求失败 (${url}):`, error);
            throw error;
        }
    }

    /**
     * POST请求
     * @param {string} url - API端点
     * @param {Object} data - 请求数据
     * @returns {Promise} 请求Promise
     */
    static async post(url, data) {
        return this.request(url, {
            method: 'POST',
            body: JSON.stringify(data)
        });
    }

    /**
     * GET请求
     * @param {string} url - API端点
     * @returns {Promise} 请求Promise
     */
    static async get(url) {
        return this.request(url);
    }
}

// ==================== UI状态管理 ====================
class UIManager {
    constructor() {
        this.elements = {};
        this.init();
    }

    async init() {
        // 等待DOM完全加载
        if (document.readyState === 'loading') {
            await new Promise(resolve => {
                document.addEventListener('DOMContentLoaded', resolve);
            });
        }
        
        this.cacheElements();
        this.bindEvents();
        this.hideLoadingOverlay();
        this.initConnectionStatus();
        
        console.log('UI管理器初始化完成，连接状态元素:', {
            connectionStatus: this.elements.connectionStatus,
            statusIndicator: this.elements.statusIndicator,
            statusText: this.elements.statusText
        });
    }

    cacheElements() {
        // 缓存常用DOM元素
        this.elements = {
            voltage: Utils.safeGetElementById('voltage'),
            current: Utils.safeGetElementById('current'),
            power: Utils.safeGetElementById('power'),
            powerSwitch: Utils.safeGetElementById('powerSwitch'),
            statusItems: {
                enable: Utils.safeGetElementById('enable'),
                ocp: Utils.safeGetElementById('ocp'),
                normal: Utils.safeGetElementById('normal'),
                ohp: Utils.safeGetElementById('ohp')
                // 注意：uvp, ovp, fault 状态指示器在HTML中不存在
            },
            connectionStatus: Utils.safeGetElementById('connectionStatus'),
            statusIndicator: Utils.safeGetElementById('statusIndicator'),
            statusText: Utils.safeGetElementById('statusText'),
            loadingOverlay: Utils.safeGetElementById('loadingOverlay'),
            toastContainer: Utils.safeGetElementById('toastContainer')
        };

        // 检查关键元素是否存在
        if (!this.elements.statusIndicator) {
            console.error('无法找到状态指示器元素: statusIndicator');
        }
        if (!this.elements.statusText) {
            console.error('无法找到状态文本元素: statusText');
        }
    }

    hideLoadingOverlay() {
        if (this.elements.loadingOverlay) {
            this.elements.loadingOverlay.style.display = 'none';
        }
    }

    initConnectionStatus() {
        this.updateConnectionStatus('connecting', '连接中...');
    }

    updateConnectionStatus(status, text) {
        console.log(`UI管理器更新连接状态: ${status} - ${text}`);
        
        if (!this.elements.statusIndicator || !this.elements.statusText) {
            console.error('连接状态元素未找到，无法更新状态');
            return;
        }
        
        // 确保状态类名有效
        const validStatuses = ['', 'connecting', 'connected'];
        if (!validStatuses.includes(status)) {
            console.warn(`无效的状态类名: ${status}，使用空字符串`);
            status = '';
        }
        
        try {
            this.elements.statusIndicator.className = `status-indicator ${status}`;
            this.elements.statusText.textContent = text;
            console.log(`连接状态更新成功: ${status} - ${text}`);
        } catch (error) {
            console.error('更新连接状态时出错:', error);
        }
    }

    showToast(message, type = 'info', duration = 3000) {
        if (!this.elements.toastContainer) return;
        
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        toast.textContent = message;
        
        this.elements.toastContainer.appendChild(toast);
        
        // 自动移除Toast
        setTimeout(() => {
            if (toast.parentNode) {
                toast.parentNode.removeChild(toast);
            }
        }, duration);
    }

    bindEvents() {
        // 电源开关事件
        if (this.elements.powerSwitch) {
            this.elements.powerSwitch.addEventListener('change', (e) => this.handlePowerSwitch(e));
        }

        // 保护配置卡片事件
        this.bindProtectionCardEvents();
        this.bindSliderEvents();
    }


    bindProtectionCardEvents() {
        document.querySelectorAll('.ctrl-card').forEach(card => {
            this.bindCardButtonEvents(card, this.sendProtectionConfig.bind(this));
        });
    }

    bindCardButtonEvents(card, sendCallback) {
        // 范围按钮
        card.querySelectorAll('.range-btn').forEach(btn => {
            btn.addEventListener('click', function () {
                card.querySelectorAll('.range-btn').forEach(b => b.classList.remove('active'));
                this.classList.add('active');
                sendCallback(card);
            });
        });

        // 动作按钮
        card.querySelectorAll('.act-btn').forEach(btn => {
            btn.addEventListener('click', function () {
                card.querySelectorAll('.act-btn').forEach(b => b.classList.remove('active'));
                this.classList.add('active');
                sendCallback(card);
            });
        });
    }

    updateOVP() {
        const slider = Utils.safeGetElementById('ovpSlider');
        const codeElement = Utils.safeGetElementById('ovpCode');
        const slopeElement = Utils.safeGetElementById('ovpSlope');

        if (!slider || !codeElement || !slopeElement) return;

        const idx = parseInt(slider.value, 10);
        codeElement.textContent = `OVP = ${OVP_OPTIONS[idx].val}`;
        slopeElement.textContent = '过压保护阈值';
        
    }

    bindSliderEvents() {
        // FLTTMR滑块
        const flttmrSlider = Utils.safeGetElementById('flttmrSlider');
        if (flttmrSlider) {
            flttmrSlider.addEventListener('input', () => this.updateFLTTMR());
            this.updateFLTTMR(); // 初始化
        }

        // SSSR滑块
        const sssrSlider = Utils.safeGetElementById('sssrSlider');
        if (sssrSlider) {
            sssrSlider.addEventListener('input', () => this.updateSSSR());
            this.updateSSSR(); // 初始化
        }

        // OVP滑块
        const ovpSlider = Utils.safeGetElementById('ovpSlider');
        if (ovpSlider) {
            ovpSlider.addEventListener('input', () => this.updateOVP());
            this.updateOVP(); // 初始化
        }
    }

    async handlePowerSwitch(event) {
        const want = event.target.checked;
        try {
            const result = await APIManager.post(CONFIG.API_ENDPOINTS.POWER_CONTROL, { enabled: want });
            const message = result.enabled ? '电源已开启' : '电源已关闭';
            console.log(message);
            this.showToast(message, 'success');
            this.setStatus('enable', result.enabled);
        } catch (error) {
            console.error('电源控制失败:', error);
            this.showToast('电源控制失败', 'error');
            event.target.checked = !want; // 回滚状态
        }
    }

    async sendProtectionConfig(card) {
        const channel = parseInt(card.dataset.channel, 10);
        let threshold, action;

        // 处理不同类型的卡片
        if (card.dataset.channel === '3') {
            // OVP滑块使用新的逻辑
            const ovpSlider = card.querySelector('#ovpSlider');
            if (ovpSlider) {
                const idx = parseInt(ovpSlider.value, 10);
                threshold = idx;
            }
        } else if (card.dataset.channel === '0'){
            threshold = parseInt(0, 10);
        } else {
            const activeRangeBtn = card.querySelector('.range-btn.active');
            threshold = parseInt(activeRangeBtn.dataset.act, 10);
        }

        const activeActBtn = card.querySelector('.act-btn.active');
        action = parseInt(activeActBtn.dataset.act, 10);

        try {
            const result = await APIManager.post(CONFIG.API_ENDPOINTS.PROTECTION_CONFIG, {
                channel,
                threshold,
                action
            });
            
            let message = `通道 ${channel} 保护设置已保存`;
            console.log(message, result);
            this.showToast(message, 'success');
            
            // 修复：如果点击的是"加载默认值"按钮，需要同步最新配置
            if (channel === 0 && action === 0) {
                console.log('检测到加载默认值操作，正在同步最新配置...');
                message = '已加载默认配置，正在同步UI...';
                this.showToast(message, 'info');
                
                // 延迟一下，让硬件完成配置加载
                setTimeout(async () => {
                    try {
                        console.log('开始同步最新配置到UI...');
                        
                        // 重新加载保护配置
                        const protectionStatus = await APIManager.get(CONFIG.API_ENDPOINTS.PROTECTION_STATUS);
                        console.log('重新加载保护配置:', protectionStatus);
                        this.applyProtectionConfig(protectionStatus);
                        
                        // 重新加载FLTTMR状态
                        const flttmrStatus = await APIManager.get(CONFIG.API_ENDPOINTS.FLTTMR_STATUS);
                        console.log('重新加载FLTTMR状态:', flttmrStatus);
                        this.applyFLTTMRConfig(flttmrStatus);
                        
                        // 重新加载SSSR状态
                        const sssrStatus = await APIManager.get(CONFIG.API_ENDPOINTS.SSSR_STATUS);
                        console.log('重新加载SSSR状态:', sssrStatus);
                        this.applySSSRConfig(sssrStatus);
                        
                        console.log('默认配置同步完成');
                        this.showToast('默认配置已同步到UI', 'success');
                        
                    } catch (syncError) {
                        console.error('同步默认配置失败:', syncError);
                        this.showToast('同步默认配置失败', 'error');
                    }
                }, 500); // 500ms延迟，确保硬件完成配置
            }
            
        } catch (error) {
            const errorMessage = `通道 ${channel} 保护设置保存失败`;
            console.error(errorMessage, error);
            this.showToast(errorMessage, 'error');
        }
    }

    updateSensorData(data) {
        if (this.elements.voltage) this.elements.voltage.textContent = data.voltage.toFixed(2);
        if (this.elements.current) this.elements.current.textContent = data.current.toFixed(2);
        if (this.elements.power) this.elements.power.textContent = data.power.toFixed(2);
    }

    updateStatus(status) {
        //console.log('updateStatus接收到的状态数据:', JSON.stringify(status, null, 2));
        
        // 处理所有状态字段，包括字段名映射
        if (status.normal !== undefined) {
            //console.log(`更新normal状态: ${status.normal}`);
            this.setStatus('normal', status.normal);
        }
        if (status.ocp !== undefined) {
            //console.log(`更新ocp状态: ${status.ocp}`);
            this.setStatus('ocp', status.ocp);
        }
        // if (status.output_enabled !== undefined) {
        //     console.log(`更新enable状态: ${status.output_enabled}`);
        //     this.setStatus('enable', status.output_enabled);
        // }
        
        // 修复1：处理OTP/OHP字段名不匹配问题
        if (status.otp !== undefined) {
            //console.log(`更新ohp(otp)状态: ${status.otp}`);
            this.setStatus('ohp', status.otp);  // 后端叫otp，前端叫ohp
        }
        
        // 修复2：记录其他保护状态（但不在UI中显示，因为HTML中没有对应元素）
        if (status.uvp !== undefined) {
            //console.log(`接收到的uvp状态: ${status.uvp} (UI元素不存在)`);
            // this.setStatus('uvp', status.uvp); // 注释掉，因为HTML中没有uvp元素
        }
        if (status.ovp !== undefined) {
            //console.log(`接收到的ovp状态: ${status.ovp} (UI元素不存在)`);
            // this.setStatus('ovp', status.ovp); // 注释掉，因为HTML中没有ovp元素
        }
        if (status.fault !== undefined) {
            //console.log(`接收到的fault状态: ${status.fault} (UI元素不存在)`);
            // this.setStatus('fault', status.fault); // 注释掉，因为HTML中没有fault元素
        }
        
        // 检查未处理的字段
        const expectedFields = ['normal', 'ocp', /*'output_enabled',*/ 'otp', 'uvp', 'ovp', 'fault'];
        const receivedFields = Object.keys(status);
        const missingFields = expectedFields.filter(field => !receivedFields.includes(field));
        const extraFields = receivedFields.filter(field => !expectedFields.includes(field));
        
        if (missingFields.length > 0) {
            console.warn('缺失的期望字段:', missingFields);
        }
        if (extraFields.length > 0) {
            console.warn('额外的未处理字段:', extraFields);
        }
    }

    setStatus(elementId, active) {
        const element = typeof elementId === 'string' ? 
            this.elements.statusItems[elementId] : elementId;
        
        if (!element) return;
        
        element.classList.toggle('active', active);
        element.classList.toggle('inactive', !active);
    }

    async updateFLTTMR() {
        const slider = Utils.safeGetElementById('flttmrSlider');
        const codeElement = Utils.safeGetElementById('flttmrCode');
        const slopeElement = Utils.safeGetElementById('flttmrSlope');

        if (!slider || !codeElement || !slopeElement) return;

        const idx = parseInt(slider.value, 10);
        codeElement.textContent = `FLTTMR = ${FLTTMR_OPTIONS[idx].code}`;
        slopeElement.textContent = FLTTMR_OPTIONS[idx].val;

        // 发送FLTTMR配置到后端
        try {
            const result = await APIManager.post(CONFIG.API_ENDPOINTS.FLTTMR_CONFIG, { slope: idx });
            console.log('FLTTMR配置已更新:', result);
            this.showToast('FLTTMR配置已更新', 'success');
        } catch (error) {
            console.error('FLTTMR配置更新失败:', error);
            this.showToast('FLTTMR配置更新失败', 'error');
        }
    }

    async updateSSSR() {
        const slider = Utils.safeGetElementById('sssrSlider');
        const codeElement = Utils.safeGetElementById('sssrCode');
        const slopeElement = Utils.safeGetElementById('sssrSlope');

        if (!slider || !codeElement || !slopeElement) return;

        const idx = parseInt(slider.value, 10);
        codeElement.textContent = `SSSR = ${SSSR_OPTIONS[idx].code}`;
        slopeElement.textContent = SSSR_OPTIONS[idx].val;

        // 发送SSSR配置到后端
        try {
            const result = await APIManager.post(CONFIG.API_ENDPOINTS.SSSR_CONFIG, { slope: idx });
            console.log('SSSR配置已更新:', result);
            this.showToast('SSSR配置已更新', 'success');
        } catch (error) {
            console.error('SSSR配置更新失败:', error);
            this.showToast('SSSR配置更新失败', 'error');
        }
    }

    async loadInitialStatus() {
        try {
            // 加载系统状态
            const systemStatus = await APIManager.get(CONFIG.API_ENDPOINTS.SYSTEM_STATUS);
            if (this.elements.powerSwitch) {
                this.elements.powerSwitch.checked = systemStatus.power.enabled;
            }
            this.updateStatus(systemStatus.status);
            console.log('系统状态已更新:', systemStatus);
        } catch (error) {
            console.error('获取系统状态失败:', error);
        }

        try {
            // 加载保护配置并应用到UI
            const protectionStatus = await APIManager.get(CONFIG.API_ENDPOINTS.PROTECTION_STATUS);
            console.log('保护配置:', protectionStatus);
            this.applyProtectionConfig(protectionStatus);
        } catch (error) {
            console.error('获取保护配置失败:', error);
        }

        try {
            // 加载FLTTMR状态并应用到UI
            const flttmrStatus = await APIManager.get(CONFIG.API_ENDPOINTS.FLTTMR_STATUS);
            console.log('FLTTMR状态:', flttmrStatus);
            this.applyFLTTMRConfig(flttmrStatus);
        } catch (error) {
            console.error('获取FLTTMR状态失败:', error);
        }

        try {
            // 加载SSSR状态并应用到UI
            const sssrStatus = await APIManager.get(CONFIG.API_ENDPOINTS.SSSR_STATUS);
            console.log('SSSR状态:', sssrStatus);
            this.applySSSRConfig(sssrStatus);
        } catch (error) {
            console.error('获取SSSR状态失败:', error);
        }
    }

    applyProtectionConfig(protectionStatus) {
        if (!protectionStatus || !protectionStatus.channels) {
            console.warn('无效的保护配置数据:', protectionStatus);
            return;
        }

        console.log('应用保护配置到UI:', protectionStatus);
        
        protectionStatus.channels.forEach(channel => {
            const channelNum = channel.channel;
            const threshold = channel.threshold;
            const action = channel.action;
            
            console.log(`配置通道 ${channelNum}: threshold=${threshold}, action=${action}`);
            
            // 找到对应的卡片
            const card = document.querySelector(`.ctrl-card[data-channel="${channelNum}"]`);
            if (!card) {
                console.warn(`未找到通道 ${channelNum} 的卡片`);
                return;
            }

            // 根据通道类型设置不同的UI元素
            if (channelNum === 3) {
                // OVP通道 - 设置滑块
                const ovpSlider = card.querySelector('#ovpSlider');
                if (ovpSlider) {
                    // 根据threshold值找到对应的索引
                    const ovpIndex = OVP_VALUES.indexOf(threshold);
                    if (ovpIndex !== -1) {
                        ovpSlider.value = ovpIndex;
                        this.updateOVP(); // 更新显示
                        console.log(`OVP滑块设置为索引 ${ovpIndex} (${threshold}V)`);
                    } else {
                        console.warn(`OVP阈值 ${threshold}V 不在预设值中`);
                    }
                }
            } else if (channelNum === 0) {
                // 配置管理通道 - 特殊处理
                console.log('配置管理通道，使用默认设置');
            } else {
                // OCP和UVP通道 - 设置范围按钮
                const rangeBtn = card.querySelector(`.range-btn[data-act="${threshold}"]`);
                if (rangeBtn) {
                    // 清除其他按钮的active状态
                    card.querySelectorAll('.range-btn').forEach(btn => btn.classList.remove('active'));
                    rangeBtn.classList.add('active');
                    console.log(`通道 ${channelNum} 范围按钮设置为 ${threshold}`);
                } else {
                    console.warn(`未找到通道 ${channelNum} 的阈值按钮 ${threshold}`);
                }
            }

            // 设置动作按钮
            const actBtn = card.querySelector(`.act-btn[data-act="${action}"]`);
            if (actBtn) {
                // 清除其他按钮的active状态
                card.querySelectorAll('.act-btn').forEach(btn => btn.classList.remove('active'));
                actBtn.classList.add('active');
                console.log(`通道 ${channelNum} 动作按钮设置为 ${action}`);
            } else {
                console.warn(`未找到通道 ${channelNum} 的动作按钮 ${action}`);
            }
        });
    }

    applyFLTTMRConfig(flttmrStatus) {
        if (!flttmrStatus || flttmrStatus.slope === undefined) {
            console.warn('无效的FLTTMR配置数据:', flttmrStatus);
            return;
        }

        console.log('应用FLTTMR配置到UI:', flttmrStatus);
        
        const slider = Utils.safeGetElementById('flttmrSlider');
        if (slider) {
            slider.value = flttmrStatus.slope;
            this.updateFLTTMR(); // 更新显示
            console.log(`FLTTMR滑块设置为 ${flttmrStatus.slope}`);
        } else {
            console.warn('未找到FLTTMR滑块元素');
        }
    }

    applySSSRConfig(sssrStatus) {
        if (!sssrStatus || sssrStatus.slope === undefined) {
            console.warn('无效的SSSR配置数据:', sssrStatus);
            return;
        }

        console.log('应用SSSR配置到UI:', sssrStatus);
        
        const slider = Utils.safeGetElementById('sssrSlider');
        if (slider) {
            slider.value = sssrStatus.slope;
            this.updateSSSR(); // 更新显示
            console.log(`SSSR滑块设置为 ${sssrStatus.slope}`);
        } else {
            console.warn('未找到SSSR滑块元素');
        }
    }
}

// ==================== 应用主控制器 ====================
class App {
    constructor() {
        this.chart = null;
        this.uiManager = null;
        this.wsManager = null;
        this.init();
    }

    async init() {
        try {
            // 初始化图表
            this.chart = new PowerChart('powerChart');
            
            // 初始化UI管理器
            this.uiManager = new UIManager();
            
            // 初始化WebSocket管理器
            this.wsManager = new WebSocketManager((data) => this.handleWebSocketMessage(data));
            
            // 加载初始状态
            await this.uiManager.loadInitialStatus();
            
            // 初始化图表绘制
            this.chart.draw();
            
            console.log('应用初始化完成');
        } catch (error) {
            console.error('应用初始化失败:', error);
        }
    }

    handleWebSocketMessage(data) {
        if (data.type === 'sensor_data') {
            // 更新传感器数据显示
            this.uiManager.updateSensorData(data);
            
            // 更新功率图表
            this.chart.updateData(data.power, data.timestamp);
            
            // 更新状态指示器
            if (data.status) {
                this.uiManager.updateStatus(data.status);
            }
        }
    }

    destroy() {
        if (this.wsManager) {
            this.wsManager.close();
        }
    }
}

// ==================== 应用启动 ====================
document.addEventListener('DOMContentLoaded', () => {
    window.app = new App();
});

// 页面卸载时清理资源
window.addEventListener('beforeunload', () => {
    if (window.app) {
        window.app.destroy();
    }
});