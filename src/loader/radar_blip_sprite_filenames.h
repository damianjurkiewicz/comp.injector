#pragma once
#include <vector>

class CFLARadarBlipSpriteFilenamesLoader
{
private:
    std::vector<std::string> store;
    void UpdateRadarBlipSpriteFilenamesFile();

public:
    void Parse(const std::string &line);
    void Process();
};

extern CFLARadarBlipSpriteFilenamesLoader FLARadarBlipSpriteFilenamesLoader;
