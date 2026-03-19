#include "game/ai.h"
#include "game/game_state.h"
#include "game/country.h"
#include "game/division.h"
#include "game/faction.h"
#include "game/buildings.h"
#include "game/helpers.h"
#include "data/region_data.h"


AIController::AIController(const std::string& name, GameState& gs)
    : countryName(name)
{

    Country* c = gs.getCountry(name);
    if (c) {
        std::string ideology = getIdeologyName(c->ideology[0], c->ideology[1]);
        if (ideology == "communist") {
            personality = {0.6f, 0.5f, 0.3f, 0.7f, 0.6f, 0.8f};
        } else if (ideology == "nationalist") {
            personality = {0.8f, 0.4f, 0.2f, 0.4f, 0.8f, 0.7f};
        } else if (ideology == "liberal") {
            personality = {0.3f, 0.5f, 0.7f, 0.7f, 0.3f, 0.5f};
        } else if (ideology == "monarchist") {
            personality = {0.5f, 0.6f, 0.5f, 0.5f, 0.5f, 0.6f};
        } else {
            personality = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        }
    }
}

AIController::~AIController() = default;

Country* AIController::getCountry(GameState& gs) const {
    return gs.getCountry(countryName);
}


void AIController::update(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;

    float now = getCurrent();
    if (now - lastActed < actionInterval) return;
    lastActed = now;

    atWar = !country->atWarWith.empty();
    enemies = country->atWarWith;


    evaluateThreats(gs);
    evaluateEconomy(gs);
    evaluatePolitics(gs);


    if (atWar) {
        manageDivisions(gs);
        assignFrontline(gs);
    } else {
        evaluateWar(gs);
        evaluateAlliances(gs);
        manageDivisionTraining(gs);
    }


    manageBuildQueue(gs);
    manageTrade(gs);
    manageFocus(gs);
}


void AIController::evaluateThreats(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;

    enemies.clear();
    potentialAllies.clear();

    for (auto& neighbor : country->bordering) {
        if (neighbor == countryName) continue;

        Country* n = gs.getCountry(neighbor);
        if (!n) continue;

        float threat = threatLevel(neighbor, gs);

        if (threat > 50.0f) {
            enemies.push_back(neighbor);
        }


        std::string myIdeology = getIdeologyName(country->ideology[0], country->ideology[1]);
        std::string theirIdeology = getIdeologyName(n->ideology[0], n->ideology[1]);
        if (myIdeology == theirIdeology && threat < 30.0f) {
            potentialAllies.push_back(neighbor);
        }
    }
}

void AIController::evaluateAlliances(GameState& gs) {
    considerJoinFaction(gs);
    considerCreateFaction(gs);
}

void AIController::evaluateWar(GameState& gs) {
    considerDeclareWar(gs);
}

void AIController::evaluateEconomy(GameState& ) {

}

void AIController::evaluatePolitics(GameState& ) {

}


void AIController::manageDivisions(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;

    for (auto& div : country->divisions) {
        if (!div) continue;
        if (div->fighting || div->locked) continue;

        microDivision(div.get(), gs);
    }
}

void AIController::assignFrontline(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;

    if (country->atWarWith.empty()) return;
    if (country->battleBorder.empty()) return;


    std::vector<Division*> available;
    for (auto& div : country->divisions) {
        if (!div) continue;
        if (div->fighting || div->locked || !div->commands.empty()) continue;
        available.push_back(div.get());
    }

    if (available.empty()) return;

    auto& border = country->battleBorder;
    int divsPerRegion = std::max(1, static_cast<int>(available.size()) / std::max(1, static_cast<int>(border.size())));

    int assigned = 0;
    for (int region : border) {
        for (int d = 0; d < divsPerRegion; ++d) {
            if (assigned >= static_cast<int>(available.size())) break;

            Division* div = available[assigned];
            if (div->region != region) {
                div->command(region, gs, true, true);
            }
            assigned++;
        }
    }
}

void AIController::orderAttack(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;

    for (auto& div : country->divisions) {
        if (!div || div->fighting || div->locked) continue;

        int target = findBestTarget(div.get(), gs);
        if (target >= 0) {
            div->command(target, gs, false, true, 200);
        }
    }
}

void AIController::orderDefend(GameState& gs) {

    assignFrontline(gs);
}

void AIController::orderRetreat(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;

    for (auto& div : country->divisions) {
        if (!div || !div->fighting) continue;

        if (shouldRetreat(div.get(), gs)) {
            int safe = findSafeRegion(div.get(), gs);
            if (safe >= 0) {
                div->command(safe, gs, true, true, 100);
            }
        }
    }
}

