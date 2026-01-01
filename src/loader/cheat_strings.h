#pragma once
#include <vector>

class CFLACheatStringsLoader
{
private:
    std::vector<std::string> store;
    void UpdateCheatStringsFile();

public:
    void AddLine(const std::string &line);
    void Parse(const std::string &line);
    void Process();
};

extern CFLACheatStringsLoader FLACheatStringsLoader;
