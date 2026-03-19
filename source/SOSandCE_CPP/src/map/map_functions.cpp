#include "map/map_functions.h"

#include <stack>


static const Color WATER_COLOR = {126, 142, 158};
static const Color BLACK_COLOR = {0, 0, 0};
static const Color BORDER_GREY = {105, 118, 132};

static bool isWater(Color c) {
    return c.r == WATER_COLOR.r && c.g == WATER_COLOR.g && c.b == WATER_COLOR.b;
}

static bool isBlack(Color c) {
    return c.r == 0 && c.g == 0 && c.b == 0;
}


void MapFunc::fill(SDL_Surface* surface, float x, float y, Color newColor) {
    if (!surface) return;

    int sx = static_cast<int>(x);
    int sy = static_cast<int>(y);
    if (sx < 0 || sy < 0 || sx >= surface->w || sy >= surface->h) return;

    SDL_LockSurface(surface);

    Color oldColor = getPixel(surface, sx, sy);
    if (oldColor == newColor) {
        SDL_UnlockSurface(surface);
        return;
    }

    std::stack<std::pair<int, int>> stack;
    stack.push({sx, sy});

    while (!stack.empty()) {
        auto [cx, cy] = stack.top();
        stack.pop();

        if (cx < 0 || cy < 0 || cx >= surface->w || cy >= surface->h) continue;

        Color current = getPixel(surface, cx, cy);
        if (current != oldColor) continue;

        setPixel(surface, cx, cy, newColor);

        stack.push({cx + 1, cy});
        stack.push({cx - 1, cy});
        stack.push({cx, cy + 1});
        stack.push({cx, cy - 1});
    }

    SDL_UnlockSurface(surface);
}

void MapFunc::fillRegionMask(SDL_Surface* surface, SDL_Surface* maskSurface,
                             float x, float y, Color newColor) {
    if (!surface || !maskSurface) return;

    int sx = static_cast<int>(x);
    int sy = static_cast<int>(y);
    if (sx < 0 || sy < 0 || sx >= surface->w || sy >= surface->h) return;
    if (sx >= maskSurface->w || sy >= maskSurface->h) return;

    SDL_LockSurface(surface);
    SDL_LockSurface(maskSurface);

    Color maskColor = getPixel(maskSurface, sx, sy);
    if (isBlack(maskColor) || isWater(maskColor) || maskColor == BORDER_GREY) {
        SDL_UnlockSurface(maskSurface);
        SDL_UnlockSurface(surface);
        return;
    }

    std::stack<std::pair<int, int>> stack;
    stack.push({sx, sy});

    while (!stack.empty()) {
        auto [cx, cy] = stack.top();
        stack.pop();

        if (cx < 0 || cy < 0 || cx >= surface->w || cy >= surface->h) continue;
        if (cx >= maskSurface->w || cy >= maskSurface->h) continue;

        Color currentMask = getPixel(maskSurface, cx, cy);
        if (currentMask != maskColor) continue;

        Color currentColor = getPixel(surface, cx, cy);
        if (currentColor == newColor) continue;

        setPixel(surface, cx, cy, newColor);

        stack.push({cx + 1, cy});
        stack.push({cx - 1, cy});
        stack.push({cx, cy + 1});
        stack.push({cx, cy - 1});
    }

    SDL_UnlockSurface(maskSurface);
    SDL_UnlockSurface(surface);
}


void MapFunc::fillWithBorder(SDL_Surface* surface, SDL_Surface* referenceSurface,
                             float x, float y, Color newColor) {
    if (!surface || !referenceSurface) return;

    int sx = static_cast<int>(x);
    int sy = static_cast<int>(y);
    if (sx < 0 || sy < 0 || sx >= surface->w || sy >= surface->h) return;

    SDL_LockSurface(surface);
    SDL_LockSurface(referenceSurface);

    Color oldColor = getPixel(surface, sx, sy);
    if (oldColor == newColor) {
        SDL_UnlockSurface(referenceSurface);
        SDL_UnlockSurface(surface);
        return;
    }


    Color borderColor;
    borderColor.r = static_cast<uint8_t>(std::max(0.0f, newColor.r / 1.4f));
    borderColor.g = static_cast<uint8_t>(std::max(0.0f, newColor.g / 1.4f));
    borderColor.b = static_cast<uint8_t>(std::max(0.0f, newColor.b / 1.4f));
    borderColor.a = 255;

    std::stack<std::pair<int, int>> stack;
    stack.push({sx, sy});

    while (!stack.empty()) {
        auto [cx, cy] = stack.top();
        stack.pop();

        if (cx < 0 || cy < 0 || cx >= surface->w || cy >= surface->h) continue;

        Color current = getPixel(surface, cx, cy);
        if (current != oldColor) continue;


        Color refColor = getPixel(referenceSurface, cx, cy);

        if (isWater(refColor)) {

            continue;
        } else if (isBlack(refColor)) {

            setPixel(surface, cx, cy, BLACK_COLOR);
            continue;
        } else if (refColor == BORDER_GREY) {

            setPixel(surface, cx, cy, borderColor);
            continue;
        }


        setPixel(surface, cx, cy, newColor);

        stack.push({cx + 1, cy});
        stack.push({cx - 1, cy});
        stack.push({cx, cy + 1});
        stack.push({cx, cy - 1});
    }

    SDL_UnlockSurface(referenceSurface);
    SDL_UnlockSurface(surface);
}


