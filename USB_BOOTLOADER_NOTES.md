# USB Direct Download Notes

This project targets `STM32F103CBT6`.

## Important limitation

The STM32F103 system ROM bootloader does not provide native USB DFU download support on this device, so this application alone cannot be programmed directly over USB by entering the factory boot mode.

To support "USB direct download", the board needs:

1. A dedicated USB bootloader stored at the beginning of Flash.
2. The application linked to a non-zero Flash offset.
3. A jump from the bootloader to the application after validation.

## Flash layout used by this project

- Bootloader region: `0x08000000` - `0x08003FFF` (16 KB)
- Application region: `0x08004000` - `0x0800FFFF` (48 KB)

The current project has been updated so the application is linked at `0x08004000`, and its vector table is relocated to the same offset.

## What the USB bootloader still needs to do

1. Initialize USB on `PA11/PA12`.
2. Expose a DFU or custom USB upgrade protocol to the PC.
3. Erase and program the application area starting at `0x08004000`.
4. Validate the application image.
5. Load the application's MSP from `0x08004000`.
6. Jump to the application's reset handler from `0x08004004`.

## PC side

If the bootloader implements DFU class, you can use tools such as STM32CubeProgrammer or `dfu-util`.

## Current status

This repository now contains the application-side changes required for a future USB bootloader, but it does not yet include the bootloader project itself.
