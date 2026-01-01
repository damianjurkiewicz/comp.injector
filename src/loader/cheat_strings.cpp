#include "pch.h"
#include "cheat_strings.h"
#include <algorithm>
#include <cctype>
#include <unordered_set>

CFLACheatStringsLoader FLACheatStringsLoader;

namespace {
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
}

void CFLACheatStringsLoader::UpdateCheatStringsFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/cheatStrings.dat");
    std::string settingsPathTemp = settingsPath + ".bak";
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

    if (!std::filesystem::exists(settingsPath))
    {
        return;
    }

    std::unordered_set<std::string> linesToAdd(store.begin(), store.end());
    std::unordered_set<std::string> writtenLines;

    std::ifstream in(settingsPath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        const std::string marker = "; comp.injector added cheatStrings";

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

            if (!linesToAdd.count(line))
            {
                continue;
            }

            if (writtenLines.insert(line).second)
            {
                out << line << "\n";
            }
        }

        out << marker << "\n";

        for (const auto &e : store)
        {
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

void CFLACheatStringsLoader::Process()
{
    if (!store.empty())
    {
        UpdateCheatStringsFile();
    }
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
