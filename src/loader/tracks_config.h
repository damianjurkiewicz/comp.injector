#pragma once
#include <vector>

class CFLATracksConfigLoader
{
private:
    std::vector<std::string> store;
    void UpdateTracksConfigFile();

public:
    void AddLine(const std::string &line);
    void Parse(const std::string &line);
    void Process();
};

extern CFLATracksConfigLoader FLATracksConfigLoader;
