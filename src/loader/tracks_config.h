#pragma once
#include <vector>

class CFLATracksConfigLoader
{
private:
    std::vector<std::string> store;
    void UpdateTracksConfigFile();

public:
    void Parse(const std::string &line);
    void Process();
};

extern CFLATracksConfigLoader FLATracksConfigLoader;
