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
#include <unordered_set>


CompInjector::CompInjector(HINSTANCE pluginHandle)
{

    handle = pluginHandle;

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

    FLAAudioLoader.Process();
    FLAWeaponConfigLoader.Process();
    FLAModelSpecialFeaturesLoader.Process();
    FLATrainTypeCarriagesLoader.Process();
    FLARadarBlipSpriteFilenamesLoader.Process();
    FLAMeleeConfigLoader.Process();
    FLACheatStringsLoader.Process();
    FLATracksConfigLoader.Process();
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

    std::unordered_set<std::string> modloaderFiles;
    {
        std::filesystem::path modloaderRoot = GAME_PATH((char*)"modloader");
        if (std::filesystem::exists(modloaderRoot))
        {
            std::filesystem::directory_options options = std::filesystem::directory_options::skip_permission_denied;
            for (auto it = std::filesystem::recursive_directory_iterator(modloaderRoot, options);
                it != std::filesystem::recursive_directory_iterator();
                ++it)
            {
                if (it->is_directory())
                {
                    std::string folderName = it->path().filename().string();
                    if (!folderName.empty() && folderName[0] == '.')
                    {
                        it.disable_recursion_pending();
                    }
                    continue;
                }

                if (!it->is_regular_file())
                {
                    continue;
                }

                modloaderFiles.insert(it->path().filename().string());
            }
        }
    }

    const bool hasVehicleAudio = modloaderFiles.count("gtasa_vehicleAudioSettings.cfg") > 0;
    const bool hasWeaponConfig = modloaderFiles.count("gtasa_weapon_config.dat") > 0;
    const bool hasModelSpecialFeatures = modloaderFiles.count("model_special_features.dat") > 0;
    const bool hasTrainTypeCarriages = modloaderFiles.count("gtasa_trainTypeCarriages.dat") > 0;
    const bool hasMeleeConfig = modloaderFiles.count("gtasa_melee_config.dat") > 0;
    const bool hasCheatStrings = modloaderFiles.count("cheatStrings.dat") > 0;
    const bool hasRadarBlipSprites = modloaderFiles.count("gtasa_radarBlipSpriteFilenames.dat") > 0;
    const bool hasTracksConfig = modloaderFiles.count("gtasa_tracks_config.dat") > 0;

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
                        if (hasVehicleAudio)
                        {
                            FLAAudioLoader.Parse(line);
                        }
                        if (hasWeaponConfig)
                        {
                            FLAWeaponConfigLoader.Parse(line);
                        }
                        if (hasModelSpecialFeatures)
                        {
                            FLAModelSpecialFeaturesLoader.Parse(line);
                        }
                        if (hasTrainTypeCarriages)
                        {
                            FLATrainTypeCarriagesLoader.Parse(line);
                        }
                        if (hasRadarBlipSprites)
                        {
                            FLARadarBlipSpriteFilenamesLoader.Parse(line);
                        }
                        if (hasMeleeConfig)
                        {
                            FLAMeleeConfigLoader.Parse(line);
                        }
                        if (hasCheatStrings)
                        {
                            FLACheatStringsLoader.Parse(line);
                        }
                        if (hasTracksConfig)
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
