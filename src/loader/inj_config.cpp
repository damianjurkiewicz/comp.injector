#include "pch.h"
#include "inj_config.h"
#include <fstream>
#include <optional>
#include <unordered_map>
#include <unordered_set>

CInjConfigLoader InjConfigLoader;

namespace
{
    bool IsCommentOrEmpty(const std::string& line)
    {
        const auto firstNonWhitespace = line.find_first_not_of(" \t\r\n");
        if (firstNonWhitespace == std::string::npos)
        {
            return true;
        }

        const std::string_view trimmed(line.c_str() + firstNonWhitespace, line.size() - firstNonWhitespace);

        return trimmed.starts_with(";") || trimmed.starts_with("#") || trimmed.starts_with("//");
    }

    std::string Trim(const std::string& value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return "";
        }

        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    std::string TrimLeft(const std::string& value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return "";
        }

        return value.substr(first);
    }

    std::vector<std::string> SplitTokens(const std::string& value)
    {
        std::vector<std::string> tokens;
        size_t i = 0;
        while (i < value.size())
        {
            while (i < value.size() && std::isspace(static_cast<unsigned char>(value[i])))
            {
                ++i;
            }

            if (i >= value.size())
            {
                break;
            }

            size_t start = i;
            while (i < value.size() && !std::isspace(static_cast<unsigned char>(value[i])))
            {
                ++i;
            }

            tokens.emplace_back(value.substr(start, i - start));
        }

        return tokens;
    }

    bool ContainsTokenSequence(const std::string& currentValue, const std::string& candidate)
    {
        if (candidate.empty())
        {
            return true;
        }

        std::vector<std::string> currentTokens = SplitTokens(currentValue);
        std::vector<std::string> candidateTokens = SplitTokens(candidate);
        if (candidateTokens.empty())
        {
            return true;
        }

        if (candidateTokens.size() > currentTokens.size())
        {
            return false;
        }

        for (size_t i = 0; i + candidateTokens.size() <= currentTokens.size(); ++i)
        {
            bool match = true;
            for (size_t j = 0; j < candidateTokens.size(); ++j)
            {
                if (currentTokens[i + j] != candidateTokens[j])
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                return true;
            }
        }

        return false;
    }

    bool TryParseModifierLine(const std::string& line, InjModifier& modifier, bool& opensBlock)
    {
        std::string trimmed = Trim(line);
        opensBlock = false;

        if (trimmed.empty())
        {
            return false;
        }

        if (trimmed.back() == '{')
        {
            trimmed = Trim(trimmed.substr(0, trimmed.size() - 1));
            opensBlock = true;
        }

        if (EqualsIgnoreCase(trimmed, "Replace"))
        {
            modifier = InjModifier::Replace;
            return true;
        }

        if (EqualsIgnoreCase(trimmed, "Merge"))
        {
            modifier = InjModifier::Merge;
            return true;
        }

        return false;
    }

    void AppendMergeValue(std::string& target, const std::string& candidate)
    {
        if (candidate.empty())
        {
            return;
        }

        if (ContainsTokenSequence(target, candidate))
        {
            return;
        }

        if (!target.empty())
        {
            if (!std::isspace(static_cast<unsigned char>(target.back())))
            {
                target += " ";
            }
        }

        target += candidate;
    }

    bool TryParseSection(const std::string& line, std::string& section)
    {
        std::string trimmed = Trim(line);
        if (trimmed.size() < 3 || trimmed.front() != '[' || trimmed.back() != ']')
        {
            return false;
        }

        section = Trim(trimmed.substr(1, trimmed.size() - 2));
        return !section.empty();
    }

    bool EqualsIgnoreCase(const std::string& left, const std::string& right)
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (size_t i = 0; i < left.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(left[i])) != std::tolower(static_cast<unsigned char>(right[i])))
            {
                return false;
            }
        }

        return true;
    }

    std::filesystem::path GetGameRoot()
    {
        char buffer[MAX_PATH] = {};
        if (GetModuleFileNameA(nullptr, buffer, MAX_PATH) == 0)
        {
            return {};
        }

        return std::filesystem::path(buffer).parent_path();
    }

    std::optional<std::filesystem::path> FindFileByName(
        const std::filesystem::path& root,
        const std::string& filename)
    {
        if (root.empty() || !std::filesystem::exists(root))
        {
            return std::nullopt;
        }

        std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;
        for (auto it = std::filesystem::recursive_directory_iterator(root, options);
            it != std::filesystem::recursive_directory_iterator();
            ++it)
        {
            if (it->is_directory())
            {
                const std::string folderName = it->path().filename().string();
                if (!folderName.empty() && folderName[0] == '.')
                {
                    it.disable_recursion_pending();
                    continue;
                }
            }

            if (!it->is_regular_file())
            {
                continue;
            }

            if (it->path().filename().string() == filename)
            {
                return it->path();
            }
        }

        return std::nullopt;
    }
}

