# Release Policy

The public repository should contain framework code, not local generated projects.

## Allowed

- `bsp/interface`
- `bsp/api`
- `bsp/port`
- `bsp/chip` capability headers without secrets
- reusable `modules`
- code generation tools
- platform templates
- documentation
- examples that do not require private SDK files to be committed

## Not Allowed

- `build/`, `out/`, `Debug/`, `Release/`
- firmware images, map files, object files, and tool caches
- `vendor/` SDK mirrors or local protocol-stack drops
- STM32CubeMX generated `Core/Drivers` in the core framework repo
- HPM generated `.hpmpc` files
- ESP-IDF `sdkconfig` containing local settings or credentials
- Wi-Fi credentials, tokens, certificates, private keys, passwords, or secret keys

## Pre-Push Checks

Run these before publishing:

```powershell
git status --short
git ls-files
git grep -n -i "password\|secret\|token\|ssid\|private key"
```

If a file is needed to build only on one machine, it probably does not belong in the public framework repository.
