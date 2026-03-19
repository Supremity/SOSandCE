#include "game/economy.h"
#include "game/game_state.h"
#include "game/country.h"
#include "game/buildings.h"


ResourceManager::ResourceManager() {
    production.fill(0.0f);
    consumption.fill(0.0f);
    stockpile.fill(0.0f);
    tradeImports.fill(0.0f);
    tradeExports.fill(0.0f);
    deficit.fill(0.0f);
}

float ResourceManager::getNet(Resource r) const {
    int i = static_cast<int>(r);
    return production[i] + tradeImports[i] - consumption[i] - tradeExports[i];
}

float ResourceManager::getAvailable(Resource r) const {
    int i = static_cast<int>(r);
    return stockpile[i];
}

bool ResourceManager::hasDeficit(Resource r) const {
    int i = static_cast<int>(r);
    return deficit[i] > 0.0f;
}

void ResourceManager::resetFlows() {
    production.fill(0.0f);
    consumption.fill(0.0f);
}

void ResourceManager::addProduction(Resource r, float amount) {
    production[static_cast<int>(r)] += amount;
}

void ResourceManager::addConsumption(Resource r, float amount) {
    consumption[static_cast<int>(r)] += amount;
}

void ResourceManager::addRegionResources(int regionId, const std::unordered_map<Resource, float>& res) {
    regionProduction[regionId] = res;
}

void ResourceManager::removeRegionResources(int regionId) {
    regionProduction.erase(regionId);
}

void ResourceManager::recalculate() {
    for (int i = 0; i < RESOURCE_COUNT; ++i) {
        production[i] = 1.0f;
    }
    for (auto& [regionId, resMap] : regionProduction) {
        for (auto& [res, amount] : resMap) {
            production[static_cast<int>(res)] += amount;
        }
    }
}


void ResourceManager::update(GameState& gs, const std::string& countryName) {
    Country* country = gs.getCountry(countryName);
    if (!country) return;


    constexpr float kLegacyContinuousRatePerHour = 1.0f / 6.0f;
    float rate = kLegacyContinuousRatePerHour;


    for (int i = 0; i < RESOURCE_COUNT; ++i) {
        production[i] = 1.0f;
    }

    for (int regionId : country->regions) {
        auto it = regionProduction.find(regionId);
        if (it == regionProduction.end()) continue;

        auto regionBuildings = country->buildingManager.getInRegion(regionId);

        for (auto& [res, amount] : it->second) {
            float bonus = 0.0f;
            float refineryMult = 1.0f;

            for (auto& bld : regionBuildings) {

                if (bld.type == BuildingType::Mine) {
                    bonus += 0.5f;
                }

                if (bld.type == BuildingType::OilWell && res == Resource::Oil) {
                    bonus += 0.5f;
                }

                if (bld.type == BuildingType::Refinery && res == Resource::Oil) {
                    refineryMult *= 1.3f;
                }
            }

            float multiplier = std::min((1.0f + bonus) * refineryMult, 5.0f);
            production[static_cast<int>(res)] += amount * multiplier;
        }
    }


    for (int i = 0; i < RESOURCE_COUNT; ++i) {
        consumption[i] = 0.0f;
    }

    int divCount = static_cast<int>(country->divisions.size());
    int factoryCount = country->factories;


    consumption[static_cast<int>(Resource::Oil)]    += divCount * 0.5f;
    consumption[static_cast<int>(Resource::Steel)]  += divCount * 0.3f;
    consumption[static_cast<int>(Resource::Rubber)] += divCount * 0.1f;


    consumption[static_cast<int>(Resource::Steel)]    += factoryCount * 0.2f;
    consumption[static_cast<int>(Resource::Aluminum)] += factoryCount * 0.15f;


    int armsCount = country->buildingManager.countAll(BuildingType::MilitaryFactory);
    consumption[static_cast<int>(Resource::Tungsten)] += armsCount * 0.3f;
    consumption[static_cast<int>(Resource::Chromium)] += armsCount * 0.2f;


    for (int i = 0; i < RESOURCE_COUNT; ++i) {
        float net = production[i] + tradeImports[i] - consumption[i] - tradeExports[i];
        stockpile[i] += net * rate;
        stockpile[i] = std::max(0.0f, std::min(maxStockpile, stockpile[i]));
        deficit[i] = (stockpile[i] <= 0.0f && net < 0.0f) ? 1.0f : 0.0f;
    }
}


void TradeContract::update(GameState& gs) {
    if (!active) return;

    Country* expObj = gs.getCountry(exporter);
    Country* impObj = gs.getCountry(importer);
    if (!expObj || !impObj) {
        active = false;
        return;
    }

    for (auto& enemy : expObj->atWarWith) {
        if (enemy == importer) {
            active = false;
            return;
        }
    }

    constexpr float kLegacyContinuousRatePerHour = 1.0f / 6.0f;
    float rate = kLegacyContinuousRatePerHour;

    int ri = static_cast<int>(resource);

    float available = expObj->resourceManager.stockpile[ri];
    float actual = std::min(amount * rate, available);

    if (actual > 0.0f) {
        expObj->resourceManager.stockpile[ri] -= actual;
        expObj->resourceManager.tradeExports[ri] = amount;
        impObj->resourceManager.stockpile[ri] += actual;
        impObj->resourceManager.tradeImports[ri] = amount;

        float payment = actual * price;
        impObj->money -= payment;
        expObj->money += payment;
    }
}

void TradeContract::cancel() {
    active = false;
}

void TradeContract::cancelWithCleanup(GameState& gs) {
    active = false;
    int ri = static_cast<int>(resource);
    Country* expObj = gs.getCountry(exporter);
    Country* impObj = gs.getCountry(importer);
    if (expObj) expObj->resourceManager.tradeExports[ri] = 0.0f;
    if (impObj) impObj->resourceManager.tradeImports[ri] = 0.0f;
}

float TradeContract::totalValue() const {
    return amount * price;
}
