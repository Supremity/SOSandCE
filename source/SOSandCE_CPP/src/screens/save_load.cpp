#include "screens/save_load.h"
#include "screens/game_screen.h"
#include "core/app.h"
#include "core/engine.h"
#include "core/audio.h"
#include "core/input.h"
#include "ui/primitives.h"
#include "ui/theme.h"
#include "ui/toast.h"
#include "ui/ui_assets.h"
#include "save/save_system.h"


void SaveLoadScreen::enter(App& app) {
    hoveredIndex_ = -1;
    newSaveName_.clear();
    enteringName_ = false;
    saves_.clear();
    printf("[SaveLoadScreen] enter mode=%s return=%d\n",
           mode == Mode::SAVE ? "SAVE" : "LOAD",
           static_cast<int>(returnScreen));
    fflush(stdout);

    SaveSystem saveSystem;
    for (const auto& info : saveSystem.listSaves()) {
        SaveEntry se;
        se.name = info.name;
        se.date = info.date;
        se.exists = info.valid;
        saves_.push_back(se);
    }

    std::sort(saves_.begin(), saves_.end(),
              [](const SaveEntry& a, const SaveEntry& b) {
                  return a.date > b.date;
              });
}


void SaveLoadScreen::handleInput(App& app, const InputState& input) {
    auto& eng = Engine::instance();
    int W = eng.WIDTH;
    int H = eng.HEIGHT;
    int mx = input.mouseX;
    int my = input.mouseY;

    auto saveCurrentGame = [&]() {
        printf("[SaveLoadScreen] saveCurrentGame name='%s'\n", newSaveName_.c_str());
        fflush(stdout);
        if (newSaveName_.empty()) {
            toasts().show("Enter a save name first");
            return;
        }

        SaveSystem saver;
        auto* gameScreen = dynamic_cast<GameScreen*>(app.screen(ScreenType::GAME));
        if (gameScreen) {
            bool ok = saver.saveGame(app.gameState(), gameScreen->mapManager(), newSaveName_);
            if (ok) toasts().show("Game saved: " + newSaveName_);
            else toasts().show("Save failed!");
            printf("[SaveLoadScreen] save result=%d\n", ok ? 1 : 0);
            fflush(stdout);
        } else {
            toasts().show("Cannot save outside of game");
            printf("[SaveLoadScreen] save failed: game screen unavailable\n");
            fflush(stdout);
        }

        if (enteringName_) SDL_StopTextInput();
        enteringName_ = false;
        nextScreen = returnScreen;
    };

    float winW = std::round(W * 0.45f);
    winW = std::clamp(winW, 550.0f, 1200.0f);
    float winH = std::round(H * 0.70f);
    float winX = std::round((W - winW) / 2.0f);
    float winY = std::round((H - winH) / 2.0f);

    if (mode == Mode::SAVE && enteringName_) {
        if (input.hasTextInput) {
            newSaveName_ += input.textInput;
            if (newSaveName_.size() > 32) newSaveName_.resize(32);
        }
        if (input.isKeyDown(SDL_SCANCODE_BACKSPACE) && !newSaveName_.empty()) {
            newSaveName_.pop_back();
        }
        if (input.isKeyDown(SDL_SCANCODE_RETURN) && !newSaveName_.empty()) {
            saveCurrentGame();
        }
    }

    float bw = Theme::s(14.0f);
    float bh = Theme::s(2.2f);
    float bx = W * 0.5f - bw * 0.5f;

    if (input.mouseLeftDown) {

        if (mode == Mode::LOAD) {
            float startY = winY + Theme::s(3.5f);
            float gap = Theme::s(2.8f);
            for (int i = 0; i < static_cast<int>(saves_.size()); i++) {
                float by = startY + i * gap;
                if (by > winY + winH - Theme::s(4.0f)) break;
                SDL_Rect btnRect = {
                    static_cast<int>(bx), static_cast<int>(by),
                    static_cast<int>(bw), static_cast<int>(bh)
                };
                SDL_Point pt = {mx, my};
                if (SDL_PointInRect(&pt, &btnRect)) {
                    Audio::instance().playSound("clickedSound");
                    auto* gameScreen = dynamic_cast<GameScreen*>(app.screen(ScreenType::GAME));
                    if (gameScreen) {
                        bool ok = gameScreen->loadGame(app, saves_[i].name);
                        if (ok) {
                            nextScreen = ScreenType::GAME;
                        }
                    } else {
                        toasts().show("Game screen is unavailable");
                    }
                    return;
                }


                float delBtnW = Theme::s(2.5f);
                float delBtnH = bh * 0.7f;
                float delBtnX = bx + bw - delBtnW - Theme::s(0.3f);
                float delBtnY = by + (bh - delBtnH) * 0.5f;
                SDL_Rect delRect = {
                    static_cast<int>(delBtnX), static_cast<int>(delBtnY),
                    static_cast<int>(delBtnW), static_cast<int>(delBtnH)
                };
                if (SDL_PointInRect(&pt, &delRect)) {
                    Audio::instance().playSound("clickedSound");
                    toasts().show("Delete not implemented yet");
                }
            }
        }


        if (mode == Mode::SAVE) {
            float inputY = winY + Theme::s(3.5f);
            float inputH = Theme::s(2.0f);
            float saveY = inputY + inputH + Theme::s(1.0f);
            SDL_Rect inputRect = {
                static_cast<int>(bx), static_cast<int>(inputY),
                static_cast<int>(bw), static_cast<int>(inputH)
            };
            SDL_Rect saveRect = {
                static_cast<int>(bx), static_cast<int>(saveY),
                static_cast<int>(bw), static_cast<int>(bh)
            };
            SDL_Point pt = {mx, my};
            if (SDL_PointInRect(&pt, &inputRect)) {
                Audio::instance().playSound("clickedSound");
                enteringName_ = true;
                SDL_StartTextInput();
                printf("[SaveLoadScreen] text input activated\n");
                fflush(stdout);
            }
            if (SDL_PointInRect(&pt, &saveRect) && !newSaveName_.empty()) {
                Audio::instance().playSound("clickedSound");
                saveCurrentGame();
                return;
            }
        }


        float backY = winY + winH - Theme::s(3.0f);
        SDL_Rect backRect = {
            static_cast<int>(bx), static_cast<int>(backY),
            static_cast<int>(bw), static_cast<int>(bh)
        };
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &backRect)) {
            Audio::instance().playSound("clickedSound");
            if (enteringName_) SDL_StopTextInput();
            nextScreen = returnScreen;
        }
    }


    if (input.isKeyDown(SDL_SCANCODE_ESCAPE)) {
        if (enteringName_) SDL_StopTextInput();
        nextScreen = returnScreen;
    }
}


