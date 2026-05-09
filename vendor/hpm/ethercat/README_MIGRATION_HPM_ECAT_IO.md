# HPM5E31 EtherCAT (ecat_io) 移植记录

> 更新时间：2026-05-02  
> 工程根目录：`E:/HPM/my_project/HPM5E31_CODE`

## 1. 背景与目标

本次将工程中的 EtherCAT 从“混合/旧 SSC 版本（含 Beckhoff 评估板风格文件）”切换为 **HPM SDK ecat_io 官方生成配置**，并保证在 HPM5E31 上可构建通过。

主要来源：
- SSC 生成源：`E:/HPM/sdk_env_official/hpm_sdk/samples/ethercat/ecat_io/SSC/Config/Src`
- 参考应用：`E:/HPM/sdk_env_official/hpm_sdk/samples/ethercat/ecat_io/application/digital_io.c`
- 补丁参考：`E:/HPM/sdk_env_official/hpm_sdk/samples/ethercat/ecat_io/SSC/ssc_pdi_mask.patch`

---

## 2. 目录层面的变更

### 2.1 当前目录

- `Clight/vendor/ethercat/application/`
  - 放置本地 EtherCAT 应用层文件（`digital_io.c/.h`）

### 2.2 替换目录内容

- `Clight/vendor/ethercat/SSC/Src/`
  - 已按 `ecat_io/SSC/Config/Src` 全量替换同名 SSC 文件
  - 保留本地兼容桥接头：`el9800hw.h`（文件名必须保留，供 SSC include）

### 2.3 备份目录

- `Clight/vendor/ethercat/SSC/backup_20260408_hpm_regen/`
  - 备份替换前的旧 SSC 版本（含旧 `cia402appl.*`）

---

## 3. 文件级改动说明

## 3.1 新增文件

- `Clight/vendor/ethercat/application/digital_io.c`
  - 基于 SDK `ecat_io/application/digital_io.c` 导入
  - 提供 APPL_* 回调和 PDO 映射应用逻辑

- `Clight/vendor/ethercat/application/digital_io.h`
  - 本地补齐（SDK 中缺失）
  - 定义 `ApplicationObjDic`、0x1600/0x1A00/0x1C12/0x1C13 对象及变量声明

## 3.2 修改文件

- `app/CMakeLists.txt`
  - EtherCAT 模式改为从 `Clight/vendor/ethercat` 和 `Clight/vendor/eeprom_emulation` 收集本地 EtherCAT 源和头
  - 将 `digital_io.c/.h` 设为 EtherCAT 必需文件
  - 继续忽略 `SSC/Src/el9800hw.c`（避免与 HPM 端口实现冲突）

- `app/src/main.cpp`
  - 头文件从 `cia402appl.h` 切换为 `digital_io.h`
  - `MainInit()` 后补充 EEPROM 仿真回调绑定：
    - `pAPPL_EEPROM_Read = ecat_eeprom_emulation_read`
    - `pAPPL_EEPROM_Write = ecat_eeprom_emulation_write`
    - `pAPPL_EEPROM_Reload = ecat_eeprom_emulation_reload`
    - `pAPPL_EEPROM_Store = ecat_eeprom_emulation_store`

- `Clight/vendor/ethercat/SSC/Src/ecatslv.c`
  - 应用 `ssc_pdi_mask.patch` 同等修正（两处）：
    - `ResetALEventMask()` 中屏蔽 `SYNC0_EVENT | SYNC1_EVENT`
    - `SetALEventMask()` 中屏蔽 `SYNC0_EVENT | SYNC1_EVENT`
  - 目的：避免 Sync0/Sync1 事件进入 PDI IRQ（已有独立 Sync IRQ）

- `.vscode/settings.json`
  - 将 IntelliSense/clangd 的 `compile_commands` 路径切到 `build_ethercat`
  - CMake 默认 preset 切到 EtherCAT：
    - `hpm5e31-ethercat`
    - `hpm5e31-ethercat-build`
  - 解决“`#if APP_ENABLE_ETHERCAT_IO` 不高亮、不跳转”问题

## 3.3 替换后的 SSC 文件（当前生效）

位于 `Clight/vendor/ethercat/SSC/Src/`：
- `applInterface.h`
- `coeappl.c/.h`
- `ecatappl.c/.h`
- `ecatcoe.c/.h`
- `ecatslv.c/.h`
- `ecat_def.h`
- `eeprom.h`
- `esc.h`
- `mailbox.c/.h`
- `objdef.c/.h`
- `sdoserv.c/.h`
- `el9800hw.h`（本地兼容桥接头）

---

## 4. 构建配置与结果

### 4.1 构建目录

- EtherCAT 构建目录：`build_ethercat`

### 4.2 关键宏（来自 `build_ethercat`）

- `APP_ENABLE_ETHERCAT_IO=1`
- `APP_ENABLE_LWIP_TCPECHO=0`
- `ESC_EEPROM_EMULATION=1`
- `USE_DEFAULT_MAIN=0`

### 4.3 构建结果

- 已成功链接：`build_ethercat/output/demo.elf`
- 产物包含 EtherCAT 关键符号：
  - `main`
  - `MainInit`
  - `ecat_hardware_init`
  - `APPL_GenerateMapping`

---

## 5. 运行与维护注意事项

1. 烧录请使用 `build_ethercat/output/demo.elf`（或同目录 `demo.bin`），避免误烧录 `build/` 下 lwIP 产物。  
2. 若重新用 SSC Tool 生成代码，需再次执行：
   - 覆盖 `Clight/vendor/ethercat/SSC/Src`（保留 `el9800hw.h`）
   - 保留本地 `Clight/vendor/ethercat/application/digital_io.h`（SDK 无该头）
   - 复核 `ecatslv.c` 两处 Sync 事件 mask 补丁
3. 当前 `coeappl.c` 的 `ApplicationObjDic != NULL` 为上游代码风格警告，可忽略，不影响功能。

---

## 6. 快速自检清单

- [ ] `build_ethercat/CMakeCache.txt` 中 `APP_ENABLE_ETHERCAT_IO:BOOL=ON`  
- [ ] `build_ethercat/compile_commands.json` 中 `main.cpp` 含 `-DAPP_ENABLE_ETHERCAT_IO=1`  
- [ ] `app/src/main.cpp` 包含 `digital_io.h` 且绑定 EEPROM 回调  
- [ ] `Clight/vendor/ethercat/SSC/Src/ecatslv.c` 两处存在 `u32Mask &= ~(SYNC0_EVENT | SYNC1_EVENT);`  
- [ ] 最终链接产物 `build_ethercat/output/demo.elf` 更新时间为最新
