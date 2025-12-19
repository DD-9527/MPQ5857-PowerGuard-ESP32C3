# MPQ5857 电源管理芯片 Web 界面 - 优化版本

## 项目概述

这是一个用于MPQ5857电源管理芯片的Web控制界面，提供实时监控、保护配置和高级设置功能。本优化版本对原始代码进行了全面重构，提高了性能、可维护性和用户体验。

## 文件结构

```
├── index-optimized.html      # 优化后的HTML文件
├── app-optimized.js         # 优化后的JavaScript文件
├── style-optimized.css      # 优化后的CSS文件
├── fonts/
│   └── SmileySans-Ob.woff2 # 自定义字体
└── README.md               # 项目文档
```

## 主要优化内容

### 1. 代码组织优化

#### 模块化结构
- **Utils**: 工具函数集合，包含防抖、节流、安全DOM操作等
- **DataManager**: 循环缓冲区实现，高效管理时间序列数据
- **PowerChart**: 图表绘制类，支持自动量程和性能优化
- **WebSocketManager**: WebSocket连接管理，支持自动重连和状态指示
- **APIManager**: 统一的API请求管理
- **UIManager**: UI状态管理和用户交互处理
- **App**: 应用主控制器，协调各模块

#### 性能优化
- 使用循环缓冲区替代数组shift操作，提高大数据量时的性能
- 图表绘制添加节流机制，避免过度重绘
- DOM元素缓存，减少重复查询
- 防抖处理窗口调整事件

### 2. 用户体验改进

#### 新增UI元素
- 连接状态指示器：实时显示WebSocket连接状态
- 加载覆盖层：页面加载时提供视觉反馈
- Toast提示系统：友好的操作反馈，替代alert弹窗
- 图表控制按钮：暂停和重置图表功能（预留）

#### 可访问性增强
- 完整的ARIA标签支持
- 语义化HTML结构
- 键盘导航支持
- 高对比度模式支持
- 减少动画模式支持

#### 响应式设计
- 移动设备优化布局
- 触摸友好的控件尺寸
- 灵活的网格系统

### 3. 错误处理和边界情况

#### 安全性改进
- 所有JSON解析都有错误处理
- DOM操作前进行存在性检查
- API请求统一错误处理
- WebSocket连接异常处理和自动重连

#### 边界情况处理
- 图表数据范围检查，防止除零错误
- 用户输入验证
- 网络请求超时处理
- 资源清理机制

## 功能特性

### 实时监控
- 电压、电流、功率实时显示
- 功率波形图表，支持自动量程
- 系统状态指示（输出、过流、正常、过热）

### 保护配置
- 过流保护：5A/10A/15A/20A可选，支持立即断开或钳位断开
- 欠压保护：2V/4V/9V可选，支持多种保护行为
- 过压保护：16-42V可调，支持多种保护行为

### 高级设置
- 消抖时间设置（FLTTMR）：8级可调，0ms-128ms，使用统一滑块样式
- 软启动斜率设置（SSSR）：8级可调，0.25V/ms到无软启动，使用统一滑块样式

### 系统功能
- 电源输出开关控制
- 配置保存和加载
- 实时连接状态监控

## 使用方法

### 直接使用
1. 将所有文件部署到Web服务器
2. 访问 `index-optimized.html`
3. 确保后端API端点可用

### 开发环境
1. 使用本地Web服务器（如Live Server）
2. 确保CORS配置正确
3. 检查WebSocket连接路径

## API接口

### WebSocket
- 端点: `ws://host/ws`
- 数据格式: JSON
- 消息类型: `sensor_data`

```json
{
  "type": "sensor_data",
  "voltage": 12.34,
  "current": 1.23,
  "power": 15.18,
  "timestamp": 1234567890,
  "status": {
    "normal": true,
    "ocp": false,
    "output_enabled": true,
    "otp": false
  }
}
```

### REST API
- `POST /api/power-control` - 电源控制
- `POST /api/protection-config` - 保护配置
- `GET /api/system-status` - 系统状态
- `GET /api/protection-status` - 保护配置状态
- `GET /api/sssr-status` - SSSR状态
- `POST /api/sssr-config` - SSSR配置

## 浏览器兼容性

- Chrome 60+
- Firefox 55+
- Safari 12+
- Edge 79+

## 开发指南

### 代码风格
- 使用ES6+语法
- 采用模块化设计
- 遵循JSDoc注释规范
- 使用语义化命名
- 统一UI组件样式（如滑块使用range-slider类）

### 扩展功能
1. 在相应类中添加新方法
2. 更新CONFIG配置
3. 添加必要的HTML元素
4. 补充CSS样式
5. 更新文档

### 调试技巧
- 使用浏览器开发者工具
- 检查Console日志
- 监控Network请求
- 使用断点调试

## 更新日志

### v2.1 (滑块样式统一版本)
- 统一所有滑块样式为range-slider类
- 改进滑块交互体验，添加悬停效果
- 优化滑块视觉反馈

### v2.0 (优化版本)
- 全面重构代码架构
- 提升性能和用户体验
- 增强错误处理
- 改进可访问性
- 优化移动端体验

### v1.0 (原始版本)
- 基础功能实现
- 简单的监控界面

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 联系方式

如有问题或建议，请联系：dd_9527@foxmail.com