#include "screens/map_select.h"
#include "core/app.h"
#include "core/engine.h"
#include "core/input.h"
#include "ui/primitives.h"
#include "ui/theme.h"
#include "ui/ui_assets.h"
#include "game/game_state.h"


void MapSelectScreen::enter(App& app) {
    hoveredIndex_ = -1;
    selectedMap.clear();
    scenarios_.clear();


    auto& eng = Engine::instance();
    std::string startsDir = eng.assetsPath + "starts/";

    if (fs::exists(startsDir)) {
        for (auto& entry : fs::directory_iterator(startsDir)) {
            if (!entry.is_directory()) continue;

            ScenarioInfo info;
            info.name = entry.path().filename().string();


            std::string creatorPath = entry.path().string() + "/creator.txt";
            if (fs::exists(creatorPath)) {
                std::ifstream file(creatorPath);
                if (file.is_open()) {
                    std::getline(file, info.creator);
                }
            }
            std::string datePath = entry.path().string() + "/startDate.txt";
            if (fs::exists(datePath)) {
                std::ifstream file(datePath);
                if (file.is_open()) {
                    std::getline(file, info.startDate);
                }
            }

            if (info.creator.empty()) {
                std::string infoPath = entry.path().string() + "/info.txt";
                if (fs::exists(infoPath)) {
                    std::ifstream file(infoPath);
                    if (file.is_open()) {
                        std::getline(file, info.creator);
                        std::getline(file, info.startDate);
                    }
                }
            }

            if (info.creator.empty()) info.creator = "Unknown";
            if (info.startDate.empty()) info.startDate = "";

            scenarios_.push_back(info);
        }
    }

    std::sort(scenarios_.begin(), scenarios_.end(),
              [](const ScenarioInfo& a, const ScenarioInfo& b) {
                  return a.name < b.name;
              });
}


void MapSelectScreen::handleInput(App& app, const InputState& input) {
    auto& eng = Engine::instance();
    int W = eng.WIDTH;
    int H = eng.HEIGHT;
    int mx = input.mouseX;
    int my = input.mouseY;

    if (input.mouseLeftDown) {
        float winW = Theme::s(16.0f);
        float winH = Theme::s(18.0f);
        float winX = W * 0.5f - winW * 0.5f;
        float winY = H * 0.15f;

        float bw = Theme::s(14.0f);
        float bh = Theme::s(3.0f);
        float bx = W * 0.5f - bw * 0.5f;
        float startY = winY + Theme::s(3.5f);
        float gap = Theme::s(3.5f);

        for (int i = 0; i < static_cast<int>(scenarios_.size()); i++) {
            float by = startY + i * gap;
            if (by + bh < winY + Theme::s(3.0f) || by > winY + winH - Theme::s(4.0f)) continue;

            SDL_Rect btnRect = {
                static_cast<int>(bx), static_cast<int>(by),
                static_cast<int>(bw), static_cast<int>(bh)
            };
            SDL_Point pt = {mx, my};
            if (SDL_PointInRect(&pt, &btnRect)) {
                selectedMap = scenarios_[i].name;

                app.gameState().mapName = selectedMap;
                nextScreen = ScreenType::COUNTRY_SELECT;
            }
        }


        float backY = winY + winH - Theme::s(3.0f);
        SDL_Rect backRect = {
            static_cast<int>(bx), static_cast<int>(backY),
            static_cast<int>(bw), static_cast<int>(bh)
        };
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &backRect)) {
            nextScreen = ScreenType::MAIN_MENU;
        }
    }


    if (input.isKeyDown(SDL_SCANCODE_ESCAPE)) {
        nextScreen = ScreenType::MAIN_MENU;
    }
}


void MapSelectScreen::update(App& app, float dt) {

}