void AIController::manageDivisionTraining(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;


    for (int i = static_cast<int>(country->training.size()) - 1; i >= 0; --i) {
        if (country->training[i][1] == 0) {
            auto& rd = RegionData::instance();
            int deployReg = -1;
            if (!country->deployRegion.empty()) {
                deployReg = rd.getCityRegion(country->deployRegion);
            }
            country->addDivision(gs, country->training[i][0], deployReg, true);
            country->training.erase(country->training.begin() + i);
        }
    }


    if (country->manPower >= 10000.0f &&
        country->money > gs.TRAINING_COST_PER_DIV &&
        country->training.empty()) {
        country->trainDivision(gs, 1);
    }
}

void AIController::microDivision(Division* div, GameState& gs) {
    if (!div) return;

    Country* country = getCountry(gs);
    if (!country) return;


    if (div->recovering) return;


    if (shouldRetreat(div, gs)) {
        int safe = findSafeRegion(div, gs);
        if (safe >= 0 && safe != div->region) {
            div->command(safe, gs, true, true, 100);
        }
        return;
    }


    if (atWar) {
        int target = findBestTarget(div, gs);
        if (target >= 0) {
            div->command(target, gs, false, true, 200);
        }
    }
}

int AIController::findBestTarget(Division* div, GameState& gs) const {
    if (!div) return -1;

    Country* country = getCountry(gs);
    if (!country) return -1;


    int bestTarget = -1;
    float bestScore = -1.0f;

    for (int rid : country->battleBorder) {

        auto it = gs.divisionsByRegion.find(rid);
        int enemyCount = 0;
        if (it != gs.divisionsByRegion.end()) {
            for (auto* d : it->second) {
                if (d && d->country != countryName) enemyCount++;
            }
        }


        float score = 10.0f - static_cast<float>(enemyCount) * 2.0f;

        if (score > bestScore) {
            bestScore = score;
            bestTarget = rid;
        }
    }

    return bestTarget;
}

int AIController::findSafeRegion(Division* div, GameState& gs) const {
    if (!div) return -1;

    Country* country = getCountry(gs);
    if (!country) return -1;


    if (country->regions.empty()) return -1;


    std::vector<int> safe;
    for (int rid : country->regions) {
        bool onBorder = false;
        for (int bb : country->battleBorder) {
            if (bb == rid) { onBorder = true; break; }
        }
        if (!onBorder) safe.push_back(rid);
    }

    if (safe.empty()) {

        return div->region;
    }

    return safe[randInt(0, static_cast<int>(safe.size()) - 1)];
}

bool AIController::shouldRetreat(Division* div, GameState& ) const {
    if (!div) return false;

    if (div->maxUnits > 0 && div->units / div->maxUnits < 0.3f) return true;

    if (div->maxResources > 0 && div->resources / div->maxResources < 0.2f) return true;
    return false;
}


void AIController::considerJoinFaction(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country || !country->faction.empty()) return;


    for (auto& fName : gs.factionList) {
        Faction* f = gs.getFaction(fName);
        if (!f) continue;

        Country* leader = gs.getCountry(f->factionLeader);
        if (!leader) continue;

        float desirability = allianceDesirability(f->factionLeader, gs);
        if (desirability > 0.6f && randFloat(0.0f, 1.0f) < 0.1f) {
            f->addCountry(countryName, gs, false, false);
            return;
        }
    }
}

void AIController::considerCreateFaction(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country || !country->faction.empty() || !country->canMakeFaction) return;

    if (personality.diplomacy > 0.5f && randFloat(0.0f, 1.0f) < 0.05f) {
        std::string factionName = countryName + " Pact";
        auto faction = std::make_unique<Faction>(factionName, std::vector<std::string>{countryName}, gs);
        gs.registerFaction(factionName, std::move(faction));
        country->faction = factionName;
        country->factionLeader = true;
    }
}

void AIController::considerDeclareWar(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country || !country->atWarWith.empty()) return;

    if (!canAffordWar(gs)) return;

    float warWillingness = personality.aggressiveness;
    if (randFloat(0.0f, 1.0f) > warWillingness * 0.01f) return;


    float myStrength = 0.0f;
    for (auto& div : country->divisions) {
        if (div) myStrength += div->divisionStack;
    }

    for (auto& neighbor : country->bordering) {
        Country* n = gs.getCountry(neighbor);
        if (!n) continue;
        if (n->faction == country->faction && !country->faction.empty()) continue;

        float theirStrength = 0.0f;
        for (auto& div : n->divisions) {
            if (div) theirStrength += div->divisionStack;
        }

        if (myStrength > theirStrength * 1.5f) {

            bool hasClaims = false;
            for (int core : country->coreRegions) {
                for (int rid : n->regions) {
                    if (core == rid) { hasClaims = true; break; }
                }
                if (hasClaims) break;
            }

            if (hasClaims || warWillingness > 0.6f) {
                wantsWar = true;
                targetCountry = neighbor;
                country->declareWar(neighbor, gs, false, false);
                return;
            }
        }
    }
}