void CInjConfigLoader::Process(const std::filesystem::path& pluginDir)
{
    entries.clear();

    std::vector<std::filesystem::path> injFiles;
    CollectInjFiles(GAME_PATH((char*)"modloader"), injFiles);
    if (!pluginDir.empty())
    {
        CollectInjFiles(pluginDir, injFiles);
    }

    for (const auto& file : injFiles)
    {
        ParseFile(file);
    }

    if (entries.empty())
    {
        return;
    }

    std::unordered_map<std::string, std::filesystem::path> cache;
    std::unordered_set<std::string> missing;
    std::unordered_map<std::filesystem::path, std::vector<InjEntry>> grouped;

    const std::filesystem::path gameRoot = GetGameRoot();
    const std::filesystem::path modloaderRoot = GAME_PATH((char*)"modloader");

    for (const auto& entry : entries)
    {
        std::filesystem::path iniPath = LocateIniFile(entry, gameRoot, modloaderRoot, cache, missing);
        if (iniPath.empty())
        {
            continue;
        }

        grouped[iniPath].push_back(entry);
    }

    for (const auto& group : grouped)
    {
        ApplyEntriesToFile(group.first, group.second);
    }
}

void CInjConfigLoader::CollectInjFiles(const std::filesystem::path& dir, std::vector<std::filesystem::path>& files) const
{
    if (dir.empty() || !std::filesystem::exists(dir))
    {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (entry.is_directory())
        {
            std::string folderName = entry.path().filename().string();
            if (!folderName.empty() && folderName[0] == '.')
            {
                continue;
            }
            CollectInjFiles(entry.path(), files);
            continue;
        }

        if (!entry.is_regular_file())
        {
            continue;
        }

        if (entry.path().extension() == ".inj")
        {
            files.push_back(entry.path());
        }
    }
}

void CInjConfigLoader::ParseFile(const std::filesystem::path& path)
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        return;
    }

    enum class ParseState
    {
        Modifier,
        IniFile,
        Section,
        KeyValue
    };

    ParseState state = ParseState::Modifier;
    InjModifier modifier = InjModifier::Replace;
    bool inBlock = false;
    std::string iniFile;
    std::string section;

    std::string line;
    while (getline(in, line))
    {
        if (IsCommentOrEmpty(line))
        {
            continue;
        }

        std::string trimmed = Trim(line);
        if (trimmed.empty())
        {
            continue;
        }

        if (inBlock && trimmed == "}")
        {
            inBlock = false;
            state = ParseState::Modifier;
            continue;
        }

        switch (state)
        {
        case ParseState::Modifier:
        {
            bool opensBlock = false;
            if (TryParseModifierLine(trimmed, modifier, opensBlock))
            {
                inBlock = opensBlock;
                state = ParseState::IniFile;
            }
            break;
        }
        case ParseState::IniFile:
            iniFile = trimmed;
            state = ParseState::Section;
            break;
        case ParseState::Section:
            if (TryParseSection(trimmed, section))
            {
                state = ParseState::KeyValue;
            }
            break;
        case ParseState::KeyValue:
        {
            const auto equals = line.find('=');
            if (equals == std::string::npos)
            {
                state = inBlock ? ParseState::IniFile : ParseState::Modifier;
                break;
            }

            std::string key = Trim(line.substr(0, equals));
            std::string value = TrimLeft(line.substr(equals + 1));

            if (!iniFile.empty() && !section.empty() && !key.empty())
            {
                entries.push_back({
                    modifier,
                    iniFile,
                    section,
                    key,
                    value,
                    path
                    });
            }

            iniFile.clear();
            section.clear();
            state = inBlock ? ParseState::IniFile : ParseState::Modifier;
            break;
        }
        }
    }

    in.close();
}