void MapSelectScreen::render(App& app) {
    auto& eng = Engine::instance();
    int W = eng.WIDTH, H = eng.HEIGHT;
    float u = H / 100.0f;
    auto* r = eng.renderer;
    int mx, my; SDL_GetMouseState(&mx, &my);

    eng.clear(Theme::bg);
    UIPrim::drawVignette(r, W, H, 80);


    float winW = std::round(W * 0.48f);
    winW = std::clamp(winW, 550.0f, 1250.0f);
    float winH = std::round(H * 0.70f);
    float winX = std::round((W - winW) / 2.0f);
    float winY = std::round((H - winH) / 2.0f);
    float pad = std::round(u * 1.8f);
    float headerH = std::round(u * 5.0f);


    auto& assets = UIAssets::instance();
    for (int s = 20; s >= 1; s--) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, (uint8_t)std::min(255, (int)(80.0f * s / 20.0f * 0.4f)));
        SDL_Rect sh = {(int)winX - s/2, (int)winY + s, (int)winW + s, (int)winH};
        SDL_RenderFillRect(r, &sh);
    }
    UIPrim::drawRoundedRect(r, {18, 20, 26}, winX, winY, winW, winH, 8);
    SDL_Texture* windowTex = assets.panelBodyHeaded();
    if (!windowTex) windowTex = assets.panelBodyHeadless();
    if (windowTex) UIAssets::draw9Slice(r, windowTex, winX, winY, winW, winH, 28);

    SDL_Texture* headerTex = assets.panelHeader();
    if (headerTex) UIAssets::draw9Slice(r, headerTex, winX + 3, winY + 3, winW - 6, headerH, 14);
    int titleFs = std::max(16, (int)(headerH * 0.42f));
    UIPrim::drawText(r, "SELECT SCENARIO", titleFs, winX + winW / 2, winY + headerH * 0.5f, "center", Theme::gold_bright, true);
    UIPrim::drawHLine(r, Theme::gold, winX + 8, winX + winW - 8, winY + headerH + 2, 2);


    float footerH = std::round(u * 6.5f);
    float contentY = winY + headerH + pad;
    float contentH = winH - headerH - pad - footerH;
    float wellX = winX + pad, wellW = winW - pad * 2;

    UIPrim::drawRoundedRect(r, {10, 12, 16}, wellX, contentY, wellW, contentH, 6);
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 40);
    SDL_Rect wb = {(int)wellX, (int)contentY, (int)wellW, (int)contentH};
    SDL_RenderDrawRect(r, &wb);


    float bh = std::round(u * 5.5f);
    float gap = std::round(u * 0.8f);
    float bx = wellX + pad * 0.5f;
    float bw = wellW - pad;

    if (scenarios_.empty()) {
        int emFs = std::max(14, (int)(u * 1.3f));
        UIPrim::drawText(r, "No scenarios found", emFs, wellX + wellW / 2, contentY + contentH * 0.4f, "center", Theme::grey);
        int subFs = std::max(12, (int)(u * 1.0f));
        UIPrim::drawText(r, "Place scenario folders in starts/", subFs, wellX + wellW / 2, contentY + contentH * 0.52f, "center", Theme::dark_grey);
    }

    SDL_Rect clipScen = {(int)wellX, (int)contentY, (int)wellW, (int)contentH};
    SDL_RenderSetClipRect(r, &clipScen);

    float startY = contentY + pad * 0.5f;
    int nameFs = std::max(14, (int)(bh * 0.30f));
    int subFs = std::max(12, (int)(bh * 0.22f));

    for (int i = 0; i < (int)scenarios_.size(); i++) {
        float by = startY + i * (bh + gap);
        if (by + bh < contentY || by > contentY + contentH) continue;

        bool hovered = (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh);


        SDL_Texture* btnTex = hovered ? assets.btnRectHover() : assets.btnRectDefault();
        if (btnTex) {
            SDL_SetTextureColorMod(btnTex, hovered ? 200 : 160, hovered ? 198 : 158, hovered ? 195 : 155);
            UIAssets::draw9Slice(r, btnTex, bx, by, bw, bh, 14);
            SDL_SetTextureColorMod(btnTex, 255, 255, 255);
        }


        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, Theme::gold.r, Theme::gold.g, Theme::gold.b, hovered ? 220 : 80);
        SDL_Rect lbar = {(int)bx + 4, (int)by + 6, 4, (int)bh - 12};
        SDL_RenderFillRect(r, &lbar);


        Color nameC = hovered ? Theme::gold_bright : Theme::cream;
        UIPrim::drawText(r, scenarios_[i].name, nameFs, bx + std::round(u * 1.5f), by + bh * 0.32f, "midleft", nameC, true);


        if (!scenarios_[i].creator.empty()) {
            UIPrim::drawText(r, "by " + scenarios_[i].creator, subFs, bx + std::round(u * 1.5f), by + bh * 0.68f, "midleft", Theme::grey);
        }
        if (!scenarios_[i].startDate.empty()) {
            UIPrim::drawText(r, scenarios_[i].startDate, subFs, bx + bw - std::round(u * 1.5f), by + bh * 0.5f, "midright", Theme::gold_dim);
        }
    }
    SDL_RenderSetClipRect(r, nullptr);


    float footerY = winY + winH - footerH;
    UIPrim::drawHLine(r, {Theme::gold_dim.r, Theme::gold_dim.g, Theme::gold_dim.b, 40}, winX + pad, winX + winW - pad, footerY, 1);
    float btnH2 = std::round(u * 4.8f);
    float btnW2 = std::round(winW * 0.30f);
    float fbx = winX + (winW - btnW2) / 2;
    float fby = footerY + std::round(u * 0.8f);
    UIPrim::drawMenuButton(r, fbx, fby, btnW2, btnH2, "Back", mx, my, false);

    UIPrim::drawOrnamentalFrame(r, winX, winY, winW, winH, 2);
}
