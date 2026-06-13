# 智能门牌 Smart Doorplate

基于 **ESP32** 的智能门牌系统，支持实时天气显示、留言板管理和超声波访客检测。

---

## 📂 项目结构

```
SmartDoorplate_ESP32/
├── frontend/door-front/    ← Vue 3 + Vite 前端看板
│   ├── src/
│   │   ├── App.vue         ← 主应用组件
│   │   ├── main.js         ← 入口文件
│   │   └── styles/
│   │       └── global.css  ← 全局基础样式
│   └── ...
├── server/                 ← Node.js + Express 后端 API
│   ├── server.js           ← 主服务（天气、留言、访客接口）
│   └── db.js               ← SQLite 数据库初始化
└── sketch_jun1a/           ← ESP32 Arduino 固件
    └── sketch_jun1a.ino    ← 硬件控制逻辑
```

## 🛠 技术栈

| 层 | 技术 |
|----|------|
| 前端 | Vue 3, Element Plus, Axios, Vite |
| 后端 | Node.js, Express, better-sqlite3, Axios |
| 硬件 | ESP32, TFT_eSPI, HC-SR04 超声波, LCD1602 I2C |
| 天气 | OpenWeatherMap API |

## 🚀 快速开始

### 1. 后端服务

```bash
cd server
npm install
# 编辑 server.js，将 YOUR_OPENWEATHER_API_KEY 替换为真实 API Key
node server.js
```

服务启动在 `http://localhost:3000`

### 2. 前端看板

```bash
cd frontend/door-front
npm install
npm run dev
```

### 3. ESP32 固件

1. 用 Arduino IDE 或其他支持ESP32开发板的代码编辑器打开 `sketch_jun1a/sketch_jun1a.ino`
2. 修改 WiFi 配置和服务器地址
3. 选择 ESP32 开发板，编译上传

## 📡 API 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/weather` | 获取天气（温度、描述、湿度） |
| GET | `/api/messages` | 获取留言列表 |
| POST | `/api/messages` | 添加留言 `{ content: "..." }` |
| DELETE | `/api/messages/:id` | 删除留言 |
| GET | `/api/visitors` | 获取访客计数 |
| POST | `/api/visitors` | 记录一次访客 |
