#include "sidebar/sidebar.h"
#include "core/app.h"
#include "core/engine.h"
#include "core/input.h"
#include "ui/primitives.h"
#include "ui/theme.h"
#include "ui/ui_assets.h"
#include "game/game_state.h"
#include "game/country.h"
#include "game/faction.h"
#include "game/political_options.h"
#include "game/effect_system.h"
#include "game/puppet.h"
#include "ui/toast.h"
#include "game/division.h"
#include "game/helpers.h"
#include "data/country_data.h"
#include "data/region_data.h"
#include "game/buildings.h"
#include "game/economy.h"
#include "core/audio.h"

Sidebar::Sidebar() { sideBarSize_ = 0.27f; }

void Sidebar::toggle() { targetAnimation_ = (targetAnimation_ > 0.5f) ? 0.0f : 1.0f; }
void Sidebar::open()   { targetAnimation_ = 1.0f; }
void Sidebar::close()  { targetAnimation_ = 0.0f; }

float Sidebar::getWidth(int screenW) const { return screenW * sideBarSize_; }
float Sidebar::getX(int screenW) const { return screenW - getWidth(screenW) * animation; }

void Sidebar::handleInput(App& app, const InputState& input) {}

void Sidebar::update(App& app, float dt) {
    float speed = 8.0f;
    if (animation < targetAnimation_)
        animation = std::min(animation + speed * dt, targetAnimation_);
    else if (animation > targetAnimation_)
        animation = std::max(animation - speed * dt, targetAnimation_);
}

void Sidebar::render(SDL_Renderer* r, App& app) {
    if (animation < 0.01f) return;

    auto& eng = Engine::instance();
    auto& gs = app.gameState();
    int W = eng.WIDTH, H = eng.HEIGHT;
    float ui = eng.uiScale;
    float u = H / 100.0f;

    float topBarH = std::max(50.0f, H * 0.06f);
    float sideW = W * sideBarSize_;
    float sideX = W - sideW * animation;
    float sideY = topBarH;
    float sideH = H - sideY;


    auto& assets = UIAssets::instance();
    UIPrim::drawRectFilled(r, {16, 18, 24}, sideX, sideY, sideW, sideH);


    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, Theme::gold_dim.r, Theme::gold_dim.g, Theme::gold_dim.b, 120);
    SDL_RenderDrawLine(r, (int)sideX, (int)sideY, (int)sideX, (int)(sideY + sideH));
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 80);
    SDL_RenderDrawLine(r, (int)sideX + 1, (int)sideY, (int)sideX + 1, (int)(sideY + sideH));


    float tabW = sideW / 3.0f;
    float tabH = std::max(36.0f, H * 0.045f);
    int mx, my; SDL_GetMouseState(&mx, &my);

    const char* tabs[] = {"Political", "Military", "Industry"};
    const char* tabKeys[] = {"political", "military", "industry"};
    const char* tabIcons[] = {"Star", "Player", "Cart"};
    for (int i = 0; i < 3; i++) {
        float tx = sideX + i * tabW;
        bool active = (activeTab == tabKeys[i]);
        bool hovered = (mx >= tx && mx <= tx + tabW && my >= sideY && my <= sideY + tabH);

        if (active) {
            UIPrim::drawRectFilled(r, {24, 26, 34}, tx, sideY, tabW, tabH);

            UIPrim::drawHLine(r, Theme::gold, tx + 4, tx + tabW - 4, sideY + tabH - 2, 2);
        } else {
            Color tabColor = hovered ? Color{30, 33, 40} : Color{20, 22, 28};
            UIPrim::drawRectFilled(r, tabColor, tx, sideY, tabW, tabH);
        }
        if (i > 0) UIPrim::drawVLine(r, Theme::border, tx, sideY + 6, sideY + tabH - 6, 1);


        float iconSz = tabH * 0.4f;
        SDL_Texture* tabIcon = assets.icon(tabIcons[i]);
        if (tabIcon) {
            if (active) SDL_SetTextureColorMod(tabIcon, Theme::gold_bright.r, Theme::gold_bright.g, Theme::gold_bright.b);
            else if (hovered) SDL_SetTextureColorMod(tabIcon, 210, 200, 172);
            else SDL_SetTextureColorMod(tabIcon, 170, 170, 172);
            SDL_Rect iDst = {(int)(tx + tabW * 0.5f - iconSz - 4), (int)(sideY + (tabH - iconSz) / 2), (int)iconSz, (int)iconSz};
            SDL_RenderCopy(r, tabIcon, nullptr, &iDst);
            SDL_SetTextureColorMod(tabIcon, 255, 255, 255);
        }

        int tabFs = std::max(13, static_cast<int>(tabH * 0.35f));
        float textX = tabIcon ? tx + tabW * 0.5f + 4 : tx + tabW * 0.5f;
        Color tabTextColor = active ? Theme::gold_bright : (hovered ? Theme::cream : Theme::grey);
        UIPrim::drawText(r, tabs[i], tabFs, textX, sideY + tabH * 0.5f,
                         tabIcon ? "midleft" : "center", tabTextColor, active);
    }

    UIPrim::drawHLine(r, Theme::border, sideX, sideX + sideW, sideY + tabH, 1);


    float pad = H * 0.012f;
    float contentY = sideY + tabH + pad;
    float w = sideW - pad * 2;
    float x = sideX + pad;

    Country* cc = gs.getCountry(gs.controlledCountry);
    if (!cc) return;

    float y = contentY;
    int fs = std::max(14, H / 60);
    int fsLarge = std::max(16, H / 50);
    float row_h = std::max(26.0f, H * 0.028f);
    float btn_h = std::max(34.0f, H * 0.042f);


    bool dtHovered = (mx >= x && mx <= x + w && my >= y && my <= y + btn_h);
    bool dtClicked = dtHovered && app.input().mouseLeftDown;
    UIPrim::drawActionRow(r, x, y, w, btn_h, "Decision Tree", mx, my, dtClicked);
    if (dtClicked) {
        Audio::instance().playSound("clickedSound");
        app.switchScreen(ScreenType::DECISION_TREE);
    }
    y += btn_h + H * 0.01f;


    if (!selectedCountry.empty() && selectedCountry != gs.controlledCountry) {
        y = renderForeignTab(r, app, x, y, w);
    } else {
        if (activeTab == "political") y = renderPoliticalTab(r, app, x, y, w);
        else if (activeTab == "military") y = renderMilitaryTab(r, app, x, y, w);
        else if (activeTab == "industry") y = renderIndustryTab(r, app, x, y, w);
    }
}

