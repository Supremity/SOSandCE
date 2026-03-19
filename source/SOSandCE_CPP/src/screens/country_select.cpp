#include "screens/country_select.h"
#include "core/app.h"
#include "core/engine.h"
#include "core/input.h"
#include "ui/primitives.h"
#include "ui/theme.h"
#include "ui/ui_assets.h"
#include "data/data_loader.h"
#include "game/game_state.h"


void CountrySelectScreen::enter(App& app) {
    scrollOffset_ = 0;
    hoveredIndex_ = -1;
    searchFilter_.clear();
    selectedCountry.clear();


    countries_.clear();
    try {
        json records = DataLoader::getCountryRecords();
        if (records.is_array()) {
            for (auto& rec : records) {
                if (rec.contains("name") && rec["name"].is_string()) {
                    countries_.push_back(rec["name"].get<std::string>());
                }
            }
        } else if (records.is_object()) {
            for (auto& [key, val] : records.items()) {
                countries_.push_back(key);
            }
        }
    } catch (...) {

    }
    std::sort(countries_.begin(), countries_.end());
}


void CountrySelectScreen::handleInput(App& app, const InputState& input) {
    auto& eng = Engine::instance();
    int W = eng.WIDTH;
    int H = eng.HEIGHT;
    int mx = input.mouseX;
    int my = input.mouseY;


    if (input.scrollY != 0) {
        scrollOffset_ -= input.scrollY * static_cast<int>(Theme::s(2.5f));
        if (scrollOffset_ < 0) scrollOffset_ = 0;
    }


    if (input.hasTextInput) {
        searchFilter_ += input.textInput;
        scrollOffset_ = 0;
    }
    if (input.isKeyDown(SDL_SCANCODE_BACKSPACE) && !searchFilter_.empty()) {
        searchFilter_.pop_back();
        scrollOffset_ = 0;
    }


    std::vector<std::string> filtered;
    for (auto& c : countries_) {
        if (searchFilter_.empty()) {
            filtered.push_back(c);
        } else {

            std::string lowerName = c;
            std::string lowerFilter = searchFilter_;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
            if (lowerName.find(lowerFilter) != std::string::npos) {
                filtered.push_back(c);
            }
        }
    }


    if (input.mouseLeftDown) {
        float winW = Theme::s(16.0f);
        float winH = Theme::s(18.0f);
        float winX = W * 0.5f - winW * 0.5f;
        float winY = H * 0.15f;

        float bw = Theme::s(14.0f);
        float bh = Theme::s(2.0f);
        float bx = W * 0.5f - bw * 0.5f;
        float startY = winY + Theme::s(5.0f) - scrollOffset_;
        float gap = Theme::s(2.4f);


        for (int i = 0; i < static_cast<int>(filtered.size()); i++) {
            float by = startY + i * gap;
            if (by < winY + Theme::s(4.0f) || by > winY + winH - Theme::s(4.0f)) continue;

            SDL_Rect btnRect = {
                static_cast<int>(bx), static_cast<int>(by),
                static_cast<int>(bw), static_cast<int>(bh)
            };
            SDL_Point pt = {mx, my};
            if (SDL_PointInRect(&pt, &btnRect)) {
                selectedCountry = filtered[i];
            }
        }


        float backY = winY + winH - Theme::s(3.0f);
        SDL_Rect backRect = {
            static_cast<int>(bx), static_cast<int>(backY),
            static_cast<int>(bw * 0.45f), static_cast<int>(bh)
        };
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &backRect)) {
            nextScreen = ScreenType::MAP_SELECT;
        }


        SDL_Rect startRect = {
            static_cast<int>(bx + bw * 0.55f), static_cast<int>(backY),
            static_cast<int>(bw * 0.45f), static_cast<int>(bh)
        };
        if (SDL_PointInRect(&pt, &startRect) && !selectedCountry.empty()) {

            app.gameState().controlledCountry = selectedCountry;
            app.gameState().inGame = false;
            nextScreen = ScreenType::GAME;
        }
    }


    if (input.isKeyDown(SDL_SCANCODE_ESCAPE)) {
        nextScreen = ScreenType::MAP_SELECT;
    }
}


void CountrySelectScreen::update(App& app, float dt) {

}


