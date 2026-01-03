#include "pch.h"
#include "mva_loader.h"
#include "injector_paths.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm> // Potrzebne dla std::transform jeśli ToLower byłoby inne, ale tu mamy własne

CMvaLoader MvaLoader;

namespace
{
    std::string ToLower(std::string value)
    {
        for (char& ch : value)
        {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return value;
    }

    std::string ToUpper(std::string value)
    {
        for (char& ch : value)
        {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        return value;
    }


    bool IsHiddenFolder(const std::filesystem::path& path)
    {
        std::string folderName = path.filename().string();
        return !folderName.empty() && folderName[0] == '.';
    }

    std::vector<std::string> SplitSectionNames(const std::string& section)
    {
        std::vector<std::string> names;
        std::string current;
        for (char ch : section)
        {
            if (ch == ',')
            {
                if (!current.empty())
                {
                    size_t start = current.find_first_not_of(" \t\r\n");
                    size_t end = current.find_last_not_of(" \t\r\n");
                    if (start != std::string::npos && end != std::string::npos)
                    {
                        names.push_back(current.substr(start, end - start + 1));
                    }
                }
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }

        if (!current.empty())
        {
            size_t start = current.find_first_not_of(" \t\r\n");
            size_t end = current.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos)
            {
                names.push_back(current.substr(start, end - start + 1));
            }
        }

        return names;
    }

    void RestoreIniFileFromInjector(const std::filesystem::path& iniPath)
    {
        std::filesystem::path injectorPath = InjectorPaths::GetInjectorPathFor(iniPath);
        if (injectorPath.empty() || !std::filesystem::exists(injectorPath))
        {
            return;
        }

        try
        {
            std::filesystem::copy_file(injectorPath, iniPath, std::filesystem::copy_options::overwrite_existing);
            Logger.Log("MVA: restored " + iniPath.string() + " from " + injectorPath.string());
        }
        catch (const std::exception&)
        {
            Logger.Log("MVA: failed to restore " + iniPath.string() + " from " + injectorPath.string());
        }
    }

    std::filesystem::path GetBasePath(const std::filesystem::path& iniPath)
    {
        std::filesystem::path injectorPath = InjectorPaths::GetInjectorPathFor(iniPath);
        if (!injectorPath.empty() && std::filesystem::exists(injectorPath))
        {
            return injectorPath;
        }

        return iniPath;
    }

    const std::unordered_set<std::string> kForceReplaceKeys = {
        "MergeInteriorsWithCitiesAndZones",
        "DontInheritBehaviour",
        "MergeZonesWithCities",
        "DisableOnMission",
        "UseParentVoice",
        "Voice",
        "MergeZonesWithGlobal",
        "ReplaceDriver",
        "ReplacePassengers",
        "UseOnlyGroups",
        "DriverGroup1",
        "DriverGroup2",
        "DriverGroup3",
        "DriverGroup4",
        "DriverGroup5",
        "DriverGroup6",
        "DriverGroup7",
        "DriverGroup8",
        "DriverGroup9",
        "PassengerGroup1",
        "PassengerGroup2",
        "PassengerGroup3",
        "PassengerGroup4",
        "PassengerGroup5",
        "PassengerGroup6",
        "PassengerGroup7",
        "PassengerGroup8",
        "PassengerGroup9",
        "TuningChance",
        "TuningFullBodykit",
        "TrailersHealth",
        "RecursiveVariations",
        "UseParentVoices",
        "EnableCloneRemover",
        "CloneRemoverDisableOnMission",
        "CloneRemoverIncludeVehicleOccupants",
        "CloneRemoverSpawnDelay",
        "ChangeCarGenerators",
        "ChangeScriptedCars",
        "DisablePayAndSpray",
        "EnableLights",
        "EnableSideMissions",
        "EnableSiren",
        "EnableSpecialFeatures",
        "EnablePeds",
        "EnableSpecialPeds",
        "EnableVehicles",
        "EnablePedWeapons",
        "LoadSettingsImmediately",
        "EnableStreamingFix",
        "DisableKey",
        "ReloadKey",
        "EnableLog",
        "LogJumps",
        "ForceEnable",
        "LoadStage",
        "TrackReferenceCounts"
    };
}

void RestoreIniFilesFromInjector(const std::filesystem::path& modloaderRoot)
{
    std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;
    for (auto it = std::filesystem::recursive_directory_iterator(modloaderRoot, options);
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
        if (ToLower(iniPath.extension().string()) != ".ini")
        {
            continue;
        }

        std::filesystem::path injectorPath = InjectorPaths::GetInjectorPathFor(iniPath);
        if (injectorPath.empty() || !std::filesystem::exists(injectorPath))
        {
            continue;
        }

        try
        {
            std::filesystem::copy_file(injectorPath, iniPath, std::filesystem::copy_options::overwrite_existing);
            Logger.Log("MVA: restored " + iniPath.string() + " from " + injectorPath.string());
        }
        catch (const std::exception&)
        {
            Logger.Log("MVA: failed to restore " + iniPath.string() + " from " + injectorPath.string());
        }
    }
}


void CMvaLoader::Process()
{
    const std::filesystem::path modloaderRoot = GAME_PATH((char*)"modloader");
    if (modloaderRoot.empty() || !std::filesystem::exists(modloaderRoot))
    {
        Logger.Log("MVA: modloader folder not found, skipping.");
        return;
    }

    Logger.Log(std::string("MVA: scanning modloader at ") + modloaderRoot.string());

    std::vector<MvaFileEntry> entries;
    CollectMvaFiles(modloaderRoot, entries);

    if (entries.empty())
    {
        Logger.Log("MVA: no .mva files found.");

        std::unordered_map<std::string, std::filesystem::path> originalIniCache;
        const std::vector<std::string> kRestoreNames = {
            "ModelVariations_Peds.ini",
            "ModelVariations_PedWeapons.ini",
            "ModelVariations_Vehicles.ini",
            "ModelVariations.ini",
        };

        for (const auto& name : kRestoreNames)
        {
            std::filesystem::path originalIni = FindOriginalIni(modloaderRoot, name, originalIniCache);
            if (originalIni.empty())
            {
                continue;
            }

            std::filesystem::path injectorPath = InjectorPaths::GetInjectorPathFor(originalIni);
            if (injectorPath.empty() || !std::filesystem::exists(injectorPath))
            {
                continue;
            }

            try
            {
                std::filesystem::copy_file(injectorPath, originalIni, std::filesystem::copy_options::overwrite_existing);
                Logger.Log("MVA: restored " + originalIni.string() + " from " + injectorPath.string());
            }
            catch (const std::exception&)
            {
                Logger.Log("MVA: failed to restore " + originalIni.string() + " from " + injectorPath.string());
            }
        }

        return;
    }

    Logger.Log("MVA: found " + std::to_string(entries.size()) + " .mva files.");

    const std::filesystem::path modloaderIni = modloaderRoot / "modloader.ini";
    std::unordered_map<std::string, int> priorities = LoadPriorities(modloaderIni);
    Logger.Log("MVA: loaded " + std::to_string(priorities.size()) + " mod priorities.");

    std::unordered_map<std::string, std::vector<MvaFileEntry>> grouped;
    for (auto& entry : entries)
    {
        auto it = priorities.find(entry.modName);
        if (it != priorities.end())
        {
            entry.priority = it->second;
        }

        std::filesystem::path targetFilename = entry.sourcePath.filename();
        targetFilename.replace_extension(".ini");
        Logger.Log("MVA: target ini " + targetFilename.string());
        grouped[targetFilename.string()].push_back(entry);
    }

    Logger.Log("MVA: grouped into " + std::to_string(grouped.size()) + " target files.");

    bool didUpdateAnything = false;
    std::unordered_map<std::string, std::filesystem::path> originalIniCache;
    for (auto& group : grouped)
    {
        auto& files = group.second;
        Logger.Log("MVA: processing target " + group.first + " with " + std::to_string(files.size()) + " source files.");
        std::stable_sort(files.begin(), files.end(), [](const MvaFileEntry& left, const MvaFileEntry& right)
            {
                if (left.priority != right.priority)
                {
                    return left.priority < right.priority;
                }

                return left.sourcePath.string() < right.sourcePath.string();
            });

        std::filesystem::path originalIni = FindOriginalIni(modloaderRoot, group.first, originalIniCache);
        if (originalIni.empty())
        {
            Logger.Log("MVA: original ini not found for " + group.first);
            continue;
        }

        Logger.Log("MVA: original ini " + originalIni.string());

        std::filesystem::path basePath = GetBasePath(originalIni);
        if (!std::filesystem::exists(basePath))
        {
            Logger.Log("MVA: base ini not found for " + group.first + ", restoring from injector.");
            RestoreIniFileFromInjector(originalIni);
            continue;
        }

        IniData finalData = ReadIniData(basePath);

        size_t index = 0;
        while (index < files.size())
        {
            const int priority = files[index].priority;
            Logger.Log("MVA: merging priority " + std::to_string(priority));
            IniData mergedData;
            while (index < files.size() && files[index].priority == priority)
            {
                IniData content = ReadIniData(files[index].sourcePath);
                Logger.Log("MVA: reading " + files[index].sourcePath.string());
                MergeIniData(mergedData, content);
                ++index;
            }

            ReplaceIniData(finalData, mergedData);
        }

        bool updatedThisFile = false;
        if (finalData.empty())
        {
            Logger.Log("MVA: final content empty for " + group.first + ", restoring from injector.");
            RestoreIniFileFromInjector(originalIni);
            continue;
        }

        std::string finalContent = WriteIniData(finalData);
        if (finalContent.empty())
        {
            Logger.Log("MVA: no ini data to write for " + group.first + ", restoring from injector.");
            RestoreIniFileFromInjector(originalIni);
            continue;
        }

        std::filesystem::path tempPath = originalIni;
        tempPath += ".tmp";

        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            Logger.Log("MVA: failed to write temp file " + tempPath.string());
            continue;
        }

        out.write(finalContent.data(), static_cast<std::streamsize>(finalContent.size()));
        out.close();

        try
        {
            std::filesystem::remove(originalIni);
            std::filesystem::rename(tempPath, originalIni);
            Logger.Log("MVA: updated " + originalIni.string() + " using injector base");
            didUpdateAnything = true;
            updatedThisFile = true;
        }
        catch (const std::exception& e)
        {
            Logger.Log(std::string("MVA: error swapping files: ") + e.what());
        }

        if (!updatedThisFile)
        {
            RestoreIniFileFromInjector(originalIni);
        }
    }

    if (!didUpdateAnything)
    {
        Logger.Log("MVA: nothing updated from .mva files, restoring known INIs from injector when available.");

        const std::vector<std::string> kRestoreNames = {
            "ModelVariations_Peds.ini",
            "ModelVariations_PedWeapons.ini",
            "ModelVariations_Vehicles.ini",
            "ModelVariations.ini",
        };

        for (const auto& name : kRestoreNames)
        {
            std::filesystem::path originalIni = FindOriginalIni(modloaderRoot, name, originalIniCache);
            if (originalIni.empty())
            {
                continue;
            }

            std::filesystem::path injectorPath = InjectorPaths::GetInjectorPathFor(originalIni);
            if (injectorPath.empty() || !std::filesystem::exists(injectorPath))
            {
                continue;
            }

            try
            {
                std::filesystem::copy_file(injectorPath, originalIni, std::filesystem::copy_options::overwrite_existing);
                Logger.Log("MVA: restored " + originalIni.string() + " from " + injectorPath.string());
            }
            catch (const std::exception&)
            {
                Logger.Log("MVA: failed to restore " + originalIni.string() + " from " + injectorPath.string());
            }
        }
    }
}

void CMvaLoader::CollectMvaFiles(const std::filesystem::path& modloaderRoot, std::vector<MvaFileEntry>& entries) const
{
    for (const auto& entry : std::filesystem::directory_iterator(modloaderRoot))
    {
        if (entry.is_directory())
        {
            if (IsHiddenFolder(entry.path()))
            {
                continue;
            }

            for (auto it = std::filesystem::recursive_directory_iterator(entry.path());
                it != std::filesystem::recursive_directory_iterator();
                ++it)
            {
                if (it->is_directory())
                {
                    if (IsHiddenFolder(it->path()))
                    {
                        it.disable_recursion_pending();
                        continue;
                    }
                    continue;
                }

                if (!it->is_regular_file())
                {
                    continue;
                }

                if (ToLower(it->path().extension().string()) == ".mva")
                {
                    std::filesystem::path relativePath = std::filesystem::relative(it->path(), modloaderRoot);
                    auto relIt = relativePath.begin();
                    if (relIt == relativePath.end())
                    {
                        continue;
                    }

                    std::string modName = relIt->string();
                    if (modName.empty())
                    {
                        continue;
                    }

                    Logger.Log("MVA: found " + it->path().string() + " in mod " + modName);
                    entries.push_back({ it->path(), modName, 0 });
                }
            }
        }
    }
}

std::unordered_map<std::string, int> CMvaLoader::LoadPriorities(const std::filesystem::path& modloaderIni) const
{
    std::unordered_map<std::string, int> priorities;
    if (!std::filesystem::exists(modloaderIni))
    {
        Logger.Log("MVA: modloader.ini not found, default priorities assumed.");
        return priorities;
    }

    linb::ini ini;
    if (!ini.load_file(modloaderIni.string()))
    {
        Logger.Log("MVA: failed to read modloader.ini.");
        return priorities;
    }

    auto section = ini.find("Profiles.Default.Priority");
    if (section == ini.end())
    {
        Logger.Log("MVA: Profiles.Default.Priority section not found.");
        return priorities;
    }

    for (const auto& kv : section->second)
    {
        try
        {
            int value = std::stoi(kv.second, nullptr, 10);
            priorities[kv.first] = value;
        }
        catch (const std::exception&)
        {
            Logger.Log("MVA: invalid priority for mod " + kv.first);
            continue;
        }
    }

    return priorities;
}

std::filesystem::path CMvaLoader::FindOriginalIni(
    const std::filesystem::path& modloaderRoot,
    const std::string& filename,
    std::unordered_map<std::string, std::filesystem::path>& cache) const
{
    auto cached = cache.find(filename);
    if (cached != cache.end())
    {
        return cached->second;
    }

    std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;
    for (auto it = std::filesystem::recursive_directory_iterator(modloaderRoot, options);
        it != std::filesystem::recursive_directory_iterator();
        ++it)
    {
        if (it->is_directory())
        {
            if (IsHiddenFolder(it->path()))
            {
                it.disable_recursion_pending();
                continue;
            }
            continue;
        }

        if (!it->is_regular_file())
        {
            continue;
        }

        if (it->path().filename().string() == filename && ToLower(it->path().extension().string()) == ".ini")
        {
            cache[filename] = it->path();
            return it->path();
        }
    }

    cache[filename] = std::filesystem::path();
    return {};
}

CMvaLoader::IniData CMvaLoader::ReadIniData(const std::filesystem::path& path) const
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        return {};
    }

    IniData data;
    std::string line;
    std::vector<std::string> currentSections;
    while (getline(in, line))
    {
        const auto firstNonWhitespace = line.find_first_not_of(" \t\r\n");
        if (firstNonWhitespace == std::string::npos)
        {
            continue;
        }

        const std::string_view trimmed(line.c_str() + firstNonWhitespace, line.size() - firstNonWhitespace);
        if (trimmed.starts_with(";") || trimmed.starts_with("#") || trimmed.starts_with("//"))
        {
            continue;
        }

        std::string trimmedLine = line;
        trimmedLine.erase(0, firstNonWhitespace);
        trimmedLine.erase(trimmedLine.find_last_not_of(" \t\r\n") + 1);

        // --- SEKCJE ---
        if (trimmedLine.size() >= 2 && trimmedLine.front() == '[' && trimmedLine.back() == ']')
        {
            std::string sectionName = trimmedLine.substr(1, trimmedLine.size() - 2);
            std::vector<std::string> rawSections = SplitSectionNames(sectionName);
            currentSections.clear();

            for (const auto& rawSec : rawSections)
            {
                // Jeśli to "Settings" (bez względu na wielkość liter), zachowaj oryginał
                if (ToLower(rawSec) == "settings")
                {
                    currentSections.push_back(rawSec);
                }
                else
                {
                    // Reszta sekcji -> WIELKIE LITERY
                    currentSections.push_back(ToUpper(rawSec));
                }
            }
            continue;
        }

        // --- KLUCZE I WARTOŚCI ---
        const auto equals = trimmedLine.find('=');
        if (equals == std::string::npos || currentSections.empty())
        {
            continue;
        }

        std::string key = trimmedLine.substr(0, equals);
        key.erase(key.find_last_not_of(" \t\r\n") + 1);

        // UWAGA: Usunięto ToLower(key) -> Klucze są teraz Case-Sensitive (np. RecursiveVariations)

        std::string value = trimmedLine.substr(equals + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));

        if (key.empty())
        {
            continue;
        }

        for (const auto& sectionName : currentSections)
        {
            data[sectionName][key] = value;
        }
    }

    in.close();
    return data;
}

void CMvaLoader::MergeIniData(IniData& target, const IniData& source) const
{
    for (const auto& sectionPair : source)
    {
        auto& section = target[sectionPair.first];
        for (const auto& kv : sectionPair.second)
        {
            auto& value = section[kv.first];
            if (kForceReplaceKeys.count(kv.first) > 0)
            {
                value = kv.second;
                continue;
            }
            if (!value.empty())
            {
                // ZMIANA: Użycie przecinka zamiast spacji przy łączeniu wartości
                value += ", ";
            }
            value += kv.second;
        }
    }
}

void CMvaLoader::ReplaceIniData(IniData& target, const IniData& source) const
{
    for (const auto& sectionPair : source)
    {
        // ZMIANA: Zamiast iterować po kluczach i podmieniać pojedyncze wartości,
        // przypisujemy całą zawartość sekcji z 'source' do 'target'.
        // Dzięki temu, jeśli w 'target' (niższy priorytet) były klucze, 
        // których nie ma w 'source' (wyższy priorytet), zostaną one usunięte.
        // Sekcja staje się identyczna jak w pliku o wyższym priorytecie.
        target[sectionPair.first] = sectionPair.second;
    }
}

std::string CMvaLoader::WriteIniData(const IniData& data) const
{
    std::ostringstream out;
    bool firstSection = true;

    // KROK 1: Zapisz sekcję [Settings] jako pierwszą
    auto settingsIt = data.find("Settings");
    if (settingsIt != data.end())
    {
        out << "[Settings]\n";
        for (const auto& kv : settingsIt->second)
        {
            out << kv.first << "=" << kv.second << "\n";
        }
        firstSection = false;
    }

    // KROK 2: Przetwórz pozostałe sekcje
    for (const auto& sectionPair : data)
    {
        if (sectionPair.first == "Settings")
        {
            continue;
        }

        if (!firstSection)
        {
            out << "\n";
        }

        out << "[" << sectionPair.first << "]\n";

        // Kontenery tymczasowe
        std::vector<std::pair<std::string, std::string>> priorityKeys;
        std::vector<std::pair<std::string, std::string>> normalKeys;

        for (const auto& kv : sectionPair.second)
        {
            // WARUNEK PRIORYTETU WIZUALNEGO:
            // 1. Jest na liście kForceReplaceKeys (ważne ustawienia)
            // 2. LUB jest to klucz "Global" (wyjątek na żądanie)
            // 3. LUB zaczyna się od "Wanted" (żeby też były wysoko, jak w Twoim przykładzie)
            bool isVisualPriority = (kForceReplaceKeys.count(kv.first) > 0)
                || (kv.first == "Global")
                || (kv.first.rfind("Wanted", 0) == 0); // rfind(..., 0) == 0 to odpowiednik starts_with

            if (isVisualPriority)
            {
                priorityKeys.push_back(kv);
            }
            else
            {
                normalKeys.push_back(kv);
            }
        }

        // Sortowanie kluczy PRIORYTETOWYCH
        // Tutaj ustalamy sztywną kolejność: Global zawsze pierwszy
        auto prioritySort = [](const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b)
            {
                // Jeśli jeden z kluczy to Global, ma on pierwszeństwo absolutne
                if (a.first == "Global") return true;
                if (b.first == "Global") return false;

                // Jeśli oba to nie Global, sortujemy alfabetycznie
                return a.first < b.first;
            };

        // Sortowanie zwykłych kluczy alfabetycznie
        auto normalSort = [](const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b)
            {
                return a.first < b.first;
            };

        std::sort(priorityKeys.begin(), priorityKeys.end(), prioritySort);
        std::sort(normalKeys.begin(), normalKeys.end(), normalSort);

        // Zapisz priorytetowe
        for (const auto& kv : priorityKeys)
        {
            out << kv.first << "=" << kv.second << "\n";
        }

        // Zapisz resztę
        for (const auto& kv : normalKeys)
        {
            out << kv.first << "=" << kv.second << "\n";
        }

        firstSection = false;
    }

    return out.str();
}