float Sidebar::renderPoliticalTab(SDL_Renderer* r, App& app, float x, float y, float w) {
    auto& gs = app.gameState();
    Country* cc = gs.getCountry(gs.controlledCountry);
    if (!cc) return y;

    int H = Engine::instance().HEIGHT;
    float u = H / 100.0f;
    int fs = std::max(14, (int)(u * 1.2f));
    int fsLabel = std::max(12, (int)(u * 1.0f));
    int fsSmall = std::max(11, (int)(u * 0.9f));
    float row_h = std::round(u * 2.6f);
    float pad = std::round(u * 0.8f);
    float secH = std::round(u * 3.2f);


    float identH = row_h * 4.5f + pad * 2;
    UIPrim::drawRoundedRect(r, {10, 12, 16}, x, y, w, identH, 4);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 30);
    SDL_Rect ib = {(int)x, (int)y, (int)w, (int)identH};
    SDL_RenderDrawRect(r, &ib);

    float iy = y + pad;
    float ix = x + pad;
    float iw = w - pad * 2;


    std::string faction = cc->faction.empty() ? "None" : replaceAll(cc->faction, "_", " ");
    UIPrim::drawInfoRow(r, ix, iy, iw, row_h, "Faction", faction); iy += row_h;


    std::string ideo = Helpers::getIdeologyName(cc->ideology[0], cc->ideology[1]);
    if (!ideo.empty()) ideo[0] = std::toupper(ideo[0]);
    UIPrim::drawInfoRow(r, ix, iy, iw, row_h, "Ideology", ideo); iy += row_h;


    std::string focusStr = "None";
    if (cc->focus.has_value()) {
        auto& [fn, fd, fe] = *cc->focus;
        focusStr = fn + " (" + std::to_string(fd) + "d)";
    }
    UIPrim::drawInfoRow(r, ix, iy, iw, row_h, "Focus", focusStr); iy += row_h;


    UIPrim::drawInfoRow(r, ix, iy, iw, row_h, "Leader", cc->leader.name); iy += row_h;


    for (auto& minister : cc->cabinet.members) {
        if (minister.name.empty() || minister.name == "Vacant") continue;
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: %s (%.0f%%)", minister.role.c_str(), minister.name.c_str(), minister.skill * 100);
        UIPrim::drawText(r, buf, fsSmall, ix, iy + row_h * 0.3f, "midleft", Theme::grey);
        iy += std::round(u * 1.5f);
    }


    if (!cc->puppetTo.empty()) {
        UIPrim::drawText(r, "Puppet of: " + replaceAll(cc->puppetTo, "_", " "), fsSmall,
                         ix, iy + row_h * 0.3f, "midleft", Theme::red_light);
        iy += row_h;
    }


    int puppetCount = 0;
    for (auto& ps : gs.puppetStates) {
        if (ps.active && ps.overlord == gs.controlledCountry) puppetCount++;
    }
    if (puppetCount > 0) {
        UIPrim::drawText(r, "Puppets: " + std::to_string(puppetCount), fs, ix, iy + row_h * 0.3f, "midleft", Theme::green_light);
        iy += row_h;
    }


    y = iy + pad;


    float csz = std::min(w * 0.75f, std::round(u * 14.0f));
    float cx = x + (w - csz) / 2;


    UIPrim::drawRectFilled(r, {6, 8, 10}, cx - 3, y - 3, csz + 6, csz + 6);
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 60);
    SDL_Rect compFrame = {(int)(cx - 3), (int)(y - 3), (int)(csz + 6), (int)(csz + 6)};
    SDL_RenderDrawRect(r, &compFrame);

    float half = csz / 2;

    UIPrim::drawRectFilled(r, {180, 85, 85}, cx, y, half, half);
    UIPrim::drawRectFilled(r, {60, 120, 190}, cx + half, y, half, half);
    UIPrim::drawRectFilled(r, {80, 165, 80}, cx, y + half, half, half);
    UIPrim::drawRectFilled(r, {140, 110, 180}, cx + half, y + half, half, half);


    int ctrX = (int)(cx + csz / 2), ctrY2 = (int)(y + csz / 2), ctrR = (int)(csz / 5);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int dy = -ctrR; dy <= ctrR; dy++) {
        int dx = (int)std::sqrt(ctrR * ctrR - dy * dy);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 140);
        SDL_RenderDrawLine(r, ctrX - dx, ctrY2 + dy, ctrX + dx, ctrY2 + dy);
    }


    SDL_SetRenderDrawColor(r, 0, 0, 0, 60);
    SDL_RenderDrawLine(r, (int)cx, ctrY2, (int)(cx + csz), ctrY2);
    SDL_RenderDrawLine(r, ctrX, (int)y, ctrX, (int)(y + csz));


    float eco = cc->ideology[0], soc = cc->ideology[1];
    int dotX = (int)(cx + csz / 2 + eco * csz / 2);
    int dotY = (int)(y + csz / 2 - soc * csz / 2);
    int dotR = std::max(3, (int)(csz / 22));

    for (int dy = -dotR-1; dy <= dotR+1; dy++) {
        int dx = (int)std::sqrt((dotR+1) * (dotR+1) - dy * dy);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
        SDL_RenderDrawLine(r, dotX - dx + 1, dotY + dy + 1, dotX + dx + 1, dotY + dy + 1);
    }

    for (int dy = -dotR; dy <= dotR; dy++) {
        int dx = (int)std::sqrt(dotR * dotR - dy * dy);
        SDL_SetRenderDrawColor(r, Theme::gold_bright.r, Theme::gold_bright.g, Theme::gold_bright.b, 255);
        SDL_RenderDrawLine(r, dotX - dx, dotY + dy, dotX + dx, dotY + dy);
    }
    y += csz + std::round(u * 1.2f);


    y = UIPrim::drawSectionHeader(r, x, y, w, secH, "Statistics");
    y += std::round(u * 0.4f);


    float statsH = row_h * 5 + pad;
    UIPrim::drawRoundedRect(r, {10, 12, 16}, x, y, w, statsH, 4);
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 25);
    SDL_Rect sb = {(int)x, (int)y, (int)w, (int)statsH};
    SDL_RenderDrawRect(r, &sb);

    float sy = y + pad * 0.5f;
    float sw = w - pad;
    UIPrim::drawInfoRow(r, x + pad * 0.5f, sy, sw, row_h, "Population", Helpers::prefixNumber(cc->population)); sy += row_h;
    UIPrim::drawInfoRow(r, x + pad * 0.5f, sy, sw, row_h, "Stability", std::to_string((int)cc->stability) + "%",
                        {0,0,0,0}, cc->stability >= 50 ? Theme::green_light : Theme::red_light); sy += row_h;
    UIPrim::drawInfoRow(r, x + pad * 0.5f, sy, sw, row_h, "Political Power", std::to_string((int)cc->politicalPower)); sy += row_h;
    UIPrim::drawInfoRow(r, x + pad * 0.5f, sy, sw, row_h, "Regions", std::to_string(cc->regions.size())); sy += row_h;
    UIPrim::drawInfoRow(r, x + pad * 0.5f, sy, sw, row_h, "At War With", std::to_string(cc->atWarWith.size()),
                        {0,0,0,0}, cc->atWarWith.empty() ? Theme::grey : Theme::red_light); sy += row_h;
    y += statsH + std::round(u * 0.6f);


    if (!cc->atWarWith.empty()) {
        for (auto& enemy : cc->atWarWith) {
            std::string enemyName = replaceAll(enemy, "_", " ");
            UIPrim::drawText(r, "  " + enemyName, fsSmall, x + pad, y, "midleft", Theme::red_light);
            y += std::round(u * 1.8f);
        }
    }


    {
        y += std::round(u * 0.4f);
        y = UIPrim::drawSectionHeader(r, x, y, w, secH, "Combat Stats");
        y += std::round(u * 0.4f);

        float csRow = std::round(u * 1.8f);
        float atkVal = std::round(cc->combatStats.attack * cc->attackMultiplier * 10.0f) / 10.0f;
        float defVal = std::round(cc->combatStats.defense * cc->defenseMultiplier * 10.0f) / 10.0f;

        struct StatLine { const char* name; float val; };
        StatLine stats[] = {
            {"Attack", atkVal},
            {"Defense", defVal},
            {"Armor", cc->combatStats.armor},
            {"Piercing", cc->combatStats.piercing},
            {"Speed", cc->combatStats.speed},
        };
        for (auto& st : stats) {
            char sbuf[64];
            snprintf(sbuf, sizeof(sbuf), "  %s: %.1f", st.name, st.val);
            UIPrim::drawText(r, sbuf, fsSmall, x + pad, y, "midleft", Theme::cream);
            y += csRow;
        }
        y += std::round(u * 0.4f);
    }


    int mx, my; SDL_GetMouseState(&mx, &my);
    auto options = getOptions(gs.controlledCountry, gs);
    if (!options.empty()) {
        y += std::round(u * 0.5f);
        y = UIPrim::drawSectionHeader(r, x, y, w, secH, "Political Actions");
        y += std::round(u * 0.5f);

        float actH = std::round(u * 3.5f);
        float catH = std::round(u * 3.0f);
        bool mouseDown = app.input().mouseLeftDown;


        std::map<std::string, std::vector<PoliticalOption*>> grouped;
        for (auto& opt : options) {
            grouped[opt.category].push_back(&opt);
        }

        static std::string openCategory;

        for (auto& [category, catOpts] : grouped) {

            std::string catTitle = category.empty() ? "General" : category;
            catTitle[0] = std::toupper(catTitle[0]);
            bool catHov = mx >= x && mx <= x + w && my >= y && my <= y + catH;

            Color catBg = catHov ? Color{30, 33, 40} : Color{22, 24, 30};
            UIPrim::drawRoundedRect(r, catBg, x, y, w, catH, 3, Theme::border);
            int catFs = std::max(11, (int)(catH * 0.38f));
            UIPrim::drawText(r, catTitle, catFs, x + std::round(u * 1.0f), y + catH / 2, "midleft",
                             catHov ? Theme::gold : Theme::cream);

            UIPrim::drawText(r, openCategory == category ? "v" : ">", catFs,
                             x + w - std::round(u * 1.5f), y + catH / 2, "center", Theme::grey);

            if (catHov && mouseDown) {
                openCategory = (openCategory == category) ? "" : category;
            }
            y += catH + std::round(u * 0.2f);


            if (openCategory == category) {
                for (auto* opt : catOpts) {
                    bool canAfford = cc->politicalPower >= opt->ppCost;
                    std::string costStr = std::to_string(opt->ppCost) + " PP";
                    bool hov = mx >= x + std::round(u * 0.8f) && mx <= x + w && my >= y && my <= y + actH;

                    UIPrim::drawActionRow(r, x + std::round(u * 0.8f), y, w - std::round(u * 1.6f), actH,
                                          opt->name, mx, my, hov && mouseDown,
                                          costStr, opt->description, canAfford);

                    if (hov && mouseDown && canAfford) {
                        cc->politicalPower -= opt->ppCost;
                        EffectSystem efx;
                        auto effects = efx.parse(opt->effects, gs.controlledCountry);
                        efx.executeBatch(effects, gs);
                        Audio::instance().playSound("clickedSound");
                        toasts().show("Enacted: " + opt->name, 2000);
                    }

                    y += actH + std::round(u * 0.2f);
                }
            }
        }
    }

    return y;
}

