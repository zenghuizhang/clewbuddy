#pragma once
#include <M5Unified.h>

// ⚔️ ClewBuddy — 战士版 UI (无闪烁)
// 使用 Sprite 双缓冲 + 局部刷新消除闪屏

namespace Face {

// ── 颜色 ──────────────────────────────────────────────
constexpr uint16_t BG         = 0x0000;
constexpr uint16_t GOLD       = 0xFE60;
constexpr uint16_t DIM_GOLD   = 0x8B20;
constexpr uint16_t WHITE      = 0xFFFF;
constexpr uint16_t BLACK      = 0x0000;
constexpr uint16_t RED        = 0xF800;
constexpr uint16_t CYAN       = 0x07FF;
constexpr uint16_t GREEN      = 0x07E0;
constexpr uint16_t DARK_GREY  = 0x4208;
constexpr uint16_t BAR_BG     = 0x1082;

// ── 布局 ──────────────────────────────────────────────
constexpr int CX = 160;
constexpr int CY = 105;
constexpr int EYE_L_X = CX - 42;
constexpr int EYE_R_X = CX + 42;
constexpr int EYE_Y   = CY - 10;
constexpr int EYE_W   = 22;
constexpr int EYE_H   = 20;
constexpr int PUPIL_R = 7;

// Face sprite: covers the animated area (y=28 to y=222, full width)
// This avoids redrawing top/bottom bars every frame
static M5Canvas faceSprite(&M5.Lcd);
static bool spriteInited = false;

constexpr int SPRITE_Y = 28;
constexpr int SPRITE_H = 194;  // 222 - 28

inline void initSprite() {
    if (!spriteInited) {
        faceSprite.createSprite(320, SPRITE_H);
        faceSprite.setColorDepth(16);
        spriteInited = true;
    }
}

// Push sprite to screen at the face area
inline void pushFace() {
    faceSprite.pushSprite(0, SPRITE_Y);
}

// ── Sprite-local coordinates (y offset by -SPRITE_Y) ──
constexpr int SY(int screenY) { return screenY - SPRITE_Y; }

// ── 顶栏 (直接画到LCD，只在状态切换时调用) ──────────
inline void drawTopBar(const char* label, uint16_t labelColor) {
    M5.Lcd.fillRect(0, 0, 320, 28, BAR_BG);

    // ⚔️ 交叉双剑（带剑尖+护手）
    // 左剑
    M5.Lcd.drawLine(6, 4, 24, 24, GOLD);
    M5.Lcd.drawLine(7, 4, 25, 24, GOLD);
    M5.Lcd.drawLine(6, 3, 6, 5, WHITE);  // 剑尖
    M5.Lcd.drawLine(12, 12, 18, 16, GOLD);  // 护手
    // 右剑
    M5.Lcd.drawLine(24, 4, 6, 24, GOLD);
    M5.Lcd.drawLine(25, 4, 7, 24, GOLD);
    M5.Lcd.drawLine(24, 3, 24, 5, WHITE);  // 剑尖
    // 交叉点
    M5.Lcd.fillCircle(15, 14, 1, WHITE);

    // ClewBuddy
    M5.Lcd.setTextColor(GOLD, BAR_BG);
    M5.Lcd.setFont(nullptr);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(32, 7);
    M5.Lcd.setTextFont(2);
    M5.Lcd.print("ClewBuddy");

    // 右侧状态标签（中文）
    M5.Lcd.setTextColor(labelColor, BAR_BG);
    M5.Lcd.setFont(&fonts::efontCN_14);
    M5.Lcd.setCursor(260, 5);
    M5.Lcd.print(label);
    M5.Lcd.setFont(nullptr);
}

// ── 底栏 (直接画到LCD，只在状态切换时调用) ──────────
inline void drawBottomBar(const char* text, uint16_t color) {
    M5.Lcd.fillRect(0, 220, 320, 20, BAR_BG);
    M5.Lcd.setTextColor(color, BAR_BG);
    M5.Lcd.setFont(&fonts::efontCN_14);
    M5.Lcd.setTextDatum(textdatum_t::top_center);
    M5.Lcd.drawString(text, CX, 222);
    M5.Lcd.setTextDatum(textdatum_t::top_left);
    M5.Lcd.setFont(nullptr);
}

// ── Sprite 上画战士眼睛 ───────────────────────────────
inline void spriteWarriorEye(int cx, int cy, int pupilOffX = 0, int pupilOffY = 0, bool angry = false) {
    int sy = SY(cy);
    faceSprite.drawEllipse(cx, sy, EYE_W + 2, EYE_H + 2, DIM_GOLD);
    faceSprite.fillEllipse(cx, sy, EYE_W, EYE_H, WHITE);
    faceSprite.fillCircle(cx + pupilOffX, sy + pupilOffY, PUPIL_R, BLACK);
    faceSprite.fillCircle(cx + pupilOffX - 2, sy + pupilOffY - 2, 2, WHITE);
    if (angry) {
        faceSprite.drawLine(cx - EYE_W, sy - EYE_H - 6, cx + EYE_W/2, sy - EYE_H - 2, GOLD);
        faceSprite.drawLine(cx - EYE_W, sy - EYE_H - 5, cx + EYE_W/2, sy - EYE_H - 1, GOLD);
    }
}

// ── Sprite 上画伤疤 ──────────────────────────────────
inline void spriteScar() {
    int sx = EYE_L_X + EYE_W + 5;
    int sy = SY(EYE_Y) - EYE_H - 2;
    faceSprite.drawLine(sx, sy, sx + 8, sy + 14, DIM_GOLD);
    faceSprite.drawLine(sx + 1, sy, sx + 9, sy + 14, DIM_GOLD);
    for (int i = 0; i < 3; i++) {
        int yy = sy + 3 + i * 4;
        faceSprite.drawLine(sx + i * 2 - 1, yy, sx + i * 2 + 3, yy, DIM_GOLD);
    }
}

// ── Sprite 上画微笑 ──────────────────────────────────
inline void spriteSmile() {
    for (int i = -22; i <= 22; i++) {
        int y = SY(CY) + 35 + (i * i) / 35;
        faceSprite.fillCircle(CX + i, y, 2, GOLD);
    }
}

// ════════════════════════════════════════════════════════
// 待机 — 第一次全画，后续只刷 sprite
// ════════════════════════════════════════════════════════
inline void drawIdle(bool blink = false) {
    initSprite();

    if (!blink) {
        // 全画（状态切换时）
        drawTopBar("\xe5\xb0\xb1\xe7\xbb\xaa", GREEN);  // 就绪
        drawBottomBar("[ \xe6\x8c\x89\xe4\xbd\x8f\xe8\xaf\xb4\xe8\xaf\x9d ]", DIM_GOLD);  // [ 按住说话 ]
    }

    faceSprite.fillSprite(BG);
    if (blink) {
        faceSprite.fillRect(EYE_L_X - EYE_W, SY(EYE_Y) - 1, EYE_W * 2, 3, GOLD);
        faceSprite.fillRect(EYE_R_X - EYE_W, SY(EYE_Y) - 1, EYE_W * 2, 3, GOLD);
    } else {
        spriteWarriorEye(EYE_L_X, EYE_Y);
        spriteWarriorEye(EYE_R_X, EYE_Y);
    }
    spriteScar();
    spriteSmile();
    pushFace();
}

// ════════════════════════════════════════════════════════
// 录音 — sprite 刷新，无闪
// ════════════════════════════════════════════════════════
inline void drawRecording(int frame = 0) {
    initSprite();

    if (frame == 0) {
        drawTopBar("\xe5\xbd\x95\xe9\x9f\xb3", RED);  // 录音
        drawBottomBar("... \xe5\x80\xbe\xe5\x90\xac\xe4\xb8\xad ...", RED);  // ... 倾听中 ...
    }

    faceSprite.fillSprite(BG);
    spriteWarriorEye(EYE_L_X, EYE_Y, 0, 0, true);
    spriteWarriorEye(EYE_R_X, EYE_Y, 0, 0, true);
    spriteScar();

    int mouthH = 10 + (frame % 3) * 4;
    int my = SY(CY) + 38;
    faceSprite.fillEllipse(CX, my, 16, mouthH, RED);
    faceSprite.fillEllipse(CX, my, 10, mouthH > 6 ? mouthH - 4 : 2, BG);

    // 声波
    for (int w = 0; w < 3; w++) {
        int r = 28 + w * 14 + (frame * 6) % 14;
        uint16_t c = (w == frame % 3) ? RED : DARK_GREY;
        for (int a = 20; a <= 160; a += 3) {
            float rad = a * 3.14159f / 180.0f;
            int px = CX + (int)(r * cosf(rad));
            int py = my + (int)(r * sinf(rad) * 0.5f);
            if (py >= 0 && py < SPRITE_H) faceSprite.drawPixel(px, py, c);
        }
    }

    // 红点
    int dotR = 4 + (frame % 2) * 2;
    faceSprite.fillCircle(305, 0, dotR, RED);

    pushFace();
}

// ════════════════════════════════════════════════════════
// 思考 — 磨刀中
// ════════════════════════════════════════════════════════
inline void drawThinking(int frame = 0) {
    initSprite();

    if (frame == 0) {
        drawTopBar("\xe6\x80\x9d\xe8\x80\x83", CYAN);  // 思考
        drawBottomBar("\xe7\xa3\xa8\xe5\x88\x80\xe4\xb8\xad...", CYAN);  // 磨刀中...
    }

    faceSprite.fillSprite(BG);

    float angle = (frame % 12) * 3.14159f / 6.0f;
    int px = (int)(5.0f * cosf(angle));
    int py = (int)(5.0f * sinf(angle));
    spriteWarriorEye(EYE_L_X, EYE_Y, px, py);
    spriteWarriorEye(EYE_R_X, EYE_Y, px, py);
    spriteScar();

    const char* dots[] = {".  ", ".. ", "..."};
    faceSprite.setTextColor(CYAN, BG);
    faceSprite.setTextSize(2);
    faceSprite.setTextFont(2);
    faceSprite.setCursor(CX - 18, SY(CY) + 25);
    faceSprite.print(dots[frame % 3]);

    // 旋转剑
    float sAngle = (frame % 8) * 3.14159f / 4.0f;
    int sx = 280, sy = SY(CY) - 5;
    int len = 15;
    int ex = sx + (int)(len * cosf(sAngle));
    int ey = sy + (int)(len * sinf(sAngle));
    faceSprite.drawLine(sx, sy, ex, ey, CYAN);
    faceSprite.drawLine(sx + 1, sy, ex + 1, ey, CYAN);
    int gx = sx + (int)(6 * cosf(sAngle));
    int gy = sy + (int)(6 * sinf(sAngle));
    faceSprite.drawLine(gx - 3, gy - 3, gx + 3, gy + 3, CYAN);

    pushFace();
}

// ════════════════════════════════════════════════════════
// 说话 — 报告长官
// ════════════════════════════════════════════════════════
inline void drawSpeaking(int frame = 0, const char* text = nullptr) {
    initSprite();

    if (frame == 0) {
        drawTopBar("\xe5\x9b\x9e\xe7\xad\x94", GREEN);  // 回答
        drawBottomBar("\xe6\x8a\xa5\xe5\x91\x8a\xe9\x95\xbf\xe5\xae\x98\xef\xbc\x81", GREEN);  // 报告长官！
    }

    faceSprite.fillSprite(BG);
    spriteWarriorEye(EYE_L_X, EYE_Y, 0, 2);
    spriteWarriorEye(EYE_R_X, EYE_Y, 0, 2);
    spriteScar();

    int mouthOpen = 4 + (frame % 4) * 5;
    int my = SY(CY) + 38;
    faceSprite.fillEllipse(CX, my, 15, mouthOpen, GREEN);
    if (mouthOpen > 8) {
        faceSprite.fillEllipse(CX, my, 10, mouthOpen - 4, BG);
    }

    if (text && strlen(text) > 0) {
        faceSprite.setTextColor(GREEN, BG);
        faceSprite.setFont(&fonts::efontCN_12);
        char buf[64] = {0};
        strncpy(buf, text, sizeof(buf) - 1);
        faceSprite.setCursor(20, SPRITE_H - 30);
        faceSprite.print(buf);
        faceSprite.setFont(nullptr);
    }

    pushFace();
}

// ════════════════════════════════════════════════════════
// 错误 — 负伤
// ════════════════════════════════════════════════════════
inline void drawError(const char* msg = "\xe9\x94\x99\xe8\xaf\xaf") {  // 错误
    initSprite();
    drawTopBar("\xe9\x94\x99\xe8\xaf\xaf", RED);  // 错误
    drawBottomBar(msg, RED);

    faceSprite.fillSprite(BG);
    int s = 14;
    int ley = SY(EYE_Y), rey = SY(EYE_Y);
    faceSprite.drawLine(EYE_L_X - s, ley - s, EYE_L_X + s, ley + s, RED);
    faceSprite.drawLine(EYE_L_X + s, ley - s, EYE_L_X - s, ley + s, RED);
    faceSprite.drawLine(EYE_R_X - s, rey - s, EYE_R_X + s, rey + s, RED);
    faceSprite.drawLine(EYE_R_X + s, rey - s, EYE_R_X - s, rey + s, RED);
    spriteScar();

    int my = SY(CY) + 48;
    for (int i = -18; i <= 18; i++) {
        int y = my - (i * i) / 25;
        faceSprite.fillCircle(CX + i, y, 2, RED);
    }

    // 断剑
    int by = SY(CY) + 60;
    faceSprite.drawLine(CX - 40, by, CX - 20, by - 15, DIM_GOLD);
    faceSprite.drawLine(CX - 12, by - 5, CX - 5, by + 5, DIM_GOLD);

    pushFace();
}

// ════════════════════════════════════════════════════════
// 连接WiFi
// ════════════════════════════════════════════════════════
inline void drawConnecting(int frame = 0) {
    initSprite();

    if (frame == 0) {
        drawTopBar("\xe8\xbf\x9e\xe6\x8e\xa5", GOLD);  // 连接
        drawBottomBar("\xe6\x90\x9c\xe5\xaf\xbb\xe4\xbf\xa1\xe5\x8f\xb7...", GOLD);  // 搜寻信号...
    }

    faceSprite.fillSprite(BG);
    spriteWarriorEye(EYE_L_X, EYE_Y, 0, -3);
    spriteWarriorEye(EYE_R_X, EYE_Y, 0, -3);
    spriteScar();

    int baseY = SY(CY) + 50;
    for (int i = 0; i < 3; i++) {
        uint16_t c = (i <= frame % 4) ? GOLD : DARK_GREY;
        int r = 12 + i * 14;
        for (int a = 200; a <= 340; a += 2) {
            float rad = a * 3.14159f / 180.0f;
            int px = CX + (int)(r * cosf(rad));
            int py = baseY + (int)(r * sinf(rad));
            if (py >= 0 && py < SPRITE_H) {
                faceSprite.drawPixel(px, py, c);
                faceSprite.drawPixel(px, py + 1, c);
            }
        }
    }
    faceSprite.fillCircle(CX, baseY, 4, GOLD);

    pushFace();
}

// ════════════════════════════════════════════════════════
// 启动画面 — 酷炫动态开机动画
// Phase 1: 黑屏 → 火花粒子飞溅
// Phase 2: 双剑从两侧交叉划入 + 碰撞火花
// Phase 3: ClewBuddy 文字逐字闪现
// Phase 4: 战士眼睛睁开 + "苏醒中..."
// ════════════════════════════════════════════════════════

// 粒子系统
struct Particle {
    float x, y, vx, vy;
    uint16_t color;
    int life;
};

inline void drawSplash() {
    M5Canvas splash(&M5.Lcd);
    splash.createSprite(320, 240);
    splash.setColorDepth(16);

    // --- Phase 1: 火花粒子从中心爆发 (0.6s) ---
    const int NUM_PARTICLES = 30;
    Particle particles[NUM_PARTICLES];
    for (int i = 0; i < NUM_PARTICLES; i++) {
        float angle = (float)i / NUM_PARTICLES * 6.2832f;
        float speed = 2.0f + (i % 5) * 1.5f;
        particles[i] = {
            160.0f, 120.0f,
            speed * cosf(angle), speed * sinf(angle),
            (i % 3 == 0) ? GOLD : ((i % 3 == 1) ? (uint16_t)0xFBC0 : DIM_GOLD),
            15 + (i % 8)
        };
    }
    for (int f = 0; f < 15; f++) {
        splash.fillSprite(BG);
        for (int i = 0; i < NUM_PARTICLES; i++) {
            if (particles[i].life <= 0) continue;
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vy += 0.15f;  // gravity
            particles[i].life--;
            int sz = 1 + particles[i].life / 6;
            splash.fillCircle((int)particles[i].x, (int)particles[i].y, sz, particles[i].color);
        }
        splash.pushSprite(0, 0);
        delay(40);
    }

    // --- Phase 2: 双剑从两侧交叉划入 (0.8s) ---
    for (int f = 0; f <= 20; f++) {
        splash.fillSprite(BG);

        // 剩余粒子继续衰减
        for (int i = 0; i < NUM_PARTICLES; i++) {
            if (particles[i].life <= 0) continue;
            particles[i].x += particles[i].vx * 0.5f;
            particles[i].y += particles[i].vy * 0.5f;
            particles[i].life--;
            splash.fillCircle((int)particles[i].x, (int)particles[i].y, 1, particles[i].color);
        }

        float progress = (float)f / 20.0f;
        // easeOutBack for dramatic effect
        float t = progress - 1.0f;
        float ease = t * t * (2.7f * t + 1.7f) + 1.0f;
        if (ease > 1.0f) ease = 1.0f;

        int swordLen = 50;
        // Left sword: comes from left → center
        int lx1 = (int)(-60 + ease * (160 - 30 + 60));  // tip
        int ly1 = (int)(40 + ease * (70 - 30 - 40));
        int lx2 = lx1 - swordLen;
        int ly2 = ly1 - swordLen;
        // Right sword: comes from right → center
        int rx1 = (int)(380 - ease * (380 - 160 - 30));
        int ry1 = (int)(40 + ease * (70 - 30 - 40));
        int rx2 = rx1 + swordLen;
        int ry2 = ry1 - swordLen;

        // Sword glow trail
        if (f > 2) {
            for (int g = 1; g <= 3; g++) {
                int alpha_shift = g * 2;
                uint16_t trail = (g == 1) ? GOLD : ((g == 2) ? DIM_GOLD : DARK_GREY);
                splash.drawLine(lx1+g*3, ly1+g*3, lx2+g*3, ly2+g*3, trail);
                splash.drawLine(rx1-g*3, ry1+g*3, rx2-g*3, ry2+g*3, trail);
            }
        }

        // Main sword blades (thick)
        for (int w = -2; w <= 2; w++) {
            splash.drawLine(lx1+w, ly1, lx2+w, ly2, GOLD);
            splash.drawLine(rx1+w, ry1, rx2+w, ry2, GOLD);
        }

        // Cross guards
        if (ease > 0.5f) {
            int gx1 = (lx1 + lx2) / 2;
            int gy1 = (ly1 + ly2) / 2;
            splash.drawLine(gx1-8, gy1+3, gx1+8, gy1-3, GOLD);
            splash.drawLine(gx1-8, gy1+4, gx1+8, gy1-2, GOLD);
            int gx2 = (rx1 + rx2) / 2;
            int gy2 = (ry1 + ry2) / 2;
            splash.drawLine(gx2-8, gy2-3, gx2+8, gy2+3, GOLD);
            splash.drawLine(gx2-8, gy2-2, gx2+8, gy2+4, GOLD);
        }

        // Impact sparks at the final frames
        if (f >= 18) {
            int sparkCount = (f - 17) * 6;
            for (int s = 0; s < sparkCount; s++) {
                float sa = (float)s / sparkCount * 6.28f;
                int sr = 5 + (f - 17) * 8 + (s % 4) * 3;
                int sx = 160 + (int)(sr * cosf(sa));
                int sy = 55 + (int)(sr * sinf(sa) * 0.6f);
                uint16_t sc = (s % 2) ? GOLD : (uint16_t)0xFFE0;
                splash.fillCircle(sx, sy, 1 + s % 2, sc);
            }
        }

        splash.pushSprite(0, 0);
        delay(40);
    }

    // --- Phase 3: ClewBuddy 文字逐字显现 + glow (0.8s) ---
    const char* title = "ClewBuddy";
    int titleLen = 9;
    // Redraw final swords as background
    auto drawFinalSwords = [&]() {
        int sx = 160, sy = 55;
        for (int w = -2; w <= 2; w++) {
            splash.drawLine(sx-30+w, sy-30, sx+30+w, sy+30, GOLD);
            splash.drawLine(sx+30+w, sy-30, sx-30+w, sy+30, GOLD);
        }
        splash.drawLine(sx-10, sy+8, sx+10, sy+8, GOLD);
        splash.drawLine(sx-10, sy+9, sx+10, sy+9, GOLD);
        // Cross guards
        splash.drawLine(sx-30-4, sy+3, sx-30+12, sy-5, GOLD);
        splash.drawLine(sx+30-12, sy-5, sx+30+4, sy+3, GOLD);
    };

    for (int i = 0; i <= titleLen; i++) {
        splash.fillSprite(BG);
        drawFinalSwords();

        // Glow behind text
        if (i > 0) {
            splash.fillEllipse(160, 115, 70 + i * 3, 12, 0x1082);
        }

        // Draw revealed characters
        splash.setTextColor(GOLD, BG);
        splash.setTextSize(2);
        splash.setTextFont(2);
        char buf[16] = {0};
        strncpy(buf, title, i);
        int textW = i * 15;
        splash.setCursor(160 - titleLen * 15 / 2, 107);
        splash.print(buf);

        // Flash effect on newest character
        if (i > 0) {
            int charX = 160 - titleLen * 15 / 2 + (i - 1) * 15;
            splash.fillRect(charX, 105, 16, 22, GOLD);
            splash.setTextColor(BG, GOLD);
            char single[2] = {title[i-1], 0};
            splash.setCursor(charX, 107);
            splash.print(single);
            splash.setTextColor(GOLD, BG);
        }

        splash.pushSprite(0, 0);
        delay(80);

        // Remove flash (redraw normal)
        if (i > 0 && i <= titleLen) {
            splash.fillSprite(BG);
            drawFinalSwords();
            splash.fillEllipse(160, 115, 70 + i * 3, 12, 0x1082);
            splash.setTextColor(GOLD, BG);
            splash.setTextSize(2);
            splash.setTextFont(2);
            strncpy(buf, title, i);
            splash.setCursor(160 - titleLen * 15 / 2, 107);
            splash.print(buf);
            splash.pushSprite(0, 0);
            delay(30);
        }
    }

    // --- Phase 4: 眼睛睁开 + ClewBuddy (0.8s) ---
    // 布局：眼睛(y=50) → 微笑(y=90) → 剑(y=140) → 文字(y=185)
    int eyeY = 50;
    int smileY = 95;
    int swordCY = 145;
    int textY = 190;

    auto drawSwordsAt = [&](int cy) {
        // 左剑：左上 → 右下（刃身 + 剑柄）
        int lbx1 = 160 - 50, lby1 = cy - 35;  // 剑尖
        int lbx2 = 160 + 20, lby2 = cy + 35;   // 剑柄末端
        // 右剑：右上 → 左下
        int rbx1 = 160 + 50, rby1 = cy - 35;
        int rbx2 = 160 - 20, rby2 = cy + 35;

        // 剑刃 — 粗线 + 高光
        for (int w = -2; w <= 2; w++) {
            splash.drawLine(lbx1+w, lby1, lbx2+w, lby2, GOLD);
            splash.drawLine(rbx1+w, rby1, rbx2+w, rby2, GOLD);
        }
        // 剑刃中心高光
        splash.drawLine(lbx1, lby1, lbx2, lby2, WHITE);
        splash.drawLine(rbx1, rby1, rbx2, rby2, WHITE);

        // 剑尖 — 小三角
        splash.fillTriangle(lbx1-3, lby1+4, lbx1+3, lby1+4, lbx1, lby1-6, GOLD);
        splash.fillTriangle(rbx1-3, rby1+4, rbx1+3, rby1+4, rbx1, rby1-6, GOLD);

        // 护手 (cross guard) — 每把剑刃中段横一道
        int lgx = (lbx1 + lbx2) / 2, lgy = (lby1 + lby2) / 2;
        splash.fillRect(lgx - 12, lgy - 2, 24, 5, GOLD);
        splash.drawRect(lgx - 12, lgy - 2, 24, 5, DIM_GOLD);
        int rgx = (rbx1 + rbx2) / 2, rgy = (rby1 + rby2) / 2;
        splash.fillRect(rgx - 12, rgy - 2, 24, 5, GOLD);
        splash.drawRect(rgx - 12, rgy - 2, 24, 5, DIM_GOLD);

        // 剑柄 — 护手到末端用深金色加粗
        for (int w = -1; w <= 1; w++) {
            splash.drawLine(lgx+w, lgy+3, lbx2+w, lby2, DIM_GOLD);
            splash.drawLine(rgx+w, rgy+3, rbx2+w, rby2, DIM_GOLD);
        }

        // 剑柄末端圆头 (pommel)
        splash.fillCircle(lbx2, lby2, 4, GOLD);
        splash.drawCircle(lbx2, lby2, 4, DIM_GOLD);
        splash.fillCircle(rbx2, rby2, 4, GOLD);
        splash.drawCircle(rbx2, rby2, 4, DIM_GOLD);

        // 交叉点火花
        splash.fillCircle(160, cy, 3, WHITE);
        splash.fillCircle(160, cy, 6, 0x1082);
    };

    for (int f = 0; f <= 12; f++) {
        splash.fillSprite(BG);

        // Eyes opening
        if (f >= 1) {
            int openH = min((f - 1) * 3, (int)EYE_H);

            splash.drawEllipse(EYE_L_X, eyeY, EYE_W + 2, openH + 2, DIM_GOLD);
            if (openH > 4) {
                splash.fillEllipse(EYE_L_X, eyeY, EYE_W, openH, WHITE);
                splash.fillCircle(EYE_L_X, eyeY + 2, min(openH - 2, (int)PUPIL_R), BLACK);
                splash.fillCircle(EYE_L_X - 2, eyeY, 2, WHITE);
            }

            splash.drawEllipse(EYE_R_X, eyeY, EYE_W + 2, openH + 2, DIM_GOLD);
            if (openH > 4) {
                splash.fillEllipse(EYE_R_X, eyeY, EYE_W, openH, WHITE);
                splash.fillCircle(EYE_R_X, eyeY + 2, min(openH - 2, (int)PUPIL_R), BLACK);
                splash.fillCircle(EYE_R_X - 2, eyeY, 2, WHITE);
            }
        }

        // Scar
        if (f >= 4) {
            int scx = EYE_L_X + EYE_W + 5;
            int scy = eyeY - EYE_H - 2;
            splash.drawLine(scx, scy, scx + 8, scy + 14, DIM_GOLD);
            splash.drawLine(scx + 1, scy, scx + 9, scy + 14, DIM_GOLD);
        }

        // Smile
        if (f >= 5) {
            for (int i = -22; i <= 22; i++) {
                int y = smileY + (i * i) / 35;
                splash.fillCircle(CX + i, y, 2, GOLD);
            }
        }

        // Crossed swords below face
        if (f >= 3) {
            drawSwordsAt(swordCY);
        }

        // ClewBuddy title
        if (f >= 6) {
            splash.setTextColor(GOLD, BG);
            splash.setTextSize(2);
            splash.setTextFont(2);
            splash.setCursor(160 - titleLen * 15 / 2, textY);
            splash.print(title);
        }

        // 苏醒中...
        if (f >= 9) {
            splash.setTextColor(DARK_GREY, BG);
            splash.setFont(&fonts::efontCN_14);
            splash.setCursor(125, 218);
            splash.print("\xe8\x8b\x8f\xe9\x86\x92\xe4\xb8\xad...");
            splash.setFont(nullptr);
        }

        splash.pushSprite(0, 0);
        delay(80);
    }

    // Hold final frame
    delay(300);
    splash.deleteSprite();
}

// Public helpers used by main.cpp for partial redraws
// (same as sprite versions but accessible)

} // namespace Face