void CInjConfigLoader::ApplyEntriesToFile(const std::filesystem::path& iniPath, const std::vector<InjEntry>& entries) const
{
    std::vector<std::string> lines;
    if (std::filesystem::exists(iniPath))
    {
        std::ifstream in(iniPath);
        if (!in.is_open())
        {
            return;
        }

        std::string line;
        while (getline(in, line))
        {
            lines.push_back(line);
        }
        in.close();
    }

    std::unordered_map<std::string, std::string> mergedValues;
    mergedValues.reserve(entries.size());
    for (const auto& entry : entries)
    {
        if (entry.modifier != InjModifier::Merge)
        {
            continue;
        }

        std::string key = entry.section + "\n" + entry.key;
        AppendMergeValue(mergedValues[key], entry.value);
    }

    std::unordered_set<std::string> handledMergeKeys;
    handledMergeKeys.reserve(mergedValues.size());

    bool modified = false;
    for (const auto& entry : entries)
    {
        const bool isMerge = entry.modifier == InjModifier::Merge;
        if (isMerge)
        {
            std::string mergeKey = entry.section + "\n" + entry.key;
            if (handledMergeKeys.count(mergeKey) > 0)
            {
                continue;
            }
        }

        std::string sectionName = entry.section;
        size_t sectionStart = lines.size();
        size_t sectionEnd = lines.size();

        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string currentSection;
            if (!TryParseSection(lines[i], currentSection))
            {
                continue;
            }

            if (sectionStart != lines.size())
            {
                sectionEnd = i;
                break;
            }

            if (currentSection == sectionName)
            {
                sectionStart = i;
            }
        }

        if (sectionStart != lines.size() && sectionEnd == lines.size())
        {
            sectionEnd = lines.size();
        }

        if (sectionStart == lines.size())
        {
            if (!lines.empty() && !lines.back().empty())
            {
                lines.push_back("");
            }

            lines.push_back("[" + sectionName + "]");
            lines.push_back(entry.key + "=" + entry.value);
            modified = true;
            continue;
        }

        bool keyFound = false;
        for (size_t i = sectionStart + 1; i < sectionEnd; ++i)
        {
            if (IsCommentOrEmpty(lines[i]))
            {
                continue;
            }

            const auto equals = lines[i].find('=');
            if (equals == std::string::npos)
            {
                continue;
            }

            std::string key = Trim(lines[i].substr(0, equals));
            if (key != entry.key)
            {
                continue;
            }

            size_t valueStart = lines[i].find_first_not_of(" \t", equals + 1);
            std::string prefix = lines[i].substr(0, equals + 1);
            std::string spacing;
            std::string currentValue;

            if (valueStart != std::string::npos)
            {
                spacing = lines[i].substr(equals + 1, valueStart - (equals + 1));
                currentValue = lines[i].substr(valueStart);
            }
            else
            {
                spacing = "";
                currentValue = "";
            }

            std::string updatedValue = entry.value;
            if (isMerge)
            {
                const std::string mergeKey = entry.section + "\n" + entry.key;
                updatedValue = mergedValues[mergeKey];
                handledMergeKeys.insert(mergeKey);
            }

            lines[i] = prefix + spacing + updatedValue;
            keyFound = true;
            modified = true;
            break;
        }

        if (!keyFound)
        {
            size_t insertPos = sectionEnd;
            std::string updatedValue = entry.value;
            if (isMerge)
            {
                const std::string mergeKey = entry.section + "\n" + entry.key;
                updatedValue = mergedValues[mergeKey];
                handledMergeKeys.insert(mergeKey);
            }

            lines.insert(lines.begin() + static_cast<std::vector<std::string>::difference_type>(insertPos),
                entry.key + "=" + updatedValue);
            modified = true;
        }
    }

    if (!modified)
    {
        return;
    }

    if (std::filesystem::exists(iniPath))
    {
        std::filesystem::path backupPath = iniPath;
        backupPath += ".bak";
        if (!std::filesystem::exists(backupPath))
        {
            try
            {
                std::filesystem::copy_file(iniPath, backupPath, std::filesystem::copy_options::overwrite_existing);
            }
            catch (const std::exception&)
            {
            }
        }
    }

    std::ofstream out(iniPath, std::ios::trunc);
    if (!out.is_open())
    {
        return;
    }

    for (size_t i = 0; i < lines.size(); ++i)
    {
        out << lines[i];
        if (i + 1 < lines.size())
        {
            out << "\n";
        }
    }

    out.close();
}

std::filesystem::path CInjConfigLoader::LocateIniFile(
    const InjEntry& entry,
    const std::filesystem::path& gameRoot,
    const std::filesystem::path& modloaderRoot,
    std::unordered_map<std::string, std::filesystem::path>& cache,
    std::unordered_set<std::string>& missing) const
{
    const std::string key = entry.iniFile;
    auto cached = cache.find(key);
    if (cached != cache.end())
    {
        return cached->second;
    }

    if (missing.count(key))
    {
        return {};
    }

    std::filesystem::path iniPath(entry.iniFile);
    if (iniPath.is_absolute() && std::filesystem::exists(iniPath))
    {
        cache[key] = iniPath;
        return iniPath;
    }

    std::filesystem::path localPath = entry.sourcePath.parent_path() / iniPath;
    if (std::filesystem::exists(localPath))
    {
        cache[key] = localPath;
        return localPath;
    }

    const std::string filename = iniPath.filename().string();
    if (auto found = FindFileByName(modloaderRoot, filename); found.has_value())
    {
        cache[key] = *found;
        return *found;
    }

    if (auto found = FindFileByName(gameRoot, filename); found.has_value())
    {
        cache[key] = *found;
        return *found;
    }

    missing.insert(key);
    return {};
}
