#include "ui/button.h"
#include "core/input.h"
#include "ui/primitives.h"
#include "ui/theme.h"
#include "ui/ui_assets.h"


Button::Button(float x, float y, float w, float h, const std::string& label,
               std::function<void()> onClick)
    : label(label), onClick(std::move(onClick)) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}


void Button::handleInput(const InputState& input) {
    if (!visible || !enabled) {
        hovered = false;
        pressed = false;
        return;
    }

    hovered = containsPoint(static_cast<float>(input.mouseX),
                             static_cast<float>(input.mouseY));

    if (hovered && input.mouseLeftDown) {
        pressed = true;
        if (onClick) {
            onClick();
        }
    } else {
        pressed = false;
    }
}


void Button::render(SDL_Renderer* r) {
    if (!visible) return;

    auto& assets = UIAssets::instance();


    SDL_Texture* btnTex = (hovered && enabled) ? assets.btnRectHover() : assets.btnRectDefault();
    if (!btnTex) btnTex = assets.btnRectDefault();

    float drawY = (pressed && hovered) ? y + 1 : y;

    if (btnTex && enabled) {

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 25);
        SDL_Rect sh = {(int)x + 1, (int)y + 2, (int)w, (int)h};
        SDL_RenderFillRect(r, &sh);


        if (pressed && hovered) {
            SDL_SetTextureColorMod(btnTex, 150, 150, 155);
        } else if (hovered) {
            SDL_SetTextureColorMod(btnTex, 220, 218, 215);
        } else {
            SDL_SetTextureColorMod(btnTex, 185, 183, 180);
        }
        UIAssets::draw9Slice(r, btnTex, x, drawY, w, h, 14);
        SDL_SetTextureColorMod(btnTex, 255, 255, 255);


        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, Theme::gold.r, Theme::gold.g, Theme::gold.b,
                               hovered ? (uint8_t)200 : (uint8_t)70);
        SDL_Rect leftBar = {(int)x + 3, (int)drawY + 4, 3, (int)h - 8};
        SDL_RenderFillRect(r, &leftBar);


        if (hovered) {
            UIPrim::drawHLine(r, Theme::gold_dim, x + 8, x + w - 8, drawY + 2);
        }
    } else {

        Color bgColor = !enabled ? Theme::btn_disabled
                      : (pressed ? Theme::btn_press
                      : (hovered ? Theme::btn_hover : Theme::btn));
        UIPrim::drawRoundedRect(r, bgColor, x, drawY, w, h, 5,
                                hovered ? Theme::gold_dim : Theme::border);
    }


    int textSize = fontSize;
    if (textSize <= 0) {
        textSize = Theme::si(0.9f);
        if (textSize < 10) textSize = 10;
    }
    Color textColor = !enabled ? Theme::dark_grey
                    : (hovered ? Theme::gold : Theme::cream);

    if (!label.empty()) {
        if (value.empty()) {
            UIPrim::drawText(r, label, textSize,
                             x + w * 0.5f, drawY + h * 0.5f,
                             "center", textColor, true);
        } else {
            UIPrim::drawText(r, label, textSize,
                             x + Theme::s(0.5f), drawY + h * 0.5f,
                             "midleft", textColor);
            UIPrim::drawText(r, value, textSize,
                             x + w - Theme::s(0.5f), drawY + h * 0.5f,
                             "midright", Theme::gold);
        }
    }
}
