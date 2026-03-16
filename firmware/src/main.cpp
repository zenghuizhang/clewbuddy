/**
 * ClewBuddy v2.0 — AI Voice Desktop Companion ⚔️
 * M5Stack CoreS3
 *
 * Protocol: POST WAV → response:
 *   [4B json_len LE][JSON][raw PCM s16le]
 */

#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "faces.h"

// ---------------------------------------------------------------------------
// ⚠️ Fill in your WiFi and server details before compiling
static const char* WIFI_SSID   = "YOUR_WIFI_SSID";
static const char* WIFI_PASS   = "YOUR_WIFI_PASS";
static const char* SERVER_HOST = "YOUR_SERVER_IP";
static const int   SERVER_PORT = 9090;

static const int REC_SAMPLE_RATE = 16000;
static const int MAX_RECORD_SEC  = 10;
static const int RECORD_BUF_SIZE = REC_SAMPLE_RATE * 2 * MAX_RECORD_SEC;

// ---------------------------------------------------------------------------
enum State {
    STATE_CONNECTING, STATE_IDLE, STATE_RECORDING,
    STATE_THINKING, STATE_SPEAKING, STATE_SHOW_REPLY, STATE_ERROR
};

static State currentState = STATE_CONNECTING;
static int animFrame = 0;
static unsigned long lastFrameMs = 0;

static int16_t* recordBuffer = nullptr;
static int recordSamples = 0;
static bool isTouching = false;

static char replyText[512] = {0};
static char userText[256] = {0};
static uint8_t* audioResponse = nullptr;
static int audioResponseLen = 0;
static int playSampleRate = 16000;

static unsigned long replyShowStartMs = 0;
static int scrollOffset = 0;
static unsigned long lastScrollMs = 0;

static M5Canvas textSprite(&M5.Lcd);
static bool textSpriteInited = false;

// ---------------------------------------------------------------------------
struct WavHeader {
    char riff[4]={'R','I','F','F'}; uint32_t fileSize;
    char wave[4]={'W','A','V','E'}; char fmt[4]={'f','m','t',' '};
    uint32_t fmtSize=16; uint16_t audioFormat=1; uint16_t numChannels=1;
    uint32_t sampleRate=REC_SAMPLE_RATE; uint32_t byteRate=REC_SAMPLE_RATE*2;
    uint16_t blockAlign=2; uint16_t bitsPerSample=16;
    char data[4]={'d','a','t','a'}; uint32_t dataSize;
} __attribute__((packed));

// ---------------------------------------------------------------------------
void connectWiFi() {
    currentState = STATE_CONNECTING;
    Face::drawConnecting(0);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int frame=0, att=0;
    while (WiFi.status()!=WL_CONNECTED && att<40) {
        delay(500); Face::drawConnecting(frame++); att++;
    }
    if (WiFi.status()!=WL_CONNECTED) {
        Face::drawError("\xe8\xbf\x9e\xe6\x8e\xa5\xe5\xa4\xb1\xe8\xb4\xa5");
        delay(3000); ESP.restart();
    }
    Serial.printf("[WiFi] %s\n", WiFi.localIP().toString().c_str());
}

// ---------------------------------------------------------------------------
void startRecording() {
    recordSamples = 0;
    memset(recordBuffer, 0, RECORD_BUF_SIZE);
    auto cfg = M5.Mic.config();
    cfg.sample_rate = REC_SAMPLE_RATE;
    cfg.dma_buf_count = 8; cfg.dma_buf_len = 512; cfg.over_sampling = 1;
    M5.Mic.config(cfg); M5.Mic.begin();
}
void stopRecording() { M5.Mic.end(); }
void recordChunk() {
    const int n = 1024;
    if (recordSamples + n > RECORD_BUF_SIZE / 2) return;
    if (M5.Mic.isEnabled())
        if (M5.Mic.record(recordBuffer + recordSamples, n, REC_SAMPLE_RATE))
            recordSamples += n;
}

// ---------------------------------------------------------------------------
bool jsonStr(const char* j, const char* key, char* out, int maxLen) {
    char pat[64]; snprintf(pat,sizeof(pat),"\"%s\":",key);
    const char* p = strstr(j,pat); if(!p) return false;
    p += strlen(pat); while(*p==' '||*p=='\t') p++;
    if(*p!='"') return false; p++;
    int i=0;
    while(*p && *p!='"' && i<maxLen-1) {
        if(*p=='\\' && *(p+1)) { p++; out[i++]=*p; } else out[i++]=*p;
        p++;
    }
    out[i]='\0'; return true;
}

int jsonInt(const char* j, const char* key, int def) {
    char pat[64]; snprintf(pat,sizeof(pat),"\"%s\":",key);
    const char* p = strstr(j,pat); if(!p) return def;
    p += strlen(pat); while(*p==' '||*p=='\t') p++;
    return atoi(p);
}

