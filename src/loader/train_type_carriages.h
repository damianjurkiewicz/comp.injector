#pragma once
#include <vector>

class CFLATrainTypeCarriagesLoader
{
private:
    std::vector<std::string> store;
    void UpdateTrainTypeCarriagesFile();

public:
    void Parse(const std::string &line);
    void Process();
};

extern CFLATrainTypeCarriagesLoader FLATrainTypeCarriagesLoader;
