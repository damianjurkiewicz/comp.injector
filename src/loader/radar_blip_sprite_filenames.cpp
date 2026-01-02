#include "pch.h"
#include "radar_blip_sprite_filenames.h"
#include <sstream>
#include <unordered_set>

CFLARadarBlipSpriteFilenamesLoader FLARadarBlipSpriteFilenamesLoader;

namespace
{
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
}

void CFLARadarBlipSpriteFilenamesLoader::UpdateRadarBlipSpriteFilenamesFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/gtasa_radarBlipSpriteFilenames.dat");
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

    std::unordered_set<std::string> writtenLines;
    std::unordered_set<std::string> existingLines;

    std::ifstream in(basePath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        const std::string marker = "; comp.injector added gtasa_radarBlipSpriteFilenames";

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

            if (isCommentOrEmpty(line))
            {
                out << line << "\n";
                continue;
            }

            out << line << "\n";
            existingLines.insert(line);
        }

        out << marker << "\n";

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

void CFLARadarBlipSpriteFilenamesLoader::Process()
{
    if (!store.empty())
    {
        UpdateRadarBlipSpriteFilenamesFile();
    }
}

void CFLARadarBlipSpriteFilenamesLoader::AddLine(const std::string &line)
{
    store.push_back(line);
}

void CFLARadarBlipSpriteFilenamesLoader::Parse(const std::string &line)
{
    std::istringstream stream(line);
    int index = 0;
    std::string name;
    std::string texture;

    if (!(stream >> index >> name >> texture))
    {
        return;
    }

    const bool isNull = name == "NULL";
    const bool isRadar = name.starts_with("radar");
    const bool isArrow = name.starts_with("arrow");

    if (isNull || isRadar || isArrow)
    {
        store.push_back(line);
    }
}