float Sidebar::renderMilitaryTab(SDL_Renderer* r, App& app, float x, float y, float w) {
    auto& gs = app.gameState();
    Country* cc = gs.getCountry(gs.controlledCountry);
    if (!cc) return y;
    int H = Engine::instance().HEIGHT;
    float u = H / 100.0f;

    int fs = std::max(14, (int)(u * 1.2f));
    float row_h = std::round(u * 2.6f);
    float pad = std::round(u * 0.8f);
    float btn_h = std::round(u * 4.5f);
    float secH = std::round(u * 3.2f);
    int mx, my; SDL_GetMouseState(&mx, &my);


    float ovH = row_h * 4.2f + pad * 2;
    UIPrim::drawRoundedRect(r, {10, 12, 16}, x, y, w, ovH, 4);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 30);
    SDL_Rect ob = {(int)x, (int)y, (int)w, (int)ovH};
    SDL_RenderDrawRect(r, &ob);

    float iy = y + pad;
    float iw = w - pad * 2;
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Size", Helpers::getMilitarySizeName(cc->militarySize)); iy += row_h;
    std::string deploy = cc->deployRegion.empty() ? "None" : cc->deployRegion;
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Deployment", deploy); iy += row_h;

    float totalStacks = 0;
    for (auto& d : cc->divisions) totalStacks += d->divisionStack;
    float dailyUp = totalStacks * gs.DIVISION_UPKEEP_PER_DAY;
    int armsC = cc->buildingManager.countAll(BuildingType::MilitaryFactory);
    int tDays = std::max(3, (int)(14 * std::max(0.25f, 1.0f - armsC * 0.15f)));

    char buf[128];
    snprintf(buf, sizeof(buf), "$%s/day", Helpers::prefixNumber(dailyUp).c_str());
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Upkeep", buf, {0,0,0,0}, Theme::orange); iy += row_h;
    snprintf(buf, sizeof(buf), "$%s | %dd", Helpers::prefixNumber(gs.TRAINING_COST_PER_DIV).c_str(), tDays);
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Train Cost", buf); iy += row_h;

    y += ovH + std::round(u * 0.5f);


    {
        float cdH = std::round(u * 3.5f);
        bool cdHov = mx >= x && mx <= x + w && my >= y && my <= y + cdH;
        bool mouseDown2 = app.input().mouseLeftDown;
        UIPrim::drawActionRow(r, x, y, w, cdH, "Change Deployment", mx, my, cdHov && mouseDown2, "", cc->deployRegion.empty() ? "None" : cc->deployRegion);
        if (cdHov && mouseDown2) {
            cc->changeDeployment(gs);
            Audio::instance().playSound("clickedSound");
        }
        y += cdH + std::round(u * 0.5f);
    }


    float tall = std::max(40.0f, H * 0.05f);
    bool canTrain1 = (cc->money >= gs.TRAINING_COST_PER_DIV && cc->manPower >= 10000);
    bool mouseDown = app.input().mouseLeftDown;

    snprintf(buf, sizeof(buf), "Train 1 ($%s)", Helpers::prefixNumber(gs.TRAINING_COST_PER_DIV).c_str());
    bool t1Hovered = (mx >= x && mx <= x + w && my >= y && my <= y + tall);
    bool t1Clicked = t1Hovered && mouseDown;
    UIPrim::drawActionRow(r, x, y, w, tall, buf, mx, my, t1Clicked, "",
                           "10k manpower | " + std::to_string(tDays) + "d", canTrain1);
    if (t1Clicked) {
        if (canTrain1) {
            cc->trainDivision(gs, 1);
            Audio::instance().playSound("clickedSound");
        } else {
            Audio::instance().playSound("failedClickSound");
        }
    }
    y += tall + H * 0.005f;

    int allDiv = (int)(cc->manPower / 10000);
    bool canTrainAll = (allDiv > 0 && cc->money >= gs.TRAINING_COST_PER_DIV * allDiv);
    snprintf(buf, sizeof(buf), "Train All (%d)", allDiv);
    bool tAllHovered = (mx >= x && mx <= x + w && my >= y && my <= y + tall);
    bool tAllClicked = tAllHovered && mouseDown;
    UIPrim::drawActionRow(r, x, y, w, tall, buf, mx, my, tAllClicked, "",
                           Helpers::prefixNumber(allDiv * 10000.0) + " manpower", canTrainAll);
    if (tAllClicked) {
        if (canTrainAll) {
            cc->trainDivision(gs, allDiv);
            Audio::instance().playSound("clickedSound");
        } else {
            Audio::instance().playSound("failedClickSound");
        }
    }
    y += tall + H * 0.01f;


    y = UIPrim::drawSectionHeader(r, x - pad, y, w + pad * 2, secH, "Training");
    y += H * 0.005f;
    if (cc->training.empty()) {
        UIPrim::drawText(r, "No units training", fs, x, y + H * 0.005f, "midleft", Theme::dark_grey);
        y += row_h;
    } else {
        for (int i = 0; i < std::min((int)cc->training.size(), 6); i++) {
            bool ready = (cc->training[i][1] == 0);
            std::string label = ready
                ? "Deploy " + std::to_string(cc->training[i][0]) + " Div"
                : "Training " + std::to_string(cc->training[i][0]) + " Div (" + std::to_string(cc->training[i][1]) + "d)";
            bool trHov = ready && mx >= x && mx <= x + w && my >= y && my <= y + btn_h;
            bool trClick = trHov && mouseDown;
            UIPrim::drawActionRow(r, x, y, w, btn_h, label, mx, my, trClick, "", "", ready);
            if (trClick) {

                int divCount = cc->training[i][0];
                int deployReg = -1;
                if (!cc->deployRegion.empty()) {
                    deployReg = RegionData::instance().getCityRegion(cc->deployRegion);
                }
                cc->addDivision(gs, divCount, deployReg, true);
                cc->training.erase(cc->training.begin() + i);
                Audio::instance().playSound("clickedSound");
                break;
            }
            y += btn_h + H * 0.004f;
        }
        if ((int)cc->training.size() > 6) {
            UIPrim::drawText(r, "+" + std::to_string(cc->training.size() - 6) + " more...", fs,
                             x + w / 2, y, "topcenter", Theme::grey);
            y += row_h;
        }
    }

    y += H * 0.008f;
    y = UIPrim::drawSectionHeader(r, x - pad, y, w + pad * 2, secH, "Divisions");
    y += H * 0.005f;
    snprintf(buf, sizeof(buf), "%d divisions (%s troops)", (int)cc->divisions.size(),
             Helpers::prefixNumber(cc->totalMilitary).c_str());
    UIPrim::drawText(r, buf, fs, x, y + H * 0.005f, "midleft", Theme::cream);
    y += row_h;

    return y;
}

