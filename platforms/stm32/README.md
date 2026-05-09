# STM32 Platform Template

The public repository does not include STM32CubeMX generated `Core/Drivers`, startup files, or linker scripts.

To build on STM32:

1. Generate the CubeMX project locally.
2. Put generated files in `platforms/stm32/board/<board_name>/` or in your product repository.
3. Connect that board package to Clight's `bsp/interface`, `bsp/api`, and `bsp/port/stm32`.
4. Keep CubeMX output out of the public framework repository unless you intentionally publish a separate board package.

The `main` folder is only an empty entry template.
