#include "screens/settings_screen.h"
#include "core/app.h"
#include "core/engine.h"
#include "core/audio.h"
#include "core/input.h"
#include "ui/primitives.h"
#include "ui/theme.h"
#include "ui/ui_assets.h"
#include <sstream>
#include <iomanip>
#include <algorithm>


void SettingsScreen::enter(App& app) {
    auto& s = app.settings();
    tempUISize_ = s.uiSize;
    tempMusicVol_ = s.musicVolume;
    tempSoundVol_ = s.soundVolume;
    tempFPS_ = s.fps;
    tempFog_ = s.fogOfWar;
    hoveredItem_ = -1;
}


struct SettingsLayout {
    float winW, winH, winX, winY;
    float headerH;
    float rowH, rowGap, rowStartY;
    float labelX, sliderX, sliderW, sliderH, valueX;
    float btnW, btnH, btnX, btnY;
    float u, pad;

    void compute(int W, int H) {
        u = H / 100.0f;
        pad = std::round(u * 2.0f);

        winW = std::round(W * 0.42f);
        winW = std::clamp(winW, 550.0f, 1100.0f);
        winH = std::round(H * 0.62f);
        winX = std::round((W - winW) * 0.5f);
        winY = std::round((H - winH) * 0.5f);

        headerH = std::round(u * 5.0f);
        float contentH = winH - headerH;
        float footerH = std::round(u * 6.5f);


        float usableH = contentH - footerH - pad;
        rowH = std::min(std::round(u * 5.5f), usableH / 5.5f);
        rowGap = (usableH - 5 * rowH) / 6.0f;
        rowStartY = winY + headerH + pad + rowGap;

        labelX = winX + pad + std::round(u * 3.5f);
        sliderW = winW * 0.42f;
        sliderH = std::max(10.0f, std::round(u * 1.0f));
        sliderX = winX + winW * 0.35f;
        valueX = winX + winW - pad;

        btnH = std::round(u * 4.8f);
        btnW = std::round(winW * 0.40f);
        btnX = winX + (winW - btnW) * 0.5f;
        btnY = winY + winH - std::round(u * 5.8f);
    }

    float rowY(int i) const { return rowStartY + i * (rowH + rowGap); }
    float sliderBarY(int i) const { return rowY(i) + rowH * 0.58f; }
};


void SettingsScreen::handleInput(App& app, const InputState& input) {
    auto& eng = Engine::instance();
    int W = eng.WIDTH, H = eng.HEIGHT;
    int mx = input.mouseX, my = input.mouseY;

    SettingsLayout L;
    L.compute(W, H);


    if (input.mouseLeft) {
        float hitMargin = L.sliderH * 2.0f;

        for (int i = 0; i < 4; i++) {
            float sy = L.sliderBarY(i);
            if (mx >= L.sliderX && mx <= L.sliderX + L.sliderW &&
                my >= sy - hitMargin && my <= sy + L.sliderH + hitMargin) {
                float pct = std::clamp((mx - L.sliderX) / L.sliderW, 0.0f, 1.0f);
                switch (i) {
                    case 0: tempUISize_   = 16.0f + pct * 32.0f; break;
                    case 1: tempMusicVol_ = pct; break;
                    case 2: tempSoundVol_ = pct; break;
                    case 3: tempFPS_ = static_cast<int>(30.0f + pct * 114.0f);
                            tempFPS_ = std::clamp(tempFPS_, 30, 144); break;
                }
            }
        }
    }


    if (input.mouseLeftDown) {
        float fogY = L.rowY(4);
        float toggleX = L.sliderX;
        float toggleW = L.sliderW * 0.35f;
        float toggleH = L.rowH * 0.6f;
        float toggleY = fogY + L.rowH * 0.15f;
        if (mx >= toggleX && mx <= toggleX + toggleW &&
            my >= toggleY && my <= toggleY + toggleH) {
            tempFog_ = !tempFog_;
            Audio::instance().playSound("clickedSound");
        }
    }


    if (input.mouseLeftDown) {
        if (mx >= L.btnX && mx <= L.btnX + L.btnW &&
            my >= L.btnY && my <= L.btnY + L.btnH) {
            auto& s = app.settings();
            s.uiSize = tempUISize_;
            s.musicVolume = tempMusicVol_;
            s.soundVolume = tempSoundVol_;
            s.fps = tempFPS_;
            s.fogOfWar = tempFog_;

            Theme::setScale(s.uiSize);
            Audio::instance().setMusicVolume(s.musicVolume);
            Audio::instance().setSoundVolume(s.soundVolume);
            eng.FPS = s.fps;
            eng.recalcUI();
            s.save(eng.basePath + "settings.json");

            nextScreen = ScreenType::MAIN_MENU;
        }
    }

    if (input.isKeyDown(SDL_SCANCODE_ESCAPE)) {
        nextScreen = ScreenType::MAIN_MENU;
    }
}


