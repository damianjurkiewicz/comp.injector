## What does COMP.Injector do?

COMP.Injector is an `.asi` plugin that **merges and overrides selected data from configuration files** before any other mods start loading.  
This lets you safely add new objects, vehicles, and settings without manual file merging and without conflict risk.

## Core file types

1. **`.mva` – Model Variations**
   - You can merge: ModelVariations_Peds.ini, ModelVariations_PedWeapons.ini, ModelVariations_Vehicles.ini


2. **`.inj` – INI files**
   - Useful for **preconfiguring mods** (for example, **MixMods** settings).
   - Keeps all configuration in one place and applies it automatically.

3. **`.fla` and original Fastman Limit Adjuster files**
   - Support for ALL **Fastman92 Limit Adjuster**–related files.
   - Allows consistent merging and loading with the rest of your setup.

## Required loader

**COMP.Injector must load first.**  
You should use **COMP.ASI Loader**, which guarantees the correct loading order.

This is critical because COMP.Injector must **override values before other mods are loaded**.

## Why this approach?

Compared to ModLoader:

- This approach was **necessary**, because repeated attempts to integrate ModLoader with Fastman Limit Adjuster failed due to race conditions 
- COMP.Injector **does not work in RAM**. It **modifies data on disk**

**The result:**
- simple implementation,
- very stable behavior,
- no startup conflicts or unpredictable errors.

## Usage examples

### 1) Model Variations (.mva)

1. Place a `ModelVariations_Peds.ini` file in any modloader folder and change the extension to `ModelVariations_Peds.mva` 
2. Start the game. COMP.Injector merges the data before other mods load.

### 2) INI configurations (.inj)

1. Create `mixmods_config.inj`:

```ini
[General]
ExampleSetting=1
AnotherSetting=On
```

2. Place the file in a folder in any modloader folder
3. The configuration is applied automatically on game start.

### 3) Fastman Limit Adjuster (.fla)

1. Place any FLA file from /data in any modloader folder
( gtasa_weapon_config.dat , cheatStrings.dat , gtasa_melee_config.dat, gtasa_radarBlipSpriteFilenames.dat, gtasa_trainTypeCarriages.dat, model_special_features.dat , gtasa_vehicleAudioSettings.cfg)
2. COMP.Injector merges it with the original FLA file
3. Optional: prepare a .fla file like, and put in any modloader folder
`sacgyosemite 0 26 25 0 0.85 1.0 7 1.05946 2 0 13 3 38 0.0
sacgyoscclb 0 26 25 0 0.85 1.0 7 1.05946 2 0 13 3 38 0.0`
ATTENTION: Usage of .fla file works only for `gtasa_vehicleAudioSettings.cfg` and `gtasa_weapon_config.dat` 


