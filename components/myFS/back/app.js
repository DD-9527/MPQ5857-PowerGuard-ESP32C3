/* 绘制实时功率曲线 自动量程 */
const canvas = document.getElementById('powerChart');
const ctx = canvas.getContext('2d');

const maxDataPoints = 100;
const powerData = [];
const timeLabels = [];

const padding = 40;
let chartWidth, chartHeight, plotWidth, plotHeight;

function resizeCanvas() {
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width;
  canvas.height = rect.height;
  chartWidth = canvas.width;
  chartHeight = canvas.height;
  plotWidth = chartWidth - 2 * padding;
  plotHeight = chartHeight - 2 * padding;
}
resizeCanvas();
window.addEventListener('resize', resizeCanvas);

/* 初始化数据 */
for (let i = 0; i < maxDataPoints; i++) {
  powerData.push(0);
  timeLabels.push(i);
}

/* 自动量程：返回 [min, max]，保证
   1. 量程 ≥ 10 W
   2. min ≥ 0 W
*/
function getAutoRange() {
  if (!powerData.length) return [0, 10];      // 无数据时保底 0~10 W
  let min = Math.min(...powerData);
  let max = Math.max(...powerData);

  /* 下限不低于 0 */
  min = Math.max(0, min);

  /* 量程不低于 10 W */
  if (max - min < 10) {
    const center = (min + max) / 2;
    min = center - 5;
    max = center + 5;
  }

  /* 保证 min 不飘到负数 */
  if (min < 0) {
    max += -min;
    min = 0;
  }

  /* 5 % 视觉边距 */
  const margin = (max - min) * 0.05 || 0.5;
  return [min - margin, max + margin];
}

function drawChart() {
  ctx.clearRect(0, 0, chartWidth, chartHeight);

  /* 自动量程 */
  const [rangeMin, rangeMax] = getAutoRange();
  const range = rangeMax - rangeMin;

  /* 网格 – 横向 5 条 */
  ctx.strokeStyle = 'rgba(255,255,255,0.1)';
  ctx.lineWidth = 1;
  for (let i = 0; i <= 5; i++) {
    const y = padding + (plotHeight / 5) * i;
    ctx.beginPath();
    ctx.moveTo(padding, y);
    ctx.lineTo(chartWidth - padding, y);
    ctx.stroke();
  }
  /* 垂直网格 – 10 条 */
  for (let i = 0; i <= 10; i++) {
    const x = padding + (plotWidth / 10) * i;
    ctx.beginPath();
    ctx.moveTo(x, padding);
    ctx.lineTo(x, chartHeight - padding);
    ctx.stroke();
  }

  /* 坐标轴 */
  ctx.strokeStyle = 'rgba(255,255,255,0.3)';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(padding, padding);
  ctx.lineTo(padding, chartHeight - padding);
  ctx.moveTo(padding, chartHeight - padding);
  ctx.lineTo(chartWidth - padding, chartHeight - padding);
  ctx.stroke();

  /* 自动量程 Y 轴标签 */
  ctx.fillStyle = '#e2e8f0';
  ctx.font = '12px Arial';
  ctx.textAlign = 'right';
  for (let i = 0; i <= 5; i++) {
    const value = rangeMax - (range / 5) * i;       // 从上到下递减
    const y = padding + (plotHeight / 5) * i;
    ctx.fillText(value.toFixed(0) + 'W', padding - 10, y + 4);
  }

  /* 功率曲线 – 按动态量程映射 */
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 3;
  ctx.shadowBlur = 15;
  ctx.shadowColor = 'rgba(56,189,248,0.5)';
  ctx.beginPath();
  const stepX = plotWidth / (maxDataPoints - 1);
  for (let i = 0; i < powerData.length; i++) {
    const x = padding + i * stepX;
    const y = chartHeight - padding - ((powerData[i] - rangeMin) / range) * plotHeight;
    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
  }
  ctx.stroke();
  ctx.shadowBlur = 0;

  /* 当前值 */
  if (powerData.length) {
    const current = powerData[powerData.length - 1];
    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 16px Arial';
    ctx.textAlign = 'left';
    ctx.fillText(current.toFixed(1) + 'W', chartWidth - padding - 50, padding + 20);
  }
}

