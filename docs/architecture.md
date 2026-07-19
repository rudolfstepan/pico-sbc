# Firmware-Architektur

## Anwendungen

`firmware/mini-computer` und `firmware/scientific-calculator` sind getrennte
RP2040-Programme. Sie besitzen jeweils einen eigenen Einstiegspunkt und eigene
UI-Logik, teilen jedoch keine globalen Anwendungszustaende.

## Boardbibliothek

`lib/board` kapselt die Pinbelegung und alle hardwarenahen Komponenten:

- `board`: Taster, Joystick, LEDs und Buzzer
- `lcd_st7796`: Displayinitialisierung, Zeichenfont und Zeichenprimitive
- `touch_gt911`: Touchinitialisierung, Koordinaten und Freigabeereignisse

Die Bibliothek stellt ihre Header als oeffentliche CMake-Includes bereit. Neue
Firmware-Ziele sollen gegen `lafvin_board` linken, statt Treiberquellen zu
duplizieren.

## Wissenschaftlicher Rechner

Der Rechner trennt Darstellung und Rechenlogik:

- `calculator_ui`: Seiten, Touchbedienung, Editor und Graphdarstellung
- `calculator_engine`: mathematische Ausdruecke und TinyExpr-Anbindung
- `programmer_engine`: 64-Bit-Zahlenbasen und Bitoperationen
- `number_formats`: Zweierkomplement, Festkomma und IEEE-754

Die drei Rechenmodule sind hardwareunabhaengig und werden unter `tests/`
direkt auf dem Host getestet.
