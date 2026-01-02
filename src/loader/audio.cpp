#include "pch.h"
#include "audio.h"
#include "logger.h"
#include "tVehicleAudioSetting.h"
#include <unordered_set> // <<< ADD THIS INCLUDE FOR THE DUPLICATE CHECK

CFLAAudioLoader FLAAudioLoader;

namespace
{
    const char* kLogPrefix = "AUDIO";
    const char* kMarker = "; comp.injector added vehicles";

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

// <<< MODIFIED FUNCTION TO REBUILD AND REMOVE DUPLICATES >>>
void CFLAAudioLoader::UpdateAudioFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/gtasa_vehicleAudioSettings.cfg");
    std::string settingsPathTemp = settingsPath + ".tmp"; // This is just a temporary file
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

    // --- START OF NEW LOGIC ---
    // 1. Create a "cache" (a hash set) of all lines we intend to add.
    //    This is very fast for lookups.
    std::unordered_set<std::string> writtenLines;
    std::unordered_set<std::string> existingLines;
    // --- END OF NEW LOGIC ---

    std::ifstream in(basePath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        while (getline(in, line))
        {
            // 2. Check for our old marker
            if (line.find(kMarker) != std::string::npos)
            {
                ignoreLines = true; // Start ignoring all lines after this
                continue;
            }

            // 3. Check for the end of the file marker
            if (line.find("the end") != std::string::npos)
            {
                break; // Stop processing, we will add our own 'the end'
            }

            // 4. If we are in the "ignore" block, skip the line
            if (ignoreLines)
            {
                continue;
            }

            // --- START OF NEW LOGIC ---
            // 5. DUPLICATE CHECK: If the line is NOT a marker, and NOT the end,
            //    check if it's one of the lines we are about to add.
            //    If it is, skip it (continue) to prevent duplicates.
            if (isCommentOrEmpty(line))
            {
                out << line << "\n";
                continue;
            }

            // --- END OF NEW LOGIC ---

            // 6. If none of the above, it's a clean, original line. Write it.
            out << line << "\n";
            existingLines.insert(line);
        }

        // --- Now, we write the new content ---

        out << kMarker << "\n";

        // Write all the lines currently loaded from .comp.injector files
        for (auto& e : store)
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

        out << ";the end\n";

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

void CFLAAudioLoader::Process() {
    // ... (This function is unchanged)
    if (store.empty() && !HasMarker(GAME_PATH((char*)"data/gtasa_vehicleAudioSettings.cfg")))
    {
        Logger.Log(std::string(kLogPrefix) + ": no entries and no marker, skipping.");
        return;
    }

    Logger.Log(std::string(kLogPrefix) + ": processing vehicle audio settings.");
    UpdateAudioFile();
}

void CFLAAudioLoader::AddLine(const std::string &line)
{
    store.push_back(line);
}

void CFLAAudioLoader::Parse(const std::string& line)
{
    // ... (This function is unchanged)
    tVehicleAudioSetting setting;
    int count = sscanf(line.c_str(),
        "%255s %d %d %d %d %f %f %d %f %d %d %d %d %d %f",
        setting.Name,
        &setting.VehAudType,
        &setting.PlayerBank,
        &setting.DummyBank,
        &setting.BassSetting,
        &setting.BassFactor,
        &setting.EnginePitch,
        &setting.HornType,
        &setting.HornPitch,
        &setting.DoorType,
        &setting.EngineUpgrade,
        &setting.RadioStation,
        &setting.RadioType,
        &setting.VehicleAudioTypeForName,
        &setting.EngineVolumeOffset);

    if (count == 15 && strnlen(setting.Name, sizeof(setting.Name)) > 0)
    {
        store.push_back(line);
    }
}