void SaveLoadScreen::update(App& app, float dt) {

}


void SaveLoadScreen::render(App& app) {
    auto& eng = Engine::instance();
    int W = eng.WIDTH, H = eng.HEIGHT;
    float u = H / 100.0f;
    auto* r = eng.renderer;
    auto& assets = UIAssets::instance();
    int mx, my; SDL_GetMouseState(&mx, &my);

    eng.clear(Theme::bg);
    UIPrim::drawVignette(r, W, H, 80);


    float winW = std::round(W * 0.45f);
    winW = std::clamp(winW, 550.0f, 1200.0f);
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

    std::string title = (mode == Mode::SAVE) ? "SAVE GAME" : "LOAD GAME";
    SDL_Texture* headerTex = assets.panelHeader();
    if (headerTex) UIAssets::draw9Slice(r, headerTex, winX + 3, winY + 3, winW - 6, headerH, 14);
    int titleFs = std::max(16, (int)(headerH * 0.42f));
    UIPrim::drawText(r, title, titleFs, winX + winW / 2, winY + headerH * 0.5f, "center", Theme::gold_bright, true);
    UIPrim::drawHLine(r, Theme::gold, winX + 8, winX + winW - 8, winY + headerH + 2, 2);

    UIPrim::drawOrnamentalFrame(r, winX, winY, winW, winH, 2);

    float bw = Theme::s(14.0f);
    float bh = Theme::s(2.2f);
    float bx = W * 0.5f - bw * 0.5f;

    auto& slAssets = UIAssets::instance();

    if (mode == Mode::SAVE) {

        float inputY = winY + Theme::s(3.5f);
        float inputH = Theme::s(2.0f);

        UIPrim::drawRectFilled(r, Theme::slot, bx, inputY, bw, inputH);
        UIPrim::drawRect(r, Theme::border, bx, inputY, bw, inputH, Theme::border, 0, 1);

        std::string displayText = newSaveName_.empty() ? "Enter save name..." : newSaveName_ + "_";
        Color textColor = newSaveName_.empty() ? Theme::dark_grey : Theme::cream;
        UIPrim::drawText(r, displayText, Theme::si(1.0f),
                         bx + Theme::s(0.5f), inputY + inputH * 0.5f,
                         "midleft", textColor);


        float saveY = inputY + inputH + Theme::s(1.0f);
        bool canSave = !newSaveName_.empty();
        Color saveBtnColor = canSave ? Theme::btn_confirm : Theme::btn_disabled;
        UIPrim::drawRectFilled(r, saveBtnColor, bx, saveY, bw, bh);
        UIPrim::drawText(r, "Save", Theme::si(1.0f),
                         bx + bw * 0.5f, saveY + bh * 0.5f,
                         "center", canSave ? Theme::cream : Theme::dark_grey);


        float listY = saveY + bh + Theme::s(2.0f);
        UIPrim::drawSectionHeader(r, bx, listY, bw, Theme::s(1.5f), "Existing Saves");
        listY += Theme::s(2.0f);

        float gap = Theme::s(2.2f);
        for (int i = 0; i < static_cast<int>(saves_.size()); i++) {
            float by = listY + i * gap;
            if (by > winY + winH - Theme::s(4.0f)) break;

            UIPrim::drawRectFilled(r, Theme::panel_alt, bx, by, bw, bh * 0.8f);


            SDL_Texture* sFileIcon = slAssets.icon("File-Alt");
            float sfSz = bh * 0.4f;
            if (sFileIcon) {
                SDL_SetTextureColorMod(sFileIcon, 210, 200, 172);
                SDL_Rect sfDst = {(int)(bx + Theme::s(0.4f)), (int)(by + bh * 0.4f - sfSz * 0.5f), (int)sfSz, (int)sfSz};
                SDL_RenderCopy(r, sFileIcon, nullptr, &sfDst);
                SDL_SetTextureColorMod(sFileIcon, 255, 255, 255);
            }

            float sTextIndent = Theme::s(0.4f) + sfSz + Theme::s(0.3f);
            UIPrim::drawText(r, saves_[i].name, Theme::si(0.9f),
                             bx + sTextIndent, by + bh * 0.4f,
                             "midleft", Theme::cream);
            UIPrim::drawText(r, saves_[i].date, Theme::si(0.7f),
                             bx + bw - Theme::s(0.5f), by + bh * 0.4f,
                             "midright", Theme::grey);
        }
    } else {

        float startY = winY + Theme::s(3.5f);
        float gap = Theme::s(2.8f);

        if (saves_.empty()) {
            UIPrim::drawText(r, "No saves found", Theme::si(1.2f),
                             W * 0.5f, winY + winH * 0.4f, "center", Theme::grey);
        }

        for (int i = 0; i < static_cast<int>(saves_.size()); i++) {
            float by = startY + i * gap;
            if (by > winY + winH - Theme::s(4.0f)) break;

            bool hovered = (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh);
            Color bgColor = hovered ? Theme::btn_hover : Theme::btn;
            UIPrim::drawRectFilled(r, bgColor, bx, by, bw, bh);
            UIPrim::drawRect(r, Theme::border, bx, by, bw, bh, Theme::border, 0, 1);


            SDL_Texture* fileIcon = slAssets.icon("File-Alt");
            float fiSz = bh * 0.45f;
            if (fileIcon) {
                SDL_SetTextureColorMod(fileIcon, 210, 200, 172);
                SDL_Rect fiDst = {(int)(bx + Theme::s(0.4f)), (int)(by + (bh - fiSz) * 0.5f), (int)fiSz, (int)fiSz};
                SDL_RenderCopy(r, fileIcon, nullptr, &fiDst);
                SDL_SetTextureColorMod(fileIcon, 255, 255, 255);
            }

            float textIndent = Theme::s(0.4f) + fiSz + Theme::s(0.3f);
            UIPrim::drawText(r, saves_[i].name, Theme::si(1.0f),
                             bx + textIndent, by + bh * 0.35f,
                             "midleft", Theme::cream);
            UIPrim::drawText(r, saves_[i].date, Theme::si(0.8f),
                             bx + bw - Theme::s(3.2f), by + bh * 0.65f,
                             "midright", Theme::grey);


            float delBtnW = Theme::s(2.5f);
            float delBtnH = bh * 0.7f;
            float delBtnX = bx + bw - delBtnW - Theme::s(0.3f);
            float delBtnY = by + (bh - delBtnH) * 0.5f;
            bool delHov = (mx >= delBtnX && mx <= delBtnX + delBtnW &&
                           my >= delBtnY && my <= delBtnY + delBtnH);
            Color delBg = delHov ? Theme::btn_hover : Theme::slot;
            UIPrim::drawRoundedRect(r, delBg, delBtnX, delBtnY, delBtnW, delBtnH, 3, Theme::border);
            UIPrim::drawText(r, "Del", Theme::si(0.7f),
                             delBtnX + delBtnW * 0.5f, delBtnY + delBtnH * 0.5f,
                             "center", Theme::grey);
        }
    }


    float backY = winY + winH - Theme::s(3.0f);
    UIPrim::drawMenuButton(r, bx, backY, bw, bh,
                           "Back", mx, my, false);


    SDL_Texture* backIcon = slAssets.icon("SolidArrow-Left");
    if (backIcon) {
        float biSz = bh * 0.5f;
        SDL_SetTextureColorMod(backIcon, 210, 200, 172);
        SDL_Rect biDst = {(int)(bx + Theme::s(0.5f)), (int)(backY + (bh - biSz) * 0.5f), (int)biSz, (int)biSz};
        SDL_RenderCopy(r, backIcon, nullptr, &biDst);
        SDL_SetTextureColorMod(backIcon, 255, 255, 255);
    }
}