void SettingsScreen::update(App& app, float dt) {}


void SettingsScreen::render(App& app) {
    auto& eng = Engine::instance();
    int W = eng.WIDTH, H = eng.HEIGHT;
    auto* r = eng.renderer;

    eng.clear(Theme::bg);
    UIPrim::drawVignette(r, W, H, 120);

    SettingsLayout L;
    L.compute(W, H);
    auto& assets = UIAssets::instance();

    int mx, my;
    SDL_GetMouseState(&mx, &my);


    for (int s = 20; s >= 1; s--) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, (uint8_t)std::min(255, (int)(80.0f * s / 20.0f * 0.4f)));
        SDL_Rect sh = {(int)L.winX - s/2, (int)L.winY + s, (int)L.winW + s, (int)L.winH};
        SDL_RenderFillRect(r, &sh);
    }
    UIPrim::drawRoundedRect(r, {18, 20, 26}, L.winX, L.winY, L.winW, L.winH, 8);
    SDL_Texture* windowTex = assets.panelBodyHeaded();
    if (!windowTex) windowTex = assets.panelBodyHeadless();
    if (windowTex) UIAssets::draw9Slice(r, windowTex, L.winX, L.winY, L.winW, L.winH, 28);


    SDL_Texture* headerTex = assets.panelHeader();
    if (headerTex) UIAssets::draw9Slice(r, headerTex, L.winX + 3, L.winY + 3, L.winW - 6, L.headerH, 14);
    int titleFs = std::max(16, (int)(L.headerH * 0.42f));
    UIPrim::drawText(r, "SETTINGS", titleFs, L.winX + L.winW / 2, L.winY + L.headerH * 0.5f, "center", Theme::gold_bright, true);
    UIPrim::drawHLine(r, Theme::gold, L.winX + 8, L.winX + L.winW - 8, L.winY + L.headerH + 2, 2);


    float wellX = L.winX + L.pad;
    float wellY = L.winY + L.headerH + L.pad * 0.5f;
    float wellW = L.winW - L.pad * 2;
    float wellH = L.btnY - wellY - L.pad * 0.5f;
    UIPrim::drawRoundedRect(r, {10, 12, 16}, wellX, wellY, wellW, wellH, 6);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 40);
    SDL_Rect wellBrd = {(int)wellX, (int)wellY, (int)wellW, (int)wellH};
    SDL_RenderDrawRect(r, &wellBrd);

    UIPrim::drawOrnamentalFrame(r, L.winX, L.winY, L.winW, L.winH, 2);


    struct SliderRow {
        const char* label;
        float value, minV, maxV;
        bool isInt;
    };
    SliderRow rows[] = {
        {"UI Size",       tempUISize_,                  16.0f, 48.0f,  false},
        {"Music Volume",  tempMusicVol_,                0.0f,  1.0f,   false},
        {"Sound Volume",  tempSoundVol_,                0.0f,  1.0f,   false},
        {"FPS",           static_cast<float>(tempFPS_), 30.0f, 144.0f, true},
    };

    int labelFs = std::max(13, static_cast<int>(H / 55.0f));
    int valueFs = std::max(12, static_cast<int>(H / 60.0f));

    const char* rowIcons[] = {"Levels", "Music-On", "Sound-Three", "Gear"};
    for (int i = 0; i < 4; i++) {
        auto& row = rows[i];
        float ry = L.rowY(i);
        float sy = L.sliderBarY(i);
        float pct = std::clamp((row.value - row.minV) / (row.maxV - row.minV), 0.0f, 1.0f);


        auto& settingsAssets = UIAssets::instance();
        SDL_Texture* rowIcon = settingsAssets.icon(rowIcons[i]);
        float iconSz = L.rowH * 0.55f;
        if (rowIcon) {
            SDL_SetTextureColorMod(rowIcon, 210, 200, 172);
            SDL_Rect iDst = {(int)(L.labelX - iconSz - 6), (int)(ry + L.rowH * 0.3f - iconSz / 2), (int)iconSz, (int)iconSz};
            SDL_RenderCopy(r, rowIcon, nullptr, &iDst);
            SDL_SetTextureColorMod(rowIcon, 255, 255, 255);
        }


        UIPrim::drawText(r, row.label, labelFs,
                         L.labelX, ry + L.rowH * 0.3f, "midleft", Theme::cream);


        auto& assets = UIAssets::instance();
        SDL_Texture* progBg = assets.progressBg();
        SDL_Texture* progFill = assets.progressFill();
        if (progBg) {
            UIAssets::draw9Slice(r, progBg, L.sliderX, sy - 2, L.sliderW, L.sliderH + 4, 8);
        } else {
            UIPrim::drawRoundedRect(r, Theme::slot, L.sliderX, sy, L.sliderW, L.sliderH, 3);
        }


        if (pct > 0.01f) {
            float fillW = L.sliderW * pct;
            if (progFill) {
                SDL_SetTextureColorMod(progFill, 200, 180, 120);
                UIAssets::draw9Slice(r, progFill, L.sliderX + 3, sy, fillW - 3, L.sliderH, 6);
                SDL_SetTextureColorMod(progFill, 255, 255, 255);
            } else {
                UIPrim::drawRoundedRect(r, Theme::gold_dim, L.sliderX, sy, fillW, L.sliderH, 3);
            }
        }


        float knobW = std::max(6.0f, L.sliderH * 1.8f);
        float knobH = L.sliderH * 2.2f;
        float knobX = L.sliderX + L.sliderW * pct - knobW * 0.5f;
        float knobY = sy - (knobH - L.sliderH) * 0.5f;
        UIPrim::drawRoundedRect(r, Theme::gold_bright, knobX, knobY, knobW, knobH, 3, Theme::gold);


        std::ostringstream oss;
        if (row.isInt) {
            oss << static_cast<int>(row.value);
        } else if (row.maxV <= 1.0f) {
            oss << static_cast<int>(row.value * 100) << "%";
        } else {
            oss << std::fixed << std::setprecision(1) << row.value;
        }
        UIPrim::drawText(r, oss.str(), valueFs,
                         L.valueX, ry + L.rowH * 0.3f, "midright", Theme::gold);
    }


    {
        float fogY = L.rowY(4);
        float toggleX = L.sliderX;
        float toggleW = L.sliderW * 0.35f;
        float toggleH = L.rowH * 0.6f;
        float toggleY = fogY + L.rowH * 0.15f;


        auto& fogAssets = UIAssets::instance();
        const char* fogIconName = tempFog_ ? "Locker-Locked" : "Locker-Unlocked";
        SDL_Texture* fogIcon = fogAssets.icon(fogIconName);
        float fogIconSz = L.rowH * 0.55f;
        if (fogIcon) {
            SDL_SetTextureColorMod(fogIcon, 210, 200, 172);
            SDL_Rect fiDst = {(int)(L.labelX - fogIconSz - 6), (int)(fogY + L.rowH * 0.3f - fogIconSz / 2), (int)fogIconSz, (int)fogIconSz};
            SDL_RenderCopy(r, fogIcon, nullptr, &fiDst);
            SDL_SetTextureColorMod(fogIcon, 255, 255, 255);
        }


        UIPrim::drawText(r, "Fog of War", labelFs,
                         L.labelX, fogY + L.rowH * 0.3f, "midleft", Theme::cream);


        bool fogHov = (mx >= toggleX && mx <= toggleX + toggleW &&
                       my >= toggleY && my <= toggleY + toggleH);
        Color toggleBg = tempFog_ ? Theme::gold_dim : Theme::slot;
        Color toggleBorder = tempFog_ ? Theme::gold : Theme::border;
        if (fogHov) toggleBg = tempFog_ ? Theme::gold_bright : Theme::btn_hover;

        UIPrim::drawRoundedRect(r, toggleBg, toggleX, toggleY, toggleW, toggleH, 4, toggleBorder);

        const char* fogText = tempFog_ ? "ON" : "OFF";
        Color fogTextColor = tempFog_ ? Theme::cream : Theme::grey;
        UIPrim::drawText(r, fogText, valueFs,
                         toggleX + toggleW * 0.5f, toggleY + toggleH * 0.5f,
                         "center", fogTextColor);
    }


    bool btnHov = mx >= L.btnX && mx <= L.btnX + L.btnW &&
                  my >= L.btnY && my <= L.btnY + L.btnH;
    bool btnDown = btnHov && (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK);
    UIPrim::drawMenuButton(r, L.btnX, L.btnY, L.btnW, L.btnH,
                           "Apply & Back", mx, my, btnDown);
}
