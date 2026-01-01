#include "pch.h"
#include "audio.h"
#include "tVehicleAudioSetting.h"
#include <unordered_set> // <<< ADD THIS INCLUDE FOR THE DUPLICATE CHECK

CFLAAudioLoader FLAAudioLoader;

// <<< MODIFIED FUNCTION TO REBUILD AND REMOVE DUPLICATES >>>
void CFLAAudioLoader::UpdateAudioFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/gtasa_vehicleAudioSettings.cfg");
    std::string settingsPathTemp = settingsPath + ".bak"; // This is just a temporary file

    if (!std::filesystem::exists(settingsPath))
    {
        return;
    }

    // --- START OF NEW LOGIC ---
    // 1. Create a "cache" (a hash set) of all lines we intend to add.
    //    This is very fast for lookups.
    std::unordered_set<std::string> linesToAdd(store.begin(), store.end());
    // --- END OF NEW LOGIC ---

    std::ifstream in(settingsPath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        const std::string marker = "; fastloader added vehicles";

        while (getline(in, line))
        {
            // 2. Check for our old marker
            if (line.find(marker) != std::string::npos)
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
            if (linesToAdd.count(line))
            {
                continue; // This is an old line from our mod, skip it
            }
            // --- END OF NEW LOGIC ---

            // 6. If none of the above, it's a clean, original line. Write it.
            out << line << "\n";
        }

        // --- Now, we write the new content ---

        out << marker << "\n";

        // Write all the lines currently loaded from .fastloader files
        for (auto& e : store)
        {
            out << e << "\n";
        }

        out << ";the end\n";

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

void CFLAAudioLoader::Process() {
    // ... (This function is unchanged)
    if (!store.empty()) {
        UpdateAudioFile();
    }
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