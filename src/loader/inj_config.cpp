#include "pch.h"
#include "inj_config.h"
#include "logger.h"
#include <fstream>
#include <optional>
#include <unordered_map>
#include <unordered_set>

CInjConfigLoader InjConfigLoader;

namespace
{
    const char* kLogPrefix = "INJ";
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

    std::string ToLowerStr(std::string s)
    {
        for (char& c : s)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    }

    bool IsHiddenFolder(const std::filesystem::path& p)
    {
        const std::string name = p.filename().string();
        return !name.empty() && name[0] == '.';
    }

    std::filesystem::path GetBasePathFromInjector(const std::filesystem::path& iniPath)
    {
        return GetInjectorBasePath(iniPath);
    }

    // Restore *.ini from /injector originals under a root folder (best-effort).
    // This is the "nothing to update => reset to baseline" behavior.
    void RestoreIniFilesFromInjector(const std::filesystem::path& root)
    {
        if (root.empty() || !std::filesystem::exists(root))
        {
            return;
        }

        std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;

        for (auto it = std::filesystem::recursive_directory_iterator(root, options);
            it != std::filesystem::recursive_directory_iterator();
            ++it)
        {
            if (it->is_directory())
            {
                if (IsHiddenFolder(it->path()))
                {
                    it.disable_recursion_pending();
                }
                continue;
            }

            if (!it->is_regular_file())
            {
                continue;
            }

            const std::filesystem::path iniPath = it->path();
            if (ToLowerStr(iniPath.extension().string()) != ".ini")
            {
                continue;
            }

            const std::filesystem::path injectorPath = GetBasePathFromInjector(iniPath);
            if (!std::filesystem::exists(injectorPath))
            {
                continue;
            }

            try
            {
                std::filesystem::copy_file(injectorPath, iniPath, std::filesystem::copy_options::overwrite_existing);
            }
            catch (const std::exception&)
            {
                // Intentionally ignore; restore is best-effort.
            }
        }
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
                if (IsHiddenFolder(it->path()))
                {
                    it.disable_recursion_pending();
                }
                continue;
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
    const std::filesystem::path modloaderRoot = GAME_PATH((char*)"modloader");

    CollectInjFiles(modloaderRoot, injFiles);
    if (!pluginDir.empty())
    {
        CollectInjFiles(pluginDir, injFiles);
    }

    Logger.Log(std::string(kLogPrefix) + ": found " + std::to_string(injFiles.size()) + " .inj files.");

    // If there are no .inj files at all, restore every *.ini from /injector in /modloader.
    if (injFiles.empty())
    {
        Logger.Log(std::string(kLogPrefix) + ": no .inj files found, restoring ini files from /injector.");
        RestoreIniFilesFromInjector(modloaderRoot);
        return;
    }

    for (const auto& file : injFiles)
    {
        ParseFile(file);
    }

    Logger.Log(std::string(kLogPrefix) + ": parsed " + std::to_string(entries.size()) + " entries.");

    // If parsing produced no entries, treat it as "nothing to update" and restore.
    if (entries.empty())
    {
        Logger.Log(std::string(kLogPrefix) + ": no entries parsed, restoring ini files from /injector.");
        RestoreIniFilesFromInjector(modloaderRoot);
        return;
    }

    std::unordered_map<std::string, std::filesystem::path> cache;
    std::unordered_set<std::string> missing;
    std::unordered_map<std::filesystem::path, std::vector<InjEntry>> grouped;

    const std::filesystem::path gameRoot = GetGameRoot();

    for (const auto& entry : entries)
    {
        std::filesystem::path iniPath = LocateIniFile(entry, gameRoot, modloaderRoot, cache, missing);
        if (iniPath.empty())
        {
            continue;
        }

        grouped[iniPath].push_back(entry);
    }

    bool didUpdateAnything = false;
    int updatedFiles = 0;

    for (const auto& group : grouped)
    {
        if (ApplyEntriesToFile(group.first, group.second))
        {
            didUpdateAnything = true;
            ++updatedFiles;
            Logger.Log(std::string(kLogPrefix) + ": updated " + group.first.string());
        }
    }

    // If nothing was actually modified/written, restore baselines in /modloader.
    if (!didUpdateAnything)
    {
        Logger.Log(std::string(kLogPrefix) + ": no changes written, restoring ini files from /injector.");
        RestoreIniFilesFromInjector(modloaderRoot);
        return;
    }

    Logger.Log(std::string(kLogPrefix) + ": updated " + std::to_string(updatedFiles) + " ini files.");
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
    bool implicitBlock = false;
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

        if (implicitBlock)
        {
            InjModifier nextModifier = modifier;
            bool opensBlock = false;
            if (TryParseModifierLine(trimmed, nextModifier, opensBlock))
            {
                modifier = nextModifier;
                inBlock = opensBlock;
                implicitBlock = false;
                state = ParseState::IniFile;
                continue;
            }
        }

        if (inBlock && trimmed == "}")
        {
            inBlock = false;
            implicitBlock = false;
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
                implicitBlock = false;
                state = ParseState::IniFile;
            }
            else
            {
                modifier = InjModifier::Replace;
                inBlock = true;
                implicitBlock = true;
                iniFile = trimmed;
                state = ParseState::Section;
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
            else if (inBlock)
            {
                iniFile = trimmed;
                section.clear();
                state = ParseState::Section;
            }
            break;
        case ParseState::KeyValue:
        {
            std::string nextSection;
            if (TryParseSection(trimmed, nextSection))
            {
                section = nextSection;
                state = ParseState::KeyValue;
                break;
            }

            const auto equals = line.find('=');
            if (equals == std::string::npos)
            {
                if (inBlock)
                {
                    iniFile = trimmed;
                    section.clear();
                    state = ParseState::Section;
                }
                else
                {
                    state = ParseState::Modifier;
                }
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

            if (inBlock)
            {
                state = ParseState::KeyValue;
            }
            else
            {
                iniFile.clear();
                section.clear();
                state = ParseState::Modifier;
            }
            break;
        }
        }
    }

    in.close();
}

bool CInjConfigLoader::ApplyEntriesToFile(const std::filesystem::path& iniPath, const std::vector<InjEntry>& entries) const
{
    std::vector<std::string> lines;
    std::filesystem::path basePath = GetBasePathFromInjector(iniPath);
    if (std::filesystem::exists(basePath))
    {
        std::ifstream in(basePath);
        if (!in.is_open())
        {
            return false;
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

            lines.insert(
                lines.begin() + static_cast<std::vector<std::string>::difference_type>(insertPos),
                entry.key + "=" + updatedValue
            );
            modified = true;
        }
    }

    if (!modified)
    {
        return false;
    }

    std::ofstream out(iniPath, std::ios::trunc);
    if (!out.is_open())
    {
        return false;
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
    return true;
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
