#include "pch.h"
#include "cheat_strings.h"
#include "logger.h"
#include <algorithm>
#include <cctype>
#include <unordered_set>

CFLACheatStringsLoader FLACheatStringsLoader;

namespace {
const char* kLogPrefix = "CHEAT_STRINGS";
const char* kMarker = "; comp.injector added cheatStrings";

std::string TrimCopy(const std::string &value)
{
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string GetBasePathWithBackup(const std::string &settingsPath)
{
    std::string backupPath = settingsPath + ".back";
    if (std::filesystem::exists(settingsPath) && !std::filesystem::exists(backupPath))
    {
        try
        {
            std::filesystem::copy_file(settingsPath, backupPath, std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::exception &)
        {
        }
    }

    if (std::filesystem::exists(backupPath))
    {
        return backupPath;
    }

    return settingsPath;
}

bool HasMarker(const std::string &settingsPath)
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

void CFLACheatStringsLoader::UpdateCheatStringsFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/cheatStrings.dat");
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
        Logger.Log(std::string(kLogPrefix) + ": base file not found at " + basePath);
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
        Logger.Log(std::string(kLogPrefix) + ": refreshed " + settingsPath);
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
        Logger.Log(std::string(kLogPrefix) + ": updated " + settingsPath);
    }
    else
    {
        if (in.is_open()) in.close();
        if (out.is_open()) out.close();
    }
}

void CFLACheatStringsLoader::Process()
{
    if (store.empty() && !HasMarker(GAME_PATH((char*)"data/cheatStrings.dat")))
    {
        Logger.Log(std::string(kLogPrefix) + ": no entries and no marker, skipping.");
        return;
    }

    Logger.Log(std::string(kLogPrefix) + ": processing cheat strings.");
    UpdateCheatStringsFile();
}

void CFLACheatStringsLoader::AddLine(const std::string &line)
{
    store.push_back(line);
}

void CFLACheatStringsLoader::Parse(const std::string &line)
{
    const auto commaPos = line.find(',');
    if (commaPos == std::string::npos)
    {
        return;
    }

    const std::string idPart = TrimCopy(line.substr(0, commaPos));
    if (idPart.empty())
    {
        return;
    }

    int index = 0;
    try
    {
        index = std::stoi(idPart);
    }
    catch (const std::exception &)
    {
        return;
    }

    if (index <= 91)
    {
        return;
    }

    std::string remainder = line.substr(commaPos + 1);
    const auto commentPos = remainder.find('#');
    if (commentPos != std::string::npos)
    {
        remainder = remainder.substr(0, commentPos);
    }

    if (TrimCopy(remainder).empty())
    {
        return;
    }

    store.push_back(line);
}