function startWS() {
  ws = new WebSocket(`ws://${location.host}/ws`);

  ws.onopen = () => console.log('WS 已连接');
  ws.onerror = err => console.error(err);

  ws.onmessage = ev => {
      const j = JSON.parse(ev.data);          // Enhanced data format
      
      if (j.type === 'sensor_data') {
          // 显示传感器数据
          document.getElementById('voltage').textContent = j.voltage.toFixed(2);
          document.getElementById('current').textContent = j.current.toFixed(2);
          document.getElementById('power').textContent   = j.power.toFixed(2);

          // 更新功率图表
          powerData.push(j.power);
          timeLabels.push(j.timestamp);
          if (powerData.length > maxDataPoints) {
              powerData.shift();
              timeLabels.shift();
          }
          drawChart();

          // 更新状态指示器
          if (j.status) {
              setStatus('normal', j.status.normal);
              setStatus('ocp', j.status.ocp);
              setStatus('enable', j.output_enabled);
              // 可以添加更多状态指示器
          }
      }
  };

  /* 关键：断线后 2 s 重新连接 */
  ws.onclose = function() {
    console.log('WS closed, retry in 2 s');
    setTimeout(startWS, 2000);
  };
}

startWS();

// Initial draw
drawChart();


/**
 * 设置某个状态框的 active/inactive
 * @param {HTMLElement|String} el  元素或选择器
 * @param {Boolean}            active  true=active, false=inactive
 */
function setStatus(el, active){
    const box = typeof el === 'string' ? document.querySelector(el) : el;
    if(!box) return;
    box.classList.toggle('active',   active);
    box.classList.toggle('inactive', !active);
}

/* 点击自动切换示例 */
// document.querySelectorAll('.status-item').forEach(item =>
// item.addEventListener('click', () => {
//     const cur = item.classList.contains('active');
//     setStatus(item, !cur);          // 点一次取反
// })
// );

/* 手动调用示例 */
setStatus('normal', true);   // 设为异常
// setStatus('#status1', true); // 设为正常

document.getElementById('powerSwitch').addEventListener('change', async function () {
    const want = this.checked;          // true / false
    try {
        const res = await fetch('/api/power-control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ enabled: want })
        });
        if (!res.ok) throw new Error(res.status);
        const j = await res.json();         // 期待 {"enabled":true,"success":true}
        console.log(j.enabled ? '电源已开启' : '电源已关闭');
        setStatus('enable', j.enabled);
    } catch (e) {
        console.error(e);
        alert('失败');                      // 出错回滚
        this.checked = !want;
    }
});

// 给整页所有卡片绑定一次
document.querySelectorAll('.ctrl-card').forEach(card => {
  card.querySelectorAll('.gear-btn').forEach(btn =>
    btn.addEventListener('click', function () {
      card.querySelectorAll('.gear-btn').forEach(b => b.classList.remove('active'));
      this.classList.add('active');
      send(card); 
    })
  );
  card.querySelectorAll('.act-btn').forEach(btn =>
    btn.addEventListener('click', function () {
      card.querySelectorAll('.act-btn').forEach(b => b.classList.remove('active'));
      this.classList.add('active');
      send(card);
    })
  );

  async function send (c) {
    const ch  = parseInt(c.dataset.channel, 10);
    const thr = parseInt(c.querySelector('.gear-btn.active').dataset.act, 10);
    const act = parseInt(c.querySelector('.act-btn.active').dataset.act, 10);
    const body = { channel: ch, threshold: thr, action: act };
    
    try {
      const r = await fetch('/api/protection-config', {
        method : 'POST',
        headers: { 'Content-Type': 'application/json' },
        body   : JSON.stringify(body)
      });
      if (!r.ok) throw new Error(r.status);
      const j = await r.json();
      console.log(`通道 ${ch} 保护设置已保存`, j);
    } catch (e) {
      console.error(`通道 ${ch} 保护设置保存失败`, e);
    }
  }
});