float Sidebar::renderIndustryTab(SDL_Renderer* r, App& app, float x, float y, float w) {
    auto& gs = app.gameState();
    Country* cc = gs.getCountry(gs.controlledCountry);
    if (!cc) return y;
    int H = Engine::instance().HEIGHT;
    float u = H / 100.0f;

    int fs = std::max(14, (int)(u * 1.2f));
    int fsSm = std::max(12, (int)(u * 0.9f));
    float row_h = std::round(u * 2.6f);
    float pad = std::round(u * 0.8f);
    float secH = std::round(u * 3.2f);
    float constructSpeed = cc->buildingManager.getConstructionSpeed(cc) * cc->resourceManager.getProductionPenalty();
    int mx, my; SDL_GetMouseState(&mx, &my);


    int civC = cc->buildingManager.countAll(BuildingType::CivilianFactory);
    float dailyIncome = 5000.0f * (civC + 1) * cc->moneyMultiplier;
    float totalStacks = 0;
    for (auto& d : cc->divisions) totalStacks += d->divisionStack;
    float dailyUpkeep = totalStacks * gs.DIVISION_UPKEEP_PER_DAY;
    float net = dailyIncome - dailyUpkeep;
    Color nc = net >= 0 ? Theme::green_light : Theme::red_light;

    float econH = row_h * 3 + pad * 2;
    UIPrim::drawRoundedRect(r, {10, 12, 16}, x, y, w, econH, 4);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 30);
    SDL_Rect eb = {(int)x, (int)y, (int)w, (int)econH};
    SDL_RenderDrawRect(r, &eb);

    float iy = y + pad;
    float iw = w - pad * 2;
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Income", Helpers::prefixNumber(dailyIncome) + "/d", {0,0,0,0}, Theme::green_light); iy += row_h;
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Upkeep", "-" + Helpers::prefixNumber(dailyUpkeep) + "/d", {0,0,0,0}, Theme::orange); iy += row_h;
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Net", Helpers::prefixNumber(net) + "/d", {0,0,0,0}, nc); iy += row_h;

    y += econH + std::round(u * 0.8f);


    y = UIPrim::drawSectionHeader(r, x - pad, y, w + pad * 2, secH, "Resources");
    y += H * 0.005f;
    const char* resNames[] = {"Oil", "Steel", "Aluminum", "Tungsten", "Chromium", "Rubber"};
    auto& rm = cc->resourceManager;
    for (int ri = 0; ri < RESOURCE_COUNT && ri < 6; ri++) {
        float prod = rm.production[ri], cons = rm.consumption[ri];
        float imp = rm.tradeImports[ri], exp = rm.tradeExports[ri];
        float stk = rm.stockpile[ri], netR = prod + imp - cons - exp;
        Color rc = netR >= 0 ? Theme::green_light : Theme::red_light;
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: %.0f", resNames[ri], stk);
        UIPrim::drawText(r, buf, fs, x, y, "midleft", rc); y += H * 0.022f;
        if (imp > 0.0f || exp > 0.0f)
            snprintf(buf, sizeof(buf), "  +%.1f +%.1fi -%.1f -%.1fe = %+.1f/d", prod, imp, cons, exp, netR);
        else
            snprintf(buf, sizeof(buf), "  +%.1f -%.1f = %+.1f/d", prod, cons, netR);
        UIPrim::drawText(r, buf, fsSm, x, y, "midleft", Theme::grey); y += H * 0.022f;
    }
    y += H * 0.008f;


    {
        float prodPen = rm.getProductionPenalty();
        float combatPen = rm.getCombatPenalty();
        if (prodPen < 1.0f) {
            char penBuf[64];
            snprintf(penBuf, sizeof(penBuf), "Production penalty: %.0f%%", prodPen * 100);
            UIPrim::drawText(r, penBuf, fsSm, x, y, "midleft", Theme::orange);
            y += H * 0.022f;
        }
        if (combatPen < 1.0f) {
            char penBuf[64];
            snprintf(penBuf, sizeof(penBuf), "Combat penalty: %.0f%%", combatPen * 100);
            UIPrim::drawText(r, penBuf, fsSm, x, y, "midleft", Theme::red_light);
            y += H * 0.022f;
        }
    }
    y += H * 0.005f;


    y = UIPrim::drawSectionHeader(r, x, y, w, secH, "Construction");
    y += std::round(u * 0.5f);
    {
        struct CBtn { const char* label; const char* key; };
        CBtn cbtns[] = {
            {"Factory", "civilian_factory"}, {"Arms", "arms_factory"}, {"Dock", "dockyard"},
            {"Mine", "mine"}, {"Oil", "oil_well"}, {"Refine", "refinery"},
            {"Infra", "infrastructure"}, {"Port", "port"}, {"Destroy", "destroy"}
        };
        int numBtns = 9;
        int cols = 3;
        float cbH = std::round(u * 2.8f);
        float cbGap = std::round(u * 0.3f);
        float cbW = (w - (cols - 1) * cbGap) / cols;
        bool mouseDown = app.input().mouseLeftDown;

        for (int ci = 0; ci < numBtns; ci++) {
            int row = ci / cols;
            int col = ci % cols;
            float cbX = x + col * (cbW + cbGap);
            float cbY = y + row * (cbH + cbGap);
            bool isActive = (currentlyBuilding == cbtns[ci].key);
            bool cbHov = mx >= cbX && mx <= cbX + cbW && my >= cbY && my <= cbY + cbH;
            Color cbBg = isActive ? Color{35, 50, 40} : (cbHov ? Color{30, 33, 42} : Color{18, 20, 26});
            Color cbBrd = isActive ? Theme::gold : Theme::border;
            UIPrim::drawRoundedRect(r, cbBg, cbX, cbY, cbW, cbH, 4, cbBrd);
            if (isActive) UIPrim::drawHLine(r, Theme::gold, cbX + 4, cbX + cbW - 4, cbY + 1, 2);
            int cbFs = std::max(10, (int)(cbH * 0.34f));
            Color cbTc = isActive ? Theme::gold_bright : (cbHov ? Theme::cream : Theme::grey);
            UIPrim::drawText(r, cbtns[ci].label, cbFs, cbX + cbW / 2, cbY + cbH / 2, "center", cbTc, isActive);
            if (cbHov && mouseDown) currentlyBuilding = cbtns[ci].key;
        }
        int totalRows = (numBtns + cols - 1) / cols;
        y += totalRows * (cbH + cbGap) + std::round(u * 0.3f);


        int tipFs = std::max(10, (int)(u * 0.8f));
        BuildingType selBt = buildingTypeFromString(currentlyBuilding);
        float baseCost = (float)buildingBaseCost(selBt);
        float dynamicCost = baseCost * (1.0f + cc->regions.size() / 150.0f);
        int bDays = std::max(1, static_cast<int>(std::ceil(buildingDays(selBt) / std::max(0.01f, constructSpeed))));
        const char* bDesc = buildingDescription(selBt);

        char costBuf[128];
        if (currentlyBuilding == "destroy") {
            snprintf(costBuf, sizeof(costBuf), "Selected: Destroy");
        } else {
            snprintf(costBuf, sizeof(costBuf), "Selected: %s ($%s | %dd)", buildingTypeName(selBt),
                     Helpers::prefixNumber(dynamicCost).c_str(), bDays);
        }
        UIPrim::drawText(r, costBuf, tipFs, x + pad, y, "midleft", Theme::cream);
        y += std::round(u * 1.5f);
        if (bDesc[0] != '\0') {
            UIPrim::drawText(r, bDesc, tipFs, x + pad, y, "midleft", Theme::grey);
            y += std::round(u * 1.5f);
        }
        UIPrim::drawText(r, "Right-click on owned region to build", tipFs, x + w / 2, y, "topcenter", Theme::dark_grey);
        y += std::round(u * 1.8f);
    }


    y = UIPrim::drawSectionHeader(r, x, y, w, secH, "Buildings");
    y += H * 0.005f;
    const BuildingType bts[] = {BuildingType::CivilianFactory, BuildingType::MilitaryFactory,
        BuildingType::Dockyard, BuildingType::Mine, BuildingType::OilWell,
        BuildingType::Refinery, BuildingType::Infrastructure, BuildingType::Port, BuildingType::Fortress};
    for (int bi = 0; bi < 9; bi++) {
        int count = cc->buildingManager.countAll(bts[bi]);
        if (count > 0) {
            UIPrim::drawText(r, std::string(buildingTypeName(bts[bi])) + ": " + std::to_string(count), fs, x, y, "midleft", Theme::cream);
            y += H * 0.025f;
        }
    }
    y += H * 0.008f;


    y = UIPrim::drawSectionHeader(r, x - pad, y, w + pad * 2, secH, "Construction Queue");
    y += H * 0.005f;
    auto& cQueue = cc->buildingManager.constructionQueue;
    int queueSize = static_cast<int>(cQueue.size());
    if (queueSize == 0) {
        UIPrim::drawText(r, "Queue empty", fs, x, y + H * 0.005f, "midleft", Theme::dark_grey);
        y += row_h;
    } else {
        int showMax = std::min(queueSize, 8);
        for (int qi = 0; qi < showMax; qi++) {
            auto& entry = cQueue[qi];
            std::string itemName = buildingTypeName(entry.type);
            char qbuf[128];
            int estDays = std::max(0, static_cast<int>(std::ceil(entry.daysRemaining / std::max(0.01f, constructSpeed))));
            snprintf(qbuf, sizeof(qbuf), "%s (%dd left)", itemName.c_str(), estDays);
            UIPrim::drawText(r, qbuf, fs, x, y + H * 0.005f, "midleft", Theme::cream);
            y += row_h;
        }
        if (queueSize > showMax) {
            UIPrim::drawText(r, "+" + std::to_string(queueSize - showMax) + " more...", fsSm,
                             x, y + H * 0.005f, "midleft", Theme::dark_grey);
            y += row_h;
        }
    }

    return y;
}

