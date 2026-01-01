#pragma once
#include <vector>

class CFLAMeleeConfigLoader
{
private:
    std::vector<std::string> store;
    void UpdateMeleeConfigFile();

public:
    void AddLine(const std::string &line);
    void Parse(const std::string &line);
    void Process();
};

extern CFLAMeleeConfigLoader FLAMeleeConfigLoader;