void AIController::considerMakePeace(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country || country->atWarWith.empty()) return;


    float myStrength = 0.0f;
    for (auto& div : country->divisions) {
        if (div) myStrength += div->divisionStack;
    }

    if (myStrength < 1.0f && country->regions.size() < 3) {

        for (auto& enemy : country->atWarWith) {
            country->makePeace(enemy, gs, false);
            return;
        }
    }
}

void AIController::considerAlliance(GameState& ) {

}


void AIController::manageBuildQueue(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;

    auto& bm = country->buildingManager;


    int maxQueue = std::max(1, bm.countAll(BuildingType::CivilianFactory));
    if (static_cast<int>(bm.constructionQueue.size()) >= maxQueue) return;


    std::vector<BuildingType> buildOrder;
    if (personality.economicFocus > 0.6f) {
        buildOrder = {BuildingType::CivilianFactory, BuildingType::Infrastructure,
                      BuildingType::MilitaryFactory};
    } else if (personality.aggressiveness > 0.6f) {
        buildOrder = {BuildingType::MilitaryFactory, BuildingType::Infrastructure,
                      BuildingType::CivilianFactory};
    } else {
        buildOrder = {BuildingType::CivilianFactory, BuildingType::MilitaryFactory,
                      BuildingType::Infrastructure};
    }


    auto getDays = [](BuildingType t) -> int {
        switch (t) {
            case BuildingType::CivilianFactory:  return 120;
            case BuildingType::MilitaryFactory:  return 120;
            case BuildingType::Infrastructure:   return 90;
            case BuildingType::Port:             return 60;
            case BuildingType::Fortress:         return 150;
            default: return 120;
        }
    };

    for (auto type : buildOrder) {
        for (int rid : country->regions) {
            if (bm.canBuild(rid, type)) {

                std::string typeStr = buildingTypeToString(type);
                if (bm.startConstruction(rid, typeStr, country)) return;
            }
        }
    }
}

void AIController::manageTrade(GameState& ) {


}

void AIController::manageFocus(GameState& gs) {
    Country* country = getCountry(gs);
    if (!country) return;
    if (country->focus.has_value()) return;

    auto& fte = country->focusTreeEngine;


    std::vector<std::string> available;
    for (auto& n : fte.nodes) {
        if (fte.canStartFocus(n.name, country)) {
            available.push_back(n.name);
        }
    }
    if (available.empty()) return;


    std::string chosen = available[randInt(0, static_cast<int>(available.size()) - 1)];
    auto* node = fte.getNode(chosen);
    if (!node) return;


    country->politicalPower -= static_cast<float>(node->cost);
    fte.completedFocuses.insert(chosen);

    std::vector<std::string> effectStrs;
    for (auto& eff : node->effects) {
        if (auto* s = std::get_if<std::string>(&eff)) effectStrs.push_back(*s);
    }
    country->focus = std::make_tuple(chosen, node->days, effectStrs);
}


float AIController::threatLevel(const std::string& otherCountry, GameState& gs) const {
    Country* country = getCountry(gs);
    Country* other = gs.getCountry(otherCountry);
    if (!country || !other) return 0.0f;

    float threat = 0.0f;


    for (auto& e : other->atWarWith) {
        if (e == countryName) { threat += 100.0f; break; }
    }


    float myDivs = static_cast<float>(country->divisions.size());
    float theirDivs = static_cast<float>(other->divisions.size());
    if (myDivs > 0) {
        threat += std::max(0.0f, (theirDivs - myDivs) / myDivs * 30.0f);
    }


    std::string myIdeology = getIdeologyName(country->ideology[0], country->ideology[1]);
    std::string theirIdeology = getIdeologyName(other->ideology[0], other->ideology[1]);
    if (myIdeology != theirIdeology) {
        threat += 10.0f;
    }

    return threat;
}

float AIController::allianceDesirability(const std::string& otherCountry, GameState& gs) const {
    Country* country = getCountry(gs);
    Country* other = gs.getCountry(otherCountry);
    if (!country || !other) return 0.0f;

    float score = 0.0f;


    std::string myIdeology = getIdeologyName(country->ideology[0], country->ideology[1]);
    std::string theirIdeology = getIdeologyName(other->ideology[0], other->ideology[1]);
    if (myIdeology == theirIdeology) score += 0.4f;


    for (auto& e : country->atWarWith) {
        for (auto& e2 : other->atWarWith) {
            if (e == e2) { score += 0.3f; break; }
        }
    }


    bool bordering = false;
    for (auto& b : country->bordering) {
        if (b == otherCountry) { bordering = true; break; }
    }
    if (!bordering) score += 0.1f;

    return std::min(1.0f, score);
}

bool AIController::canAffordWar(GameState& gs) const {
    Country* country = getCountry(gs);
    if (!country) return false;


    return country->divisions.size() >= 3 && country->money > 500000.0f;
}