document.querySelectorAll('.ctrl-card-with-slide').forEach(card => {
  const thrRange = card.querySelector('.thrRange');
  const thrVal   = card.querySelector('.thrVal');
  thrRange.addEventListener('input', () => {
    thrVal.textContent = thrRange.value;
    send(card);
  });
  card.querySelectorAll('.act-btn').forEach(btn =>
    btn.addEventListener('click', function () {
      card.querySelectorAll('.act-btn').forEach(b => b.classList.remove('active'));
      this.classList.add('active');
      send(card);
    })
  );

  async function send (c) {
    const ch  = parseInt(c.dataset.channel, 10);
    const thr = parseInt(c.querySelector('.thrRange').value, 10);
    const act = parseInt(c.querySelector('.act-btn.active').dataset.act, 10);
    const body = { channel: ch, threshold: thr, action: act };
    
    try {
      const r = await fetch('/api/protection-config', {
        method : 'POST',
        headers: { 'Content-Type': 'application/json' },
        body   : JSON.stringify(body)
      });
      if (!r.ok) throw new Error(r.status);
      const j = await r.json();
      console.log(`通道 ${ch} 保护设置已保存`, j);
    } catch (e) {
      console.error(`通道 ${ch} 保护设置保存失败`, e);
    }
  }
});
/* 页面加载时拉一次当前值（可选） */
// window.addEventListener('load', async ()=>{
// try{
//     const j = await (await fetch('/api/ocp')).json();
//     thrRange.value = j.thr; thrVal.textContent = j.thr;
//     document.querySelector(`.act-btn[data-act="${j.act}"]`).click();
// }catch(e){ console.warn('拉取 OCP 配置失败', e); }
// });


/* ===== FLTTMR 滑条逻辑 ===== */
const flttmrOpts = [
    {code:'000', val:'0ms'},
    {code:'001', val:'0.2ms'},
    {code:'010', val:'0.4ms'},
    {code:'011', val:'1ms'},
    {code:'100', val:'2ms (Default)'},
    {code:'101', val:'8ms'},
    {code:'110', val:'32ms'},
    {code:'111', val:'128ms'}
];
const flttmrSlider = document.getElementById('flttmrSlider');
const flttmrCode   = document.getElementById('flttmrCode');
const flttmrSlope  = document.getElementById('flttmrSlope');

function updateFLTTMR() {
    const idx = parseInt(flttmrSlider.value, 10);
    flttmrCode.textContent  = `FLTTMR = ${flttmrOpts[idx].code}`;
    flttmrSlope.textContent = flttmrOpts[idx].val;
}
flttmrSlider.addEventListener('input', updateFLTTMR);
updateFLTTMR();   // 初始化

/* ===== 系统状态获取 ===== */
async function fetchSystemStatus() {
    try {
        const response = await fetch('/api/system-status');
        if (!response.ok) throw new Error(response.status);
        const status = await response.json();
        
        // 更新电源开关状态
        document.getElementById('powerSwitch').checked = status.power.enabled;
        setStatus('enable', status.power.enabled);
        
        // 更新保护状态
        if (status.status) {
            setStatus('normal', status.status.normal);
            setStatus('ocp', status.status.ocp);
            setStatus('ohp', status.status.otp);
        }
        
        console.log('系统状态已更新:', status);
    } catch (error) {
        console.error('获取系统状态失败:', error);
    }
}

/* ===== 页面加载时获取初始状态 ===== */
window.addEventListener('load', async () => {
    await fetchSystemStatus();
    
    // 获取保护配置状态
    try {
        const response = await fetch('/api/protection-status');
        if (!response.ok) throw new Error(response.status);
        const config = await response.json();
        console.log('保护配置:', config);
    } catch (error) {
        console.error('获取保护配置失败:', error);
    }
    
    // 获取SSSR状态
    try {
        const response = await fetch('/api/sssr-status');
        if (!response.ok) throw new Error(response.status);
        const sssr = await response.json();
        console.log('SSSR状态:', sssr);
    } catch (error) {
        console.error('获取SSSR状态失败:', error);
    }
});

/* ===== SSSR 滑条逻辑 ===== */
const sssrOpts = [
    {code:'000', val:'0.25V/ms（默认）'},
    {code:'001', val:'0.5V/ms'},
    {code:'010', val:'1V/ms'},
    {code:'011', val:'4V/ms'},
    {code:'100', val:'16V/ms'},
    {code:'101', val:'32V/ms'},
    {code:'110', val:'64V/ms'},
    {code:'111', val:'无软启动（直接开启）'}
];
const sssrSlider = document.getElementById('sssrSlider');
const sssrCode   = document.getElementById('sssrCode');
const sssrSlope  = document.getElementById('sssrSlope');

function updateSSSR() {
    const idx = parseInt(sssrSlider.value, 10);
    sssrCode.textContent  = `SSSR = ${sssrOpts[idx].code}`;
    sssrSlope.textContent = sssrOpts[idx].val;
    
    // 发送SSSR配置到后端
    fetch('/api/sssr-config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ slope: idx })
    }).then(r => r.json())
      .then(j => console.log('SSSR配置已更新:', j))
      .catch(e => console.error('SSSR配置更新失败:', e));
}
sssrSlider.addEventListener('input', updateSSSR);
updateSSSR();   // 初始化