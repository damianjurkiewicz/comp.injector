#include "pch.h"
#include "loader_core.h"
#include "audio.h"
#include "weapon_config.h"
#include "model_special_features.h"
#include "train_type_carriages.h"
#include "radar_blip_sprite_filenames.h"
#include "melee_config.h"
#include "cheat_strings.h"
#include "tracks_config.h"
#include "inj_config.h"
#include "mva_loader.h"
#include "logger.h"


CompInjector::CompInjector(HINSTANCE pluginHandle)
{

    handle = pluginHandle;

    HandleVanillaDataFiles();
    ParseModloader();
    {
        char modulePath[MAX_PATH] = {};
        std::filesystem::path pluginDir;
        if (GetModuleFileNameA(handle, modulePath, MAX_PATH) != 0)
        {
            pluginDir = std::filesystem::path(modulePath).parent_path();
        }

        if (!pluginDir.empty())
        {
            Logger.Init(pluginDir / "comp.injector.log");
        }

        InjConfigLoader.Process(pluginDir);
    }

    MvaLoader.Process();

    if (gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1) == 1)
    {
        FLAAudioLoader.Process();
    }

    if (gConfig.ReadInteger("MAIN", "FLAWeaponConfigLoader", 1) == 1)
    {
        FLAWeaponConfigLoader.Process();
    }

    if (gConfig.ReadInteger("MAIN", "FLAModelSpecialFeaturesLoader", 1) == 1)
    {
        FLAModelSpecialFeaturesLoader.Process();
    }

    if (gConfig.ReadInteger("MAIN", "FLATrainTypeCarriagesLoader", 1) == 1)
    {
        FLATrainTypeCarriagesLoader.Process();
    }

    if (gConfig.ReadInteger("MAIN", "FLARadarBlipSpriteFilenamesLoader", 1) == 1)
    {
        FLARadarBlipSpriteFilenamesLoader.Process();
    }

    if (gConfig.ReadInteger("MAIN", "FLAMeleeConfigLoader", 1) == 1)
    {
        FLAMeleeConfigLoader.Process();
    }

    if (gConfig.ReadInteger("MAIN", "FLACheatStringsLoader", 1) == 1)
    {
        FLACheatStringsLoader.Process();
    }

    if (gConfig.ReadInteger("MAIN", "FLATracksConfigLoader", 1) == 1)
    {
        FLATracksConfigLoader.Process();
    }
}


void CompInjector::HandleVanillaDataFiles()
{
    // Lambda function to handle one-time .back backups (DRY principle)
    auto checkAndBackup = [&](const char* iniKey, const char* relativePath)
        {
            // 1. Check if the option is enabled in the INI first.
            // Jeśli opcja jest wyłączona (nie równa się 1), przerywamy natychmiast.
            if (gConfig.ReadInteger("MAIN", iniKey, 0) != 1)
            {
                return;
            }

            std::string fullPath = GAME_PATH((char*)relativePath);
            std::string backupPath = fullPath + ".back";

            // 2. Check if the target file exists AND the backup does NOT exist yet.
            // Logika: Pytamy o backup tylko wtedy, gdy plik istnieje i nie zrobiliśmy jeszcze jego kopii.
            if (std::filesystem::exists(fullPath) && !std::filesystem::exists(backupPath))
            {
                try
                {
                    std::filesystem::copy_file(fullPath, backupPath, std::filesystem::copy_options::overwrite_existing);
                }
                catch (const std::exception&)
                {
                }
            }
        };

    // --- Process Backups ---

    // 1. Audio
    checkAndBackup("FLAAudioLoader", "data/gtasa_vehicleAudioSettings.cfg");

    // 2. Weapon Config
    checkAndBackup("FLAWeaponConfigLoader", "data/gtasa_weapon_config.dat");

    // 3. Model Special Features
    checkAndBackup("FLAModelSpecialFeaturesLoader", "data/model_special_features.dat");

    // 4. Train Type Carriages
    checkAndBackup("FLATrainTypeCarriagesLoader", "data/gtasa_trainTypeCarriages.dat");

    // 5. Radar Blip Sprites
    checkAndBackup("FLARadarBlipSpriteFilenamesLoader", "data/gtasa_radarBlipSpriteFilenames.dat");

    // 6. Melee Config (Standard vanilla file usually)
    checkAndBackup("FLAMeleeConfigLoader", "data/gtasa_melee_config.dat");

    // 7. Cheat Strings
    checkAndBackup("FLACheatStringsLoader", "data/cheatStrings.dat");

    // 8. Tracks Config (Standard vanilla file usually)
    checkAndBackup("FLATracksConfigLoader", "data/Paths/gtasa_tracks_config.dat");


    // --- Original Modloader Traversal Logic ---
    std::function<void(const std::filesystem::path&)> traverse;
    traverse = [&](const std::filesystem::path& dir)
        {
            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    if (!folderName.empty() && folderName[0] == '.')
                    {
                        continue;
                    }
                    traverse(entry.path());
                    continue;
                }
                if (!entry.is_regular_file())
                {
                    continue;
                }
                // Dalsza logika traversal, jeśli potrzebna
            }
        };

    traverse(GAME_PATH((char*)"modloader"));
}




