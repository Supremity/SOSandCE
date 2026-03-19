#pragma once
#include "screens/screen.h"

class SettingsScreen : public Screen {
public:
    void enter(App& app) override;
    void handleInput(App& app, const InputState& input) override;
    void update(App& app, float dt) override;
    void render(App& app) override;

private:
    int hoveredItem_ = -1;
    float tempUISize_, tempMusicVol_, tempSoundVol_;
    int tempFPS_;
    bool tempFog_;
};
