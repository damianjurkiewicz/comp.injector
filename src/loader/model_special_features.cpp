#include "pch.h"
#include "model_special_features.h"
#include <unordered_set>

CFLAModelSpecialFeaturesLoader FLAModelSpecialFeaturesLoader;

void CFLAModelSpecialFeaturesLoader::UpdateModelSpecialFeaturesFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/model_special_features.dat");
    std::string settingsPathTemp = settingsPath + ".bak";

    if (!std::filesystem::exists(settingsPath))
    {
        return;
    }

    std::unordered_set<std::string> linesToAdd(store.begin(), store.end());

    std::ifstream in(settingsPath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        const std::string marker = "; comp.injector added model_special_features";

        while (getline(in, line))
        {
            if (line.find(marker) != std::string::npos)
            {
                ignoreLines = true;
                continue;
            }

            if (ignoreLines)
            {
                continue;
            }

            if (linesToAdd.count(line))
            {
                continue;
            }

            out << line << "\n";
        }

        out << marker << "\n";

        for (const auto &e : store)
        {
            out << e << "\n";
        }

        in.close();
        out.close();

        std::filesystem::remove(settingsPath);
        std::filesystem::rename(settingsPathTemp, settingsPath);
    }
    else
    {
        if (in.is_open()) in.close();
        if (out.is_open()) out.close();
    }
}

void CFLAModelSpecialFeaturesLoader::Process()
{
    if (!store.empty())
    {
        UpdateModelSpecialFeaturesFile();
    }
}

void CFLAModelSpecialFeaturesLoader::AddLine(const std::string &line)
{
    store.push_back(line);
}

void CFLAModelSpecialFeaturesLoader::Parse(const std::string &line)
{
    char first[256] = {};
    char second[256] = {};

    int count = sscanf(line.c_str(), "%255s %255s", first, second);

    if (count == 2 && strnlen(first, sizeof(first)) > 0 && strnlen(second, sizeof(second)) > 0)
    {
        store.push_back(line);
    }
}