void CompInjector::ParseModloader()
{
    auto isCommentOrEmpty = [](const std::string &line)
        {
            const auto firstNonWhitespace = line.find_first_not_of(" \t\r\n");
            if (firstNonWhitespace == std::string::npos)
            {
                return true;
            }

            const std::string_view trimmed(line.c_str() + firstNonWhitespace, line.size() - firstNonWhitespace);

            return trimmed.starts_with(";") || trimmed.starts_with("#") || trimmed.starts_with("//");
        };

    std::function<void(const std::filesystem::path&)> traverse;
    traverse = [&](const std::filesystem::path& dir)
        {
            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    if (!folderName.empty() && folderName[0] == '.')
                    {
                        continue;
                    }
                    traverse(entry.path());
                    continue;
                }
                if (!entry.is_regular_file())
                {
                    continue;
                }
                std::string ext = entry.path().extension().string();
                std::string path = entry.path().string();
                std::string filename = entry.path().filename().string();

                if (ext == ".fla")
                {
                    std::ifstream in(path);
                    std::string line;
                    while (getline(in, line))
                    {
                        if (line.starts_with(";") || line.starts_with("//") || line.starts_with("#"))
                        {
                            continue;
                        }
                        if (gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1) == 1)
                        {
                            FLAAudioLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "FLAWeaponConfigLoader", 1) == 1)
                        {
                            FLAWeaponConfigLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "FLAModelSpecialFeaturesLoader", 1) == 1)
                        {
                            FLAModelSpecialFeaturesLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "FLATrainTypeCarriagesLoader", 1) == 1)
                        {
                            FLATrainTypeCarriagesLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "FLARadarBlipSpriteFilenamesLoader", 1) == 1)
                        {
                            FLARadarBlipSpriteFilenamesLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "FLAMeleeConfigLoader", 1) == 1)
                        {
                            FLAMeleeConfigLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "FLACheatStringsLoader", 1) == 1)
                        {
                            FLACheatStringsLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "FLATracksConfigLoader", 1) == 1)
                        {
                            FLATracksConfigLoader.Parse(line);
                        }
                    }
                    in.close();
                }
                else if (ext == ".dat" || ext == ".cfg")
                {
                    std::ifstream in(path);
                    if (!in.is_open())
                    {
                        continue;
                    }

                    std::string line;
                    while (getline(in, line))
                    {
                        if (isCommentOrEmpty(line))
                        {
                            continue;
                        }

                        if (filename == "gtasa_trainTypeCarriages.dat")
                        {
                            if (gConfig.ReadInteger("MAIN", "FLATrainTypeCarriagesLoader", 1) == 1)
                            {
                                FLATrainTypeCarriagesLoader.AddLine(line);
                            }
                        }
                        else if (filename == "model_special_features.dat")
                        {
                            if (gConfig.ReadInteger("MAIN", "FLAModelSpecialFeaturesLoader", 1) == 1)
                            {
                                FLAModelSpecialFeaturesLoader.AddLine(line);
                            }
                        }
                        else if (filename == "gtasa_melee_config.dat")
                        {
                            if (gConfig.ReadInteger("MAIN", "FLAMeleeConfigLoader", 1) == 1)
                            {
                                FLAMeleeConfigLoader.AddLine(line);
                            }
                        }
                        else if (filename == "gtasa_radarBlipSpriteFilenames.dat")
                        {
                            if (gConfig.ReadInteger("MAIN", "FLARadarBlipSpriteFilenamesLoader", 1) == 1)
                            {
                                FLARadarBlipSpriteFilenamesLoader.AddLine(line);
                            }
                        }
                        else if (filename == "gtasa_tracks_config.dat")
                        {
                            if (gConfig.ReadInteger("MAIN", "FLATracksConfigLoader", 1) == 1)
                            {
                                FLATracksConfigLoader.AddLine(line);
                            }
                        }
                        else if (filename == "cheatStrings.dat")
                        {
                            if (gConfig.ReadInteger("MAIN", "FLACheatStringsLoader", 1) == 1)
                            {
                                FLACheatStringsLoader.AddLine(line);
                            }
                        }
                        else if (filename == "gtasa_vehicleAudioSettings.cfg")
                        {
                            if (gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1) == 1)
                            {
                                FLAAudioLoader.AddLine(line);
                            }
                        }
                        else if (filename == "gtasa_weapon_config.dat")
                        {
                            if (gConfig.ReadInteger("MAIN", "FLAWeaponConfigLoader", 1) == 1)
                            {
                                FLAWeaponConfigLoader.AddLine(line);
                            }
                        }
                    }
                    in.close();
                }
            }
        };

    traverse(GAME_PATH((char*)"modloader"));
}