void MapFunc::fillFixBorder(SDL_Surface* surface, float x, float y, Color newColor) {
    if (!surface) return;

    int sx = static_cast<int>(x);
    int sy = static_cast<int>(y);
    if (sx < 0 || sy < 0 || sx >= surface->w || sy >= surface->h) return;

    SDL_LockSurface(surface);

    Color oldColor = getPixel(surface, sx, sy);
    if (oldColor == newColor) {
        SDL_UnlockSurface(surface);
        return;
    }


    Color borderColor;
    borderColor.r = static_cast<uint8_t>(newColor.r / 1.4f);
    borderColor.g = static_cast<uint8_t>(newColor.g / 1.4f);
    borderColor.b = static_cast<uint8_t>(newColor.b / 1.4f);
    borderColor.a = 255;


    std::vector<std::pair<int, int>> borderPixels;
    std::stack<std::pair<int, int>> stack;
    stack.push({sx, sy});

    while (!stack.empty()) {
        auto [cx, cy] = stack.top();
        stack.pop();

        if (cx < 0 || cy < 0 || cx >= surface->w || cy >= surface->h) continue;

        Color current = getPixel(surface, cx, cy);
        if (current != oldColor) continue;

        setPixel(surface, cx, cy, newColor);


        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx2 = cx + dx, ny2 = cy + dy;
                if (nx2 < 0 || ny2 < 0 || nx2 >= surface->w || ny2 >= surface->h) continue;
                Color nc = getPixel(surface, nx2, ny2);
                if (isBlack(nc)) {
                    borderPixels.push_back({nx2, ny2});
                }
            }
        }

        stack.push({cx + 1, cy});
        stack.push({cx - 1, cy});
        stack.push({cx, cy + 1});
        stack.push({cx, cy - 1});
    }


    SDL_Surface* copy = SDL_ConvertSurface(surface, surface->format, 0);
    if (copy) {
        SDL_LockSurface(copy);
        for (auto& [bx, by] : borderPixels) {

            std::vector<Color> neighborColors;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx2 = bx + dx, ny2 = by + dy;
                    if (nx2 < 0 || ny2 < 0 || nx2 >= surface->w || ny2 >= surface->h) continue;
                    Color sc = getPixel(copy, nx2, ny2);
                    if (!isBlack(sc) && !isWater(sc) && sc != BORDER_GREY) {
                        neighborColors.push_back(sc);
                    }
                }
            }

            if (neighborColors.size() == 1) {

                setPixel(surface, bx, by, borderColor);
            } else if (neighborColors.size() > 1) {

                bool allSame = true;
                for (size_t i = 1; i < neighborColors.size(); i++) {
                    if (neighborColors[i] != neighborColors[0]) { allSame = false; break; }
                }
                if (allSame) {
                    setPixel(surface, bx, by, borderColor);
                } else {
                    setPixel(surface, bx, by, BLACK_COLOR);
                }
            }
        }
        SDL_UnlockSurface(copy);
        SDL_FreeSurface(copy);
    }

    SDL_UnlockSurface(surface);
}


SDL_Surface* MapFunc::fixBorders(SDL_Surface* mapSurface,
                                  const std::vector<Color>& toChange,
                                  const std::vector<Color>& toIgnore) {
    if (!mapSurface) return nullptr;


    SDL_Surface* result = SDL_ConvertSurface(mapSurface, mapSurface->format, 0);
    if (!result) return nullptr;

    SDL_LockSurface(result);
    SDL_LockSurface(mapSurface);

    int w = mapSurface->w;
    int h = mapSurface->h;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Color current = getPixel(mapSurface, x, y);


            bool shouldChange = false;
            for (const auto& tc : toChange) {
                if (current.r == tc.r && current.g == tc.g && current.b == tc.b) {
                    shouldChange = true;
                    break;
                }
            }
            if (!shouldChange) continue;


            std::map<uint32_t, std::pair<Color, int>> colorCounts;
            Color bestColor = current;
            int bestCount = 0;

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;

                    Color nc = getPixel(mapSurface, nx, ny);


                    bool skip = false;
                    for (const auto& tc : toChange) {
                        if (nc.r == tc.r && nc.g == tc.g && nc.b == tc.b) {
                            skip = true;
                            break;
                        }
                    }
                    if (skip) continue;


                    for (const auto& ti : toIgnore) {
                        if (nc.r == ti.r && nc.g == ti.g && nc.b == ti.b) {
                            skip = true;
                            break;
                        }
                    }
                    if (skip) continue;

                    uint32_t key = nc.toUint32();
                    colorCounts[key].first = nc;
                    colorCounts[key].second++;
                    if (colorCounts[key].second > bestCount) {
                        bestCount = colorCounts[key].second;
                        bestColor = nc;
                    }
                }
            }

            if (bestCount > 0) {
                setPixel(result, x, y, bestColor);
            }
        }
    }

    SDL_UnlockSurface(mapSurface);
    SDL_UnlockSurface(result);

    return result;
}
