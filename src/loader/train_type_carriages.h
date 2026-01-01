#pragma once
#include <vector>

class CFLATrainTypeCarriagesLoader
{
private:
    std::vector<std::string> store;
    void UpdateTrainTypeCarriagesFile();

public:
    void AddLine(const std::string &line);
    void Parse(const std::string &line);
    void Process();
};

extern CFLATrainTypeCarriagesLoader FLATrainTypeCarriagesLoader;