// ---------------------------------------------------------------------------
void drawReplyArea(const char* text, int scrollX) {
    if (!textSpriteInited) {
        textSprite.createSprite(320, 43);
        textSprite.setColorDepth(16);
        textSpriteInited = true;
    }
    textSprite.fillSprite(Face::BG);
    textSprite.setFont(&fonts::efontCN_12);
    if (userText[0]) {
        textSprite.setTextColor(Face::DARK_GREY, Face::BG);
        textSprite.setCursor(5, 2);
        textSprite.print("\xe6\x88\x91: ");
        char ubuf[40]={0}; strncpy(ubuf, userText, 39);
        textSprite.print(ubuf);
    }
    textSprite.setTextColor(Face::GREEN, Face::BG);
    textSprite.setCursor(5 - scrollX, 21);
    textSprite.print("> "); textSprite.print(text);
    textSprite.setFont(nullptr);
    textSprite.pushSprite(0, 175);
}

int calcTextWidth(const char* p) {
    int w = 0;
    while (*p) {
        if ((uint8_t)*p >= 0x80) {
            w += 12;
            if ((*p & 0xE0)==0xC0) p+=2;
            else if ((*p & 0xF0)==0xE0) p+=3;
            else if ((*p & 0xF8)==0xF0) p+=4;
            else p++;
        } else { w += 7; p++; }
    }
    return w + 20;
}

// ---------------------------------------------------------------------------
bool sendToServer() {
    if (recordSamples < REC_SAMPLE_RATE / 4) return false;

    int dataBytes = recordSamples * 2;
    WavHeader hdr; hdr.dataSize = dataBytes;
    hdr.fileSize = sizeof(WavHeader) - 8 + dataBytes;
    int totalSize = sizeof(WavHeader) + dataBytes;

    uint8_t* wavBuf = (uint8_t*)ps_malloc(totalSize);
    if (!wavBuf) return false;
    memcpy(wavBuf, &hdr, sizeof(WavHeader));
    memcpy(wavBuf + sizeof(WavHeader), recordBuffer, dataBytes);

    char url[128];
    snprintf(url, sizeof(url), "http://%s:%d/chat", SERVER_HOST, SERVER_PORT);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "audio/wav");
    http.setTimeout(30000);

    int httpCode = http.POST(wavBuf, totalSize);
    free(wavBuf);

    if (httpCode != 200) {
        Serial.printf("[HTTP] Error: %d\n", httpCode);
        http.end(); return false;
    }

    int contentLen = http.getSize();
    if (contentLen <= 4) { http.end(); return false; }

    // Read all response
    uint8_t* respBuf = (uint8_t*)ps_malloc(contentLen);
    if (!respBuf) { http.end(); return false; }

    WiFiClient* stream = http.getStreamPtr();
    int received = 0;
    int thinkFrame = 0;
    while (received < contentLen) {
        int avail = stream->available();
        if (avail > 0) {
            int n = stream->readBytes(respBuf + received, min(avail, contentLen - received));
            received += n;
        } else {
            // Animate thinking while downloading
            if (millis() - lastFrameMs > 200) {
                Face::drawThinking(thinkFrame++);
                lastFrameMs = millis();
            }
            delay(5);
        }
    }
    http.end();

    // Parse: [4B json_len][JSON][PCM]
    if (received < 4) { free(respBuf); return false; }
    uint32_t jsonLen = respBuf[0]|(respBuf[1]<<8)|(respBuf[2]<<16)|(respBuf[3]<<24);
    if (4 + jsonLen >= (uint32_t)received) { free(respBuf); return false; }

    char jsonBuf[512] = {0};
    int jl = (jsonLen < 511) ? jsonLen : 511;
    memcpy(jsonBuf, respBuf + 4, jl);

    jsonStr(jsonBuf, "text", replyText, sizeof(replyText));
    jsonStr(jsonBuf, "user_text", userText, sizeof(userText));
    playSampleRate = jsonInt(jsonBuf, "sample_rate", 16000);

    // PCM data starts after header
    int pcmOffset = 4 + jsonLen;
    audioResponseLen = received - pcmOffset;
    audioResponse = (uint8_t*)ps_malloc(audioResponseLen);
    if (audioResponse) {
        memcpy(audioResponse, respBuf + pcmOffset, audioResponseLen);
    }
    free(respBuf);

    Serial.printf("[HTTP] Reply: %s (sr=%d, pcm=%db)\n", replyText, playSampleRate, audioResponseLen);
    return audioResponse != nullptr;
}

