#pragma once
#include <vector>

class CFLAWeaponConfigLoader
{
private:
    std::vector<std::string> store;
    void UpdateWeaponConfigFile();

public:
    void Parse(const std::string &line);
    void Process();
};

extern CFLAWeaponConfigLoader FLAWeaponConfigLoader;
