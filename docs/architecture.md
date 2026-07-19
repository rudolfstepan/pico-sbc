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

- `calculator_ui`: Anwendungszustand sowie Touch-, Tasten- und Joystickereignisse
- `calculator_ui_types` und `calculator_keymaps`: gemeinsame Seiten-, Aktions-
  und Tastenbeschreibungen
- `calculator_navigation`: hardwareunabhaengige Seitennavigation
- `calculator_pages` und `calculator_widgets`: LCD-Seiten und gemeinsame
  Tasten-/Treffergeometrie
- `graph_model`: drei Funktionsplaetze, Viewport, Trace, Tabelle,
  Auto-Skalierung sowie Nullstellen- und Schnittpunktsuche
- `calculator_graph`: TinyExpr-Anbindung und LCD-Darstellung des Graphmodells
- `expression_editor`, `calculator_list` und `calculator_dialog`:
  hardwareunabhaengige Eingabe-, Listen- und Dialogzustaende
- `calculation_status`: gemeinsame Fehlercodes und lesbare Meldungen
- `calculator_engine`: mathematische Ausdruecke und TinyExpr-Anbindung
- `programmer_engine`: 64-Bit-Zahlenbasen und Bitoperationen
- `number_formats`: Zweierkomplement, Festkomma und IEEE-754

Alle Module ohne LCD-, Touch- oder Boardabhaengigkeit werden unter `tests/`
direkt auf dem Host mit aktivierten Compilerwarnungen getestet. Dadurch kann
neue Rechenlogik entwickelt werden, ohne einen Pico oder ein Display fuer den
Testlauf zu benoetigen.