// ---------------------------------------------------------------------------
void playAudioWithAnimation() {
    if (!audioResponse || audioResponseLen < 4) return;

    int16_t* pcm = (int16_t*)audioResponse;
    int pcmSamples = audioResponseLen / 2;

    M5.Speaker.begin();
    M5.Speaker.setVolume(180);

    Serial.printf("[Speaker] Playing %d samples @ %dHz\n", pcmSamples, playSampleRate);
    Face::drawSpeaking(0, nullptr);
    scrollOffset = 0;
    lastScrollMs = millis();

    const int chunkSize = 512;
    int played = 0, frame = 0;

    while (played < pcmSamples) {
        int remain = pcmSamples - played;
        int n = (remain < chunkSize) ? remain : chunkSize;

        int retries = 0;
        while (!M5.Speaker.playRaw(pcm + played, n, playSampleRate, false, 1, 0)) {
            delay(1);
            if (++retries > 200) break;
        }
        played += n;

        unsigned long now = millis();
        if (now - lastFrameMs > 300) {
            Face::drawSpeaking(frame++, nullptr);
            lastFrameMs = now;
        }
        if (now - lastScrollMs > 80) {
            int tw = calcTextWidth(replyText);
            if (tw > 310) {
                scrollOffset += 2;
                if (scrollOffset > tw - 280) scrollOffset = 0;
            }
            drawReplyArea(replyText, scrollOffset);
            lastScrollMs = now;
        }
    }

    while (M5.Speaker.isPlaying()) delay(10);
    M5.Speaker.end();

    free(audioResponse);
    audioResponse = nullptr;
    audioResponseLen = 0;
}

// ---------------------------------------------------------------------------
void drawReplyScreen(int frame) {
    if (frame == 0) {
        Face::drawTopBar("\xe5\x9b\x9e\xe7\xad\x94", Face::GREEN);
        Face::drawBottomBar("[ \xe6\x8c\x89\xe4\xbd\x8f\xe8\xaf\xb4\xe8\xaf\x9d ]", Face::DIM_GOLD);
    }
    Face::initSprite();
    Face::faceSprite.fillSprite(Face::BG);
    Face::spriteWarriorEye(Face::EYE_L_X, Face::EYE_Y, 0, 2);
    Face::spriteWarriorEye(Face::EYE_R_X, Face::EYE_Y, 0, 2);
    Face::spriteScar(); Face::spriteSmile();
    Face::pushFace();

    unsigned long now = millis();
    if (now - lastScrollMs > 80) {
        int tw = calcTextWidth(replyText);
        if (tw > 310) { scrollOffset += 2; if (scrollOffset > tw-280) scrollOffset=0; }
        lastScrollMs = now;
    }
    drawReplyArea(replyText, scrollOffset);
}

// ---------------------------------------------------------------------------
void setup() {
    auto cfg = M5.config();
    cfg.internal_mic = true; cfg.internal_spk = true;
    M5.begin(cfg);
    M5.Lcd.setRotation(1);
    Face::drawSplash();
    delay(1500);

    recordBuffer = (int16_t*)ps_malloc(RECORD_BUF_SIZE);
    if (!recordBuffer) { Face::drawError("PSRAM!"); while(1) delay(1000); }

    connectWiFi();
    currentState = STATE_IDLE;
    Face::drawIdle();
}

// ---------------------------------------------------------------------------
void loop() {
    M5.update();
    auto touch = M5.Touch.getDetail();
    bool touching = touch.isPressed();

    switch (currentState) {
        case STATE_IDLE:
            if (millis() - lastFrameMs > 3000) {
                Face::drawIdle(true); delay(150); Face::drawIdle(false);
                lastFrameMs = millis();
            }
            if (touching && !isTouching) {
                currentState = STATE_RECORDING; animFrame = 0;
                startRecording(); Face::drawRecording(0);
            }
            break;

        case STATE_RECORDING:
            recordChunk();
            if (millis() - lastFrameMs > 200) {
                Face::drawRecording(animFrame++); lastFrameMs = millis();
            }
            if (!touching && isTouching) {
                stopRecording();
                currentState = STATE_THINKING; animFrame = 0;
                Face::drawThinking(0);

                if (sendToServer()) {
                    currentState = STATE_SPEAKING;
                    Face::drawSpeaking(0, nullptr);
                    drawReplyArea(replyText, 0);
                    playAudioWithAnimation();
                    currentState = STATE_SHOW_REPLY;
                    replyShowStartMs = millis();
                    drawReplyScreen(0);
                } else {
                    Face::drawError("\xe6\x9c\x8d\xe5\x8a\xa1\xe5\x99\xa8\xe9\x94\x99\xe8\xaf\xaf");
                    delay(2000);
                    currentState = STATE_IDLE; Face::drawIdle();
                }
            }
            break;

        case STATE_THINKING:
            if (millis() - lastFrameMs > 200) {
                Face::drawThinking(animFrame++); lastFrameMs = millis();
            }
            break;

        case STATE_SPEAKING:
            break;

        case STATE_SHOW_REPLY:
            drawReplyScreen(animFrame++);
            if (touching && !isTouching) {
                currentState = STATE_RECORDING; animFrame = 0;
                startRecording(); Face::drawRecording(0);
            } else if (millis() - replyShowStartMs > 1500) {
                currentState = STATE_IDLE; Face::drawIdle();
            }
            break;

        case STATE_ERROR: delay(100); break;
        default: break;
    }

    isTouching = touching;

    if (currentState == STATE_IDLE && WiFi.status() != WL_CONNECTED) {
        connectWiFi(); currentState = STATE_IDLE; Face::drawIdle();
    }
    delay(10);
}
