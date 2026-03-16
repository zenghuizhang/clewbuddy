# ClewBuddy ⚔️ 战士版 v1.0

**日期**: 2026-03-15
**代号**: Warrior 1.0

## 功能

### 设备端 (M5Stack CoreS3)
- 按住触屏录音，松开发送
- 6 个状态表情动画（启动/待机/录音/思考/说话/错误）
- 战士主题 UI：金色双剑徽章、伤疤、中文界面
- Sprite 双缓冲无闪烁刷新
- 语音播放 + 中文回复文字滚动显示
- WiFi 自动连接 + 断线重连

### 服务端 (clewbuddy_server.py)
- Whisper small 模型语音识别（CPU fp32）
- NVIDIA API (Qwen2.5-7b) 快速 LLM 响应 (~1.2s)
- Edge-TTS 中文语音合成 (zh-CN-YunxiNeural)
- 多轮对话上下文保持（最近 6 轮）
- 端口: 9090

## 技术参数
- 录音: 16kHz 16bit mono
- 播放: 16kHz 16bit mono
- 通信: HTTP POST /chat (WAV → JSON+WAV)
- 延迟: ~5-8s 端到端

## 已知问题
- 超长回复(200+字)播放时间较长
- edge-tts 网络延迟不稳定（1.8-7s）
- Whisper CPU 推理约 1-3s
