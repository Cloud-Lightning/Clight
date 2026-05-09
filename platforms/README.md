# Platform Templates

This directory contains minimal platform entry templates.

It does not contain complete generated vendor projects. Add those locally when building a real board:

- HPM generated board packages go under `platforms/hpm/boards/<board_name>/`.
- STM32CubeMX output can go under `platforms/stm32/board/<board_name>/` or in a product repository.
- ESP-IDF local `sdkconfig` and `build/` stay local.

See `docs/03_generated_files.md`.
