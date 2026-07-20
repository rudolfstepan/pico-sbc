# LAFVIN Pico SBC Firmware

Open-Source-Firmware fuer das LAFVIN Pico Development Kit mit RP2040,
ST7796U-LCD und GT911-Touchcontroller. Das Repository enthaelt zwei direkt
flashbare Anwendungen und eine gemeinsame Hardwarebibliothek.

## Aktueller Stand

| Komponente | Version/Stand |
|---|---|
| Scientific Calculator | Firmware `1.8.0` |
| USB-CDC-Protokoll | Version `4` |
| Persistenter Rechnerzustand | Flashformat `6`, ohne Altformat-Migration |
| Pico Calculator Link | Version `2.1` |
| Mini Computer | Firmware `1.0.0` |
| Automatisierte Host-Tests | 30 Tests |

## Firmware

| Anwendung | Beschreibung | Ziel |
|---|---|---|
| [Mini Computer](firmware/mini-computer/README.md) | Touch-Tastatur und kleine Kommandozeile | `lafvin_minicomputer` |
| [Scientific Calculator](firmware/scientific-calculator/README.md) | Wissenschaftlicher Rechner mit waehlbarer 40-, 80- oder 128-stelliger Arithmetik, Programmer-, Graph-, Logik-, Statistik- und BASIC-Modus | `lafvin_scientific_calculator` |

## Projektstruktur

```text
firmware/             Anwendungen mit eigener UI und Programmlogik
examples/basic/       Beispiel- und Testprogramme fuer den BASIC-Modus
lib/board/            Gemeinsame Board-, LCD- und Touchtreiber
tests/calculator/     Hardwareunabhaengige Unit-Tests
tests/tools/          Tests fuer die PC-Werkzeuge
tools/                USB-Kommandozeile und grafische Synchronisations-App
docs/                 Hardware- und Architekturunterlagen
third_party/tinyexpr/ TinyExpr als gepinntes Git-Submodul
third_party/libbf/    LibBF fuer hochpraezise wissenschaftliche Funktionen
```

## Voraussetzungen

- Raspberry Pi Pico SDK
- CMake 3.16 oder neuer
- Arm GNU Toolchain (`arm-none-eabi-gcc`)
- Ninja oder ein anderer von CMake unterstuetzter Generator
- Ein nativer C-Compiler fuer die Host-Tests
- Optional Python 3.9 und PySerial fuer den USB-Datenaustausch

## Pico Calculator Link

Die plattformunabhaengige Desktop-App `Pico Calculator Link 2.1` verbindet den
Scientific Calculator mit Windows, Linux oder macOS. Sie bildet alle
Rechner-Module ab: Wissenschaft, Programmer und Zahlenformate, Graphanalyse,
Logik, Einheiten und Konstanten, komplexe Zahlen, Statistik,
Speicher/Favoriten und BASIC. Persistente Daten werden synchronisiert und als
JSON gesichert.

![Pico Calculator Link](docs/images/pico-calculator-link.png)

```sh
python -m pip install -r tools/requirements.txt
python tools/pico_calc_gui.py
```

Unter Linux muss gegebenenfalls das Tk-Paket der Distribution, zum Beispiel
`python3-tk`, installiert werden. Das bestehende Kommandozeilenwerkzeug bleibt
unter `tools/pico_calc_cli.py` verfuegbar.

## Repository klonen

```sh
git clone --recurse-submodules https://github.com/rudolfstepan/pico-sbc.git
cd pico-sbc
```

Bei einem bereits vorhandenen Clone wird das TinyExpr-Submodul so geladen:

```sh
git submodule update --init --recursive
```

## Firmware bauen

`PICO_SDK_PATH` muss auf eine lokale Installation des Pico SDK zeigen.

```powershell
$env:PICO_SDK_PATH="C:\path\to\pico-sdk"
cmake -S . -B out/firmware -G Ninja
cmake --build out/firmware
```

Die UF2-Dateien werden unter `out/firmware/bin/` erzeugt. Einzelne Anwendungen
lassen sich mit `--target lafvin_minicomputer` beziehungsweise
`--target lafvin_scientific_calculator` bauen.

## Tests

Die Rechenkerne und PC-Werkzeuge sind vom Pico SDK entkoppelt und werden in
aktuell 30 Host-Tests geprueft:

```sh
cmake -S tests -B out/tests
cmake --build out/tests
ctest --test-dir out/tests --output-on-failure
```

## Hardware

Pinbelegung und technische Daten stehen in [docs/hardware.md](docs/hardware.md).
Die vollstaendige Bedienung des Scientific Calculator beschreibt das
[Benutzerhandbuch](docs/user-manual.md).
Die Architektur der Firmware ist in
[docs/architecture.md](docs/architecture.md) beschrieben.
Das Befehlsformat und das PC-Werkzeug sind in
[docs/usb-protocol.md](docs/usb-protocol.md) dokumentiert.

## Roadmap

Die geplanten Rechner-Erweiterungen und ihre Umsetzungsreihenfolge stehen in
[docs/roadmap.md](docs/roadmap.md).

## Lizenz

Der eigene Projektcode steht unter der [MIT-Lizenz](LICENSE). TinyExpr ist ein
separates Projekt unter der Zlib-Lizenz. Das eingebettete LibBF steht ebenfalls
unter der MIT-Lizenz. Die jeweiligen Lizenztexte liegen in den
`third_party`-Verzeichnissen.
