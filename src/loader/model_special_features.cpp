#include "pch.h"
#include "model_special_features.h"
#include <unordered_set>

CFLAModelSpecialFeaturesLoader FLAModelSpecialFeaturesLoader;

namespace
{
    const char* kMarker = "; comp.injector added model_special_features";

    std::string GetBasePathWithBackup(const std::string& settingsPath)
    {
        std::string backupPath = settingsPath + ".back";
        if (std::filesystem::exists(settingsPath) && !std::filesystem::exists(backupPath))
        {
            try
            {
                std::filesystem::copy_file(settingsPath, backupPath, std::filesystem::copy_options::overwrite_existing);
            }
            catch (const std::exception&)
            {
            }
        }

        if (std::filesystem::exists(backupPath))
        {
            return backupPath;
        }

        return settingsPath;
    }

    bool HasMarker(const std::string& settingsPath)
    {
        std::ifstream in(settingsPath);
        if (!in.is_open())
        {
            return false;
        }

        std::string line;
        while (getline(in, line))
        {
            if (line.find(kMarker) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }
}

void CFLAModelSpecialFeaturesLoader::UpdateModelSpecialFeaturesFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/model_special_features.dat");
    std::string settingsPathTemp = settingsPath + ".tmp";
    std::string basePath = GetBasePathWithBackup(settingsPath);
    auto isCommentOrEmpty = [](const std::string &value)
        {
            const auto firstNonWhitespace = value.find_first_not_of(" \t\r\n");
            if (firstNonWhitespace == std::string::npos)
            {
                return true;
            }

            const std::string_view trimmed(value.c_str() + firstNonWhitespace, value.size() - firstNonWhitespace);

            return trimmed.starts_with(";") || trimmed.starts_with("#") || trimmed.starts_with("//");
        };

    if (!std::filesystem::exists(basePath))
    {
        return;
    }

    if (store.empty())
    {
        std::ifstream in(basePath, std::ios::binary);
        std::ofstream out(settingsPathTemp, std::ios::binary | std::ios::trunc);
        if (!in.is_open() || !out.is_open())
        {
            return;
        }

        out << in.rdbuf();
        in.close();
        out.close();

        std::filesystem::remove(settingsPath);
        std::filesystem::rename(settingsPathTemp, settingsPath);
        return;
    }

    std::unordered_set<std::string> writtenLines;
    std::unordered_set<std::string> existingLines;

    std::ifstream in(basePath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        while (getline(in, line))
        {
            if (line.find(kMarker) != std::string::npos)
            {
                ignoreLines = true;
                continue;
            }

            if (ignoreLines)
            {
                continue;
            }

            if (isCommentOrEmpty(line))
            {
                out << line << "\n";
                continue;
            }

            out << line << "\n";
            existingLines.insert(line);
        }

        out << kMarker << "\n";

        for (const auto &e : store)
        {
            if (existingLines.count(e) > 0)
            {
                continue;
            }

            if (writtenLines.insert(e).second)
            {
                out << e << "\n";
            }
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
    if (store.empty() && !HasMarker(GAME_PATH((char*)"data/model_special_features.dat")))
    {
        return;
    }

    UpdateModelSpecialFeaturesFile();
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
