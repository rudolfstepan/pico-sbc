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
- `calculator_symbols`: Variablen A-F, Benutzerfunktionen F1-F3,
  Favoritenbelegung und Rekursionserkennung
- `calculator_persistence`: versionierte, explizite Serialisierung des
  Rechnerzustands mit CRC32 und Auswahl redundanter Datensaetze
- `calculator_storage`: RP2040-Flash-Backend mit zwei wechselnden Sektoren,
  Schreibvermeidung fuer unveraenderte Daten und Ruecklesepruefung
- `basic_engine`: hardwareunabhaengiger, schrittweise ausgefuehrter
  BASIC-Interpreter mit festen Speicher- und Laufzeitgrenzen
- `calculator_program`: CODE-Seite mit Programmliste, Ausgabe sowie QWERTZ-
  und BASIC-Token-Tastatur
- `numerical_analysis`: hardwareunabhaengige Nullstellen-, Schnittpunkt-,
  Ableitungs-, Integral- und Extremumsalgorithmen mit Toleranzgrenzen
- `expression_editor`, `calculator_list` und `calculator_dialog`:
  hardwareunabhaengige Eingabe-, Listen- und Dialogzustaende
- `calculation_status`: gemeinsame Fehlercodes und lesbare Meldungen
- `calculator_engine`: mathematische Ausdruecke, Symbolauflosung und
  TinyExpr-Anbindung mit geschuetzten Benutzerfunktionsaufrufen
- `programmer_engine`: wortbreitenabhaengige Zahlenbasen, Einzelbitoperationen,
  Rotationen sowie Carry- und Overflowstatus
- `number_formats`: Zweierkomplement, Festkomma, Byte-Reihenfolge und
  IEEE-754-Zerlegung

Alle Module ohne LCD-, Touch- oder Boardabhaengigkeit werden unter `tests/`
direkt auf dem Host mit aktivierten Compilerwarnungen getestet. Dadurch kann
neue Rechenlogik entwickelt werden, ohne einen Pico oder ein Display fuer den
Testlauf zu benoetigen.

Die letzten beiden 4-KiB-Sektoren des 2-MiB-Flashs sind linker-seitig von der
Firmware getrennt. Ein neuer Zustand wird immer in den jeweils anderen Sektor
geschrieben und erst nach erfolgreicher CRC-Pruefung aktiviert. Bei defekten
oder unbekannten Daten startet der Rechner mit sicheren Werkseinstellungen.
