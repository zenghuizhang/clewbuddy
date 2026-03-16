#!/usr/bin/env python3
"""ClewBuddy Server v2.0 — Fast response with parallel TTS
   Port: 9090

   Protocol: POST /chat → binary response:
     4 bytes: JSON length (LE uint32)
     N bytes: JSON {"text","user_text","sample_rate"}
     Rest:    Raw PCM s16le (all sentences concatenated)
"""

import asyncio
import json
import os
import struct
import tempfile
import time

import edge_tts
import uvicorn
import whisper
from fastapi import FastAPI, Request
from fastapi.responses import Response

WHISPER_MODEL = os.getenv("WHISPER_MODEL", "small")
TTS_VOICE = os.getenv("TTS_VOICE", "zh-CN-YunxiNeural")
SERVER_PORT = int(os.getenv("SERVER_PORT", "9090"))
PLAYBACK_RATE = 16000

LLM_BASE_URL = os.getenv("LLM_BASE_URL", "https://integrate.api.nvidia.com/v1")
LLM_API_KEY = os.getenv("LLM_API_KEY", "")  # Set via environment variable
LLM_MODEL = os.getenv("LLM_MODEL", "qwen/qwen2.5-7b-instruct")

SYSTEM_PROMPT = """你是 ClewBuddy，住在小方块硬件里的AI桌面伙伴。
性格活泼，说话自然，像朋友聊天。用中文回答，不要用markdown或emoji。
回答控制在3句话以内，简洁有趣。"""

app = FastAPI(title="ClewBuddy v2.0")
whisper_model = None
conversation_history: list[dict] = []
MAX_HISTORY = 6


@app.on_event("startup")
async def startup():
    global whisper_model
    print(f"[v2.0] Loading Whisper '{WHISPER_MODEL}' ...")
    whisper_model = whisper.load_model(WHISPER_MODEL, device="cpu")
    print(f"[v2.0] Ready — port {SERVER_PORT}")


# --- STT ---
def transcribe_audio(audio_bytes: bytes) -> str:
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        f.write(audio_bytes); tmp = f.name
    try:
        t0 = time.time()
        r = whisper_model.transcribe(tmp, language="zh", fp16=False)
        text = r["text"].strip()
        print(f"[STT] ({time.time()-t0:.1f}s) {text}")
        return text
    except Exception as e:
        print(f"[STT] Error: {e}"); return ""
    finally:
        try: os.unlink(tmp)
        except: pass


# --- LLM ---
async def chat_llm(user_text: str) -> str:
    import aiohttp
    conversation_history.append({"role": "user", "content": user_text})
    while len(conversation_history) > MAX_HISTORY * 2:
        conversation_history.pop(0)

    headers = {"Content-Type": "application/json"}
    if LLM_API_KEY:
        headers["Authorization"] = f"Bearer {LLM_API_KEY}"

    payload = {
        "model": LLM_MODEL,
        "messages": [{"role": "system", "content": SYSTEM_PROMPT}] + conversation_history,
        "max_tokens": 200, "temperature": 0.7,
    }
    url = LLM_BASE_URL.rstrip("/") + "/chat/completions"
    t0 = time.time()
    try:
        async with aiohttp.ClientSession() as session:
            async with session.post(url, json=payload, headers=headers,
                                    timeout=aiohttp.ClientTimeout(total=15)) as resp:
                if resp.status == 200:
                    data = await resp.json()
                    reply = data["choices"][0]["message"]["content"].strip()
                else:
                    body = await resp.text()
                    print(f"[LLM] Error {resp.status}: {body[:200]}")
                    reply = "脑子转不过来了，再说一遍？"
    except Exception as e:
        print(f"[LLM] Exception: {e}")
        reply = "网络有点问题。"

    conversation_history.append({"role": "assistant", "content": reply})
    print(f"[LLM] ({time.time()-t0:.1f}s) {reply}")
    return reply


# --- TTS ---
async def tts_to_pcm(text: str) -> bytes:
    t0 = time.time()
    comm = edge_tts.Communicate(text, TTS_VOICE, rate="+5%")
    mp3_chunks = []
    async for chunk in comm.stream():
        if chunk["type"] == "audio":
            mp3_chunks.append(chunk["data"])
    mp3_data = b"".join(mp3_chunks)

    with tempfile.NamedTemporaryFile(suffix=".mp3", delete=False) as f:
        f.write(mp3_data); mp3_path = f.name
    pcm_path = mp3_path.replace(".mp3", ".pcm")

    proc = await asyncio.create_subprocess_exec(
        "ffmpeg", "-y", "-i", mp3_path,
        "-ar", str(PLAYBACK_RATE), "-ac", "1", "-f", "s16le", pcm_path,
        stdout=asyncio.subprocess.DEVNULL, stderr=asyncio.subprocess.DEVNULL)
    await proc.wait()

    try:
        with open(pcm_path, "rb") as f: pcm = f.read()
    finally:
        for p in (mp3_path, pcm_path):
            try: os.unlink(p)
            except: pass
    print(f"[TTS] ({time.time()-t0:.1f}s) '{text[:20]}...' → {len(pcm)}b")
    return pcm


def split_sentences(text: str) -> list[str]:
    sentences, buf = [], []
    for ch in text:
        buf.append(ch)
        if ch in '。！？；\n!?;':
            s = ''.join(buf).strip()
            if s: sentences.append(s)
            buf = []
    r = ''.join(buf).strip()
    if r: sentences.append(r)
    return sentences


# --- POST /chat ---
@app.post("/chat")
async def chat(request: Request):
    t0 = time.time()
    audio_bytes = await request.body()
    print(f"[/chat] Received {len(audio_bytes)} bytes")

    user_text = transcribe_audio(audio_bytes)
    if not user_text: user_text = "你好"

    reply_text = await chat_llm(user_text)

    # Split and TTS in parallel
    sentences = split_sentences(reply_text)
    print(f"[/chat] {len(sentences)} sentences, parallel TTS")

    t_tts = time.time()
    tasks = [tts_to_pcm(s) for s in sentences]
    results = await asyncio.gather(*tasks)
    all_pcm = b"".join(results)
    print(f"[TTS] All done in {time.time()-t_tts:.1f}s, {len(all_pcm)} bytes")

    # Build: [4B json_len][JSON][PCM]
    resp_json = json.dumps({
        "text": reply_text, "user_text": user_text,
        "sample_rate": PLAYBACK_RATE, "pcm": True,
    }, ensure_ascii=False).encode("utf-8")

    body = struct.pack("<I", len(resp_json)) + resp_json + all_pcm
    elapsed = time.time() - t0
    print(f"[/chat] Done {elapsed:.1f}s — '{user_text}' → '{reply_text[:40]}...' ({len(all_pcm)}b)")

    return Response(content=body, media_type="application/octet-stream")


@app.get("/health")
async def health():
    return {"status": "ok", "model": WHISPER_MODEL, "tts": TTS_VOICE, "version": "2.0"}


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=SERVER_PORT)
