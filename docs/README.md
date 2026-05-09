# Clight Docs

These documents describe the public Clight framework. They are not board bring-up logs and they do not assume a private STM32CubeMX, HPM SDK, or ESP-IDF workspace is committed into this repository.

Read in this order:

1. `01_architecture.md` - framework layers and ownership.
2. `02_platform_workspace.md` - how a real product project should include Clight.
3. `03_generated_files.md` - where HPM PinMux, STM32CubeMX, and ESP-IDF generated files belong.
4. `04_porting_checklist.md` - what to change when adding a new board or chip.
5. `05_release_policy.md` - what is allowed in the public repository.

The short rule is: Clight owns the API, BSP interfaces, reusable modules, port code, and explicitly maintained vendor-origin source in `vendor/`. Full SDK mirrors, generated board packages, build outputs, credentials, and machine-local files stay outside the public framework repository.
