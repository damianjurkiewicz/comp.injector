#pragma once
#include <vector>

class CFLAModelSpecialFeaturesLoader
{
private:
    std::vector<std::string> store;
    void UpdateModelSpecialFeaturesFile();

public:
    void AddLine(const std::string &line);
    void Parse(const std::string &line);
    void Process();
};

extern CFLAModelSpecialFeaturesLoader FLAModelSpecialFeaturesLoader;