float Sidebar::renderSelfInfo(SDL_Renderer* r, App& app, float x, float y, float w) {
    return renderPoliticalTab(r, app, x, y, w);
}

float Sidebar::renderCountryInfo(SDL_Renderer* r, App& app, float x, float y, float w) {
    return y;
}

float Sidebar::renderForeignTab(SDL_Renderer* r, App& app, float x, float y, float w) {
    auto& gs = app.gameState();
    Country* fc = gs.getCountry(selectedCountry);
    if (!fc) return y;

    int H = Engine::instance().HEIGHT;
    float u = H / 100.0f;
    int fs = std::max(14, (int)(u * 1.2f));
    int fsLabel = std::max(12, (int)(u * 1.0f));
    float row_h = std::round(u * 2.6f);
    float pad = std::round(u * 0.8f);
    float secH = std::round(u * 3.2f);
    int mx, my; SDL_GetMouseState(&mx, &my);


    std::string displayName = replaceAll(selectedCountry, "_", " ");
    int nameFs = std::max(16, (int)(u * 1.6f));
    UIPrim::drawText(r, displayName, nameFs, x + w / 2, y, "topcenter", Theme::gold_bright, true);
    y += std::round(u * 2.5f);


    std::string lowerName = selectedCountry;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    SDL_Texture* flagTex = Engine::instance().loadTexture(Engine::instance().assetsPath + "flags/" + lowerName + "_flag.png");
    if (flagTex) {
        int fw, fh; SDL_QueryTexture(flagTex, nullptr, nullptr, &fw, &fh);
        float dispW = std::min(w * 0.60f, std::round(u * 14.0f));
        float dispH = dispW * fh / fw;
        SDL_Rect fDst = {(int)(x + (w - dispW) / 2), (int)y, (int)dispW, (int)dispH};
        UIPrim::drawRectFilled(r, {6, 8, 10}, fDst.x - 2, fDst.y - 2, fDst.w + 4, fDst.h + 4);
        SDL_RenderCopy(r, flagTex, nullptr, &fDst);
        SDL_SetRenderDrawColor(r, Theme::gold_dim.r, Theme::gold_dim.g, Theme::gold_dim.b, 100);
        SDL_RenderDrawRect(r, &fDst);
        y += dispH + std::round(u * 1.5f);
    }


    float infoH = row_h * 5 + pad * 2;
    UIPrim::drawRoundedRect(r, {10, 12, 16}, x, y, w, infoH, 4);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, Theme::border.r, Theme::border.g, Theme::border.b, 30);
    SDL_Rect ib = {(int)x, (int)y, (int)w, (int)infoH};
    SDL_RenderDrawRect(r, &ib);


    float infoRows = 5;
    float iy = y + pad;
    float iw = w - pad * 2;
    std::string faction = fc->faction.empty() ? "None" : replaceAll(fc->faction, "_", " ");
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Faction", faction); iy += row_h;


    Country* player = gs.getCountry(gs.controlledCountry);
    if (player) {
        bool hasAccess = player->hasLandAccessTo(selectedCountry, gs);
        Color accessColor = hasAccess ? Theme::green_light : Theme::red_light;
        UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Military Access", hasAccess ? "Yes" : "No", {0,0,0,0}, accessColor); iy += row_h;
    }

    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Ideology", fc->ideologyName); iy += row_h;
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Regions", std::to_string(fc->regions.size())); iy += row_h;
    UIPrim::drawInfoRow(r, x + pad, iy, iw, row_h, "Divisions", std::to_string(fc->divisions.size())); iy += row_h;
    y += infoH + std::round(u * 1.0f);


    y = UIPrim::drawSectionHeader(r, x, y, w, secH, "Diplomacy");
    y += std::round(u * 0.5f);

    if (player) {
        float btnH = std::round(u * 4.0f);
        bool mouseDown = app.input().mouseLeftDown;


        bool atWar = std::find(player->atWarWith.begin(), player->atWarWith.end(), selectedCountry) != player->atWarWith.end();
        if (!atWar) {
            bool hov = mx >= x && mx <= x + w && my >= y && my <= y + btnH;
            UIPrim::drawActionRow(r, x, y, w, btnH, "Declare War", mx, my, hov && mouseDown, "25 PP", "", player->politicalPower >= 25);
            if (hov && mouseDown && player->politicalPower >= 25) {
                player->politicalPower -= 25;
                player->declareWar(selectedCountry, gs);
                Audio::instance().playSound("clickedSound");
            }
            y += btnH + std::round(u * 0.4f);
        }


        bool hasAccess = player->hasLandAccessTo(selectedCountry, gs);
        if (!atWar && !hasAccess) {
            bool hov = mx >= x && mx <= x + w && my >= y && my <= y + btnH;
            UIPrim::drawActionRow(r, x, y, w, btnH, "Get Military Access", mx, my, hov && mouseDown, "50 PP", "", player->politicalPower >= 50);
            if (hov && mouseDown && player->politicalPower >= 50) {
                player->politicalPower -= 50;
                player->militaryAccess.push_back(selectedCountry);
                Audio::instance().playSound("clickedSound");
            }
            y += btnH + std::round(u * 0.4f);
        }


        if (!player->faction.empty() && player->factionLeader && fc->faction.empty() && !atWar) {
            bool hov = mx >= x && mx <= x + w && my >= y && my <= y + btnH;
            UIPrim::drawActionRow(r, x, y, w, btnH, "Invite to Faction", mx, my, hov && mouseDown, "25 PP", "", player->politicalPower >= 25);
            if (hov && mouseDown && player->politicalPower >= 25) {
                player->politicalPower -= 25;

                auto* fac = gs.getFaction(player->faction);
                if (fac) fac->addCountry(selectedCountry, gs);
                Audio::instance().playSound("clickedSound");
            }
            y += btnH + std::round(u * 0.4f);
        }
    }

    return y;
}
