# ⚔️ ClewBuddy — AI 语音桌面伴侣（战士 1.0）

基于 M5Stack CoreS3 的 AI 语音交互桌面伴侣设备。按住屏幕说话，AI 听懂、思考、回复语音，屏幕显示战士风格的表情动画。

## 🎯 功能

- **按住说话** — 触屏录音，松开自动发送
- **语音对话** — STT → LLM → TTS 全链路
- **战士 UI** — 金色双剑徽章、伤疤、6 种状态动画
- **多轮对话** — 上下文保持（最近 6 轮）
- **酷炫开机** — 火花粒子 → 双剑交叉 → 文字闪现 → 眼睛睁开

## 🏗️ 架构

```
┌─────────────────┐     HTTP/WiFi      ┌──────────────────────┐
│  M5Stack CoreS3 │ ◄────────────────► │   Local Python Server │
│                 │                     │                       │
│ • 录音 16kHz    │  POST /chat         │ • Whisper STT         │
│ • 播放 PCM      │  ──────────────►   │ • LLM (Qwen2.5-7b)   │
│ • 战士表情      │                     │ • Edge TTS            │
│ • 触屏交互      │  JSON + PCM audio   │                       │
│                 │  ◄──────────────   │                       │
└─────────────────┘                     └──────────────────────┘
```

## 🔧 硬件

- **M5Stack CoreS3** — ESP32-S3, 240MHz, 320KB RAM, 16MB Flash, PSRAM
- 内置麦克风、扬声器、320×240 触摸屏、WiFi

## 🚀 快速开始

### 服务端

```bash
cd server
pip install -r requirements.txt
# 设置 LLM API Key
export LLM_API_KEY="your-api-key"
python clewbuddy_server.py
```

### 固件端

1. 安装 [PlatformIO](https://platformio.org/)
2. 编辑 `firmware/src/main.cpp`，填入 WiFi 和服务器 IP
3. 编译烧录：

```bash
cd firmware
pio run --target upload
```

或直接使用预编译固件：`releases/v1.0-warrior/firmware.bin`

## 📁 文件结构

```
├── PRD.md                        # 产品需求文档
├── firmware/
│   ├── platformio.ini            # PlatformIO 配置
│   ├── src/main.cpp              # 主程序
│   └── include/faces.h           # 战士版 UI 动画
├── server/
│   ├── clewbuddy_server.py       # FastAPI 服务器
│   └── requirements.txt          # Python 依赖
└── releases/v1.0-warrior/
    ├── firmware.bin               # 预编译固件
    └── CHANGELOG.md               # 版本日志
```

## ⚡ 性能

- 端到端延迟: ~5-8s
- STT (Whisper small/CPU): ~1-3s
- LLM: ~1.2s
- TTS (Edge-TTS): ~1.8-7s

## 📜 版本

- **v1.0 战士版** (2026-03-15) — 首个完整版本，战士主题 UI

---

*Lobster Hackathon 项目 🦞*
