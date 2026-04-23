# FOC USB DFU Bootloader

This folder contains a standalone bootloader project for `STM32F103CBT6`.

## Flash layout

- Bootloader: `0x08000000` - `0x08003FFF` (`16 KB`)
- Application: `0x08004000` - `0x0800FFFF` (`48 KB`)

The main FOC application in the repository has already been moved to `0x08004000`.

## What this bootloader does

- Enumerates on `PA11/PA12` as a USB DFU-compatible device.
- Implements the control-endpoint DFU request flow needed for host-side download.
- Supports DfuSe-style `SET_ADDRESS_POINTER` and `ERASE` commands.
- Programs the application area starting at `0x08004000`.
- Jumps to the application automatically if a valid app exists and no DFU session starts within about `1.5 s`.

## Build

Configure this directory as a separate CMake project and point the linker script to `Bootloader/STM32F103XX_BOOT.ld`.

Example:

```powershell
cmake -S Bootloader -B build/Bootloader -G Ninja `
  -DCMAKE_MAKE_PROGRAM="D:/SoftWare/CLion 2025.2.4/bin/ninja/win/x64/ninja.exe" `
  -DCMAKE_TOOLCHAIN_FILE="D:/Prj/Clion_prj/FOC/cmake/gcc-arm-none-eabi.cmake"
cmake --build build/Bootloader
```

## Upload flow

1. Flash the bootloader once with SWD/J-Link.
2. Reset the board with USB connected to the PC.
3. The board should enumerate as a DFU bootloader.
4. Download the application image to `0x08004000`.

## Notes

- This implementation is intentionally compact and avoids pulling in the full STM32 USB middleware stack.
- Because it is a custom in-repo bootloader, test it on hardware before relying on it for production updates.