void CountrySelectScreen::render(App& app) {
    auto& eng = Engine::instance();
    int W = eng.WIDTH;
    int H = eng.HEIGHT;
    auto* r = eng.renderer;
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    float u = H / 100.0f;
    eng.clear(Theme::bg);
    UIPrim::drawVignette(r, W, H, 100);

    auto& assets = UIAssets::instance();
    float winW = std::round(W * 0.42f);
    winW = std::clamp(winW, 500.0f, 1100.0f);
    float winH = std::round(H * 0.70f);
    float winX = std::round((W - winW) / 2.0f);
    float winY = std::round((H - winH) / 2.0f);
    float pad = std::round(u * 1.8f);
    float headerH = std::round(u * 5.0f);


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

    SDL_Texture* headerTex2 = assets.panelHeader();
    if (headerTex2) UIAssets::draw9Slice(r, headerTex2, winX + 3, winY + 3, winW - 6, headerH, 14);
    int titleFs = std::max(16, (int)(headerH * 0.42f));
    UIPrim::drawText(r, "SELECT COUNTRY", titleFs, winX + winW / 2, winY + headerH * 0.5f, "center", Theme::gold_bright, true);
    UIPrim::drawHLine(r, Theme::gold, winX + 8, winX + winW - 8, winY + headerH + 2, 2);
    UIPrim::drawOrnamentalFrame(r, winX, winY, winW, winH, 2);


    float searchY = winY + headerH + pad;
    float searchW = winW - pad * 2;
    float searchH = std::round(u * 3.2f);
    float searchX = winX + pad;

    UIPrim::drawRoundedRect(r, {10, 12, 16}, searchX, searchY, searchW, searchH, 4, Theme::border);

    std::string searchText = searchFilter_.empty() ? "Search..." : searchFilter_ + "_";
    Color searchColor = searchFilter_.empty() ? Theme::dark_grey : Theme::cream;
    UIPrim::drawText(r, searchText, Theme::si(0.9f),
                     searchX + Theme::s(0.5f), searchY + searchH * 0.5f,
                     "midleft", searchColor);


    std::vector<std::string> filtered;
    for (auto& c : countries_) {
        if (searchFilter_.empty()) {
            filtered.push_back(c);
        } else {
            std::string lowerName = c;
            std::string lowerFilter = searchFilter_;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
            if (lowerName.find(lowerFilter) != std::string::npos) {
                filtered.push_back(c);
            }
        }
    }


    float bw = Theme::s(14.0f);
    float bh = Theme::s(2.0f);
    float bx = W * 0.5f - bw * 0.5f;
    float startY = winY + Theme::s(5.5f) - scrollOffset_;
    float gap = Theme::s(2.4f);


    float clipTop = winY + Theme::s(5.0f);
    float clipBottom = winY + winH - Theme::s(4.0f);

    for (int i = 0; i < static_cast<int>(filtered.size()); i++) {
        float by = startY + i * gap;
        if (by + bh < clipTop || by > clipBottom) continue;

        bool isSelected = (filtered[i] == selectedCountry);
        std::string label = filtered[i];
        if (isSelected) label = "> " + label;

        UIPrim::drawMenuButton(r, bx, by, bw, bh,
                                label, mx, my, false);

        if (isSelected) {
            UIPrim::drawRect(r, Theme::gold, bx, by, bw, bh,
                             Theme::gold, 0, 1);
        }
    }


    int maxScroll = std::max(0, static_cast<int>(filtered.size() * gap - (clipBottom - clipTop)));
    if (scrollOffset_ > maxScroll) scrollOffset_ = maxScroll;


    float backY = winY + winH - Theme::s(3.0f);
    UIPrim::drawMenuButton(r, bx, backY, bw * 0.45f, bh + Theme::s(0.3f),
                           "Back", mx, my, false);

    bool canStart = !selectedCountry.empty();
    float startBtnX = bx + bw * 0.55f;
    float startBtnW = bw * 0.45f;
    float startBtnH = bh + Theme::s(0.3f);
    bool startHov = canStart && mx >= startBtnX && mx <= startBtnX + startBtnW &&
                    my >= backY && my <= backY + startBtnH;

    if (canStart) {
        Color startBg = startHov ? Color{55, 120, 70} : Theme::btn_confirm;
        UIPrim::drawRoundedRect(r, startBg, startBtnX, backY, startBtnW, startBtnH, 5, Theme::green);

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 255, 255, 255, startHov ? 12 : 6);
        SDL_Rect hl = {(int)startBtnX + 3, (int)backY + 2, (int)startBtnW - 6, (int)(startBtnH * 0.3f)};
        SDL_RenderFillRect(r, &hl);
    } else {
        UIPrim::drawRoundedRect(r, Theme::btn_disabled, startBtnX, backY, startBtnW, startBtnH, 5, Theme::border);
    }
    Color startTc = canStart ? (startHov ? Theme::gold_bright : Theme::cream) : Theme::dark_grey;
    UIPrim::drawText(r, "Start", Theme::si(1.0f),
                     startBtnX + startBtnW * 0.5f, backY + startBtnH * 0.5f,
                     "center", startTc, canStart);
}
