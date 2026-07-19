# Mini Computer Firmware

Firmware-Basis fuer das im Markdown beschriebene LAFVIN Pico Development Kit.

## Funktionen

- ST7796U Display ueber SPI0, 480 x 320 Landscape
- GT911 Touch ueber I2C0
- Touch-Tastatur auf dem Display
- Kleine Kommandozeile mit `HELP`, `INFO`, `CLS`, `BEEP`, `LED1 ON/OFF`, `LED2 ON/OFF`
- K1 sendet Enter, K2 loescht die Eingabe
- Buzzer-Feedback und Status-LEDs
- Kein Vollbild-Framebuffer, damit die Firmware in den RP2040-SRAM passt

## Build

Voraussetzungen:

- Raspberry Pi Pico SDK
- CMake
- Arm GNU Toolchain (`arm-none-eabi-gcc`)
- Ninja
- Windows-Host-Compiler fuer `picotool`, z. B. WinLibs/MinGW

Vom Repository-Stammverzeichnis:

```powershell
$env:PICO_SDK_PATH="C:\path\to\pico-sdk"
cmake -S . -B out/firmware -G Ninja
cmake --build out/firmware --target lafvin_minicomputer
```

Nach einer frischen Tool-Installation muss eventuell ein neues Terminal geoeffnet werden, damit `cmake`, `ninja`, `gcc` und `arm-none-eabi-gcc` im `PATH` sichtbar sind.

Falls das aktuelle Terminal den neuen `PATH` noch nicht kennt, funktioniert dieser explizite Build-Aufruf:

```powershell
$env:PICO_SDK_PATH="C:\path\to\pico-sdk"
$env:CC="$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\gcc.exe"
$env:CXX="$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe"
$env:Path="C:\Program Files\CMake\bin;$env:LOCALAPPDATA\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Linux\gcc_arm\bin;$env:Path"
& "C:\Program Files\CMake\bin\cmake.exe" -S . -B out/firmware -G Ninja
& "C:\Program Files\CMake\bin\cmake.exe" --build out/firmware --target lafvin_minicomputer
```

Die erzeugte Datei `out/firmware/bin/lafvin_minicomputer.uf2` kann bei
gedrueckter BOOTSEL-Taste auf den Pico kopiert werden.

## Pinbelegung

Die Pinbelegung ist aus [docs/hardware.md](../../docs/hardware.md) uebernommen
und in `lib/board/include/board.h` zentral definiert.
