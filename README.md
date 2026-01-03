# COMP.Injector – Overview and Usage Examples

Below is a concise explanation of how **COMP.Injector** works, along with practical usage examples. This file is designed for quick setup and to avoid conflicts with other mods.

## What does COMP.Injector do?

COMP.Injector is an `.asi` plugin that **merges and overrides selected data from configuration files** before any other mods start loading.  
This lets you safely add new objects, vehicles, and settings without manual file merging and without conflict risk.

## Core file types

1. **`.mva` – Model Variations**
   - Files responsible for model variants (vehicles, objects, etc.).
   - Use them to add new variants without manual edits.

2. **`.inj` – arbitrary INI files**
   - Useful for **preconfiguring mods** (for example, **MixMods** settings).
   - Keeps all configuration in one place and applies it automatically.

3. **`.fla` and original Fastman Limit Adjuster files**
   - Support for **Fastman92 Limit Adjuster**–related files.
   - Allows consistent merging and loading with the rest of your setup.

## Required loader

**COMP.Injector must load first.**  
You must use **COMP.ASI Loader**, which guarantees the correct loading order.

This is critical because COMP.Injector must **override values before other mods are loaded**.

## Why this approach?

Compared to ModLoader:

- COMP.Injector **does not work in RAM**. It **modifies data on disk**.
- This means the system needs a moment to update files.
- This approach was **necessary**, because repeated attempts to integrate ModLoader with Fastman Limit Adjuster failed due to race conditions.

**The result:**
- simple implementation,
- very stable behavior,
- no startup conflicts or unpredictable errors.

## Usage examples

### 1) Model Variations (.mva)

1. Place `example.mva` in the COMP.Injector directory.
2. Start the game. COMP.Injector merges the data before other mods load.

### 2) INI configurations (.inj)

1. Create `mixmods_config.inj`:

```ini
[General]
ExampleSetting=1
AnotherSetting=On
```

2. Place the file in a folder handled by COMP.Injector.
3. The configuration is applied automatically on game start.

### 3) Fastman Limit Adjuster (.fla)

1. Place `fla_config.fla` in the correct location.
2. COMP.Injector merges it with the original FLA files.
3. The values are applied before other mods start.

## Summary

COMP.Injector is a reliable tool for data merging and safe mod startup.  
By supporting `.mva`, `.inj`, and `.fla` files and enforcing early loading via **COMP.ASI Loader**, it provides stable and predictable behavior even in complex mod setups.
