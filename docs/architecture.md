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
  Auswahl zwischen exakter Dezimal-, LibBF- und TinyExpr-Auswertung
- `decimal_engine`: begrenzte Multipraezisions-Dezimalarithmetik fuer bis zu
  128 Stellen, exaktes `ANS` und kompakte BCD-Serialisierung
- `high_precision_engine`: rekursiver wissenschaftlicher Ausdrucksparser mit
  waehlbarer 192-, 320- oder 512-Bit-Arbeitsgenauigkeit und LibBF-Funktionen
- `programmer_engine`: wortbreitenabhaengige Zahlenbasen, Einzelbitoperationen,
  Rotationen sowie Carry- und Overflowstatus
- `number_formats`: Zweierkomplement, Festkomma, Byte-Reihenfolge und
  IEEE-754-Zerlegung

Alle Module ohne LCD-, Touch- oder Boardabhaengigkeit werden unter `tests/`
direkt auf dem Host mit aktivierten Compilerwarnungen getestet. Dadurch kann
neue Rechenlogik entwickelt werden, ohne einen Pico oder ein Display fuer den
Testlauf zu benoetigen.

Der Rechenkern arbeitet dreistufig. Reine Dezimalausdruecke mit `+`, `-`, `*`,
`/`, `%`, ganzzahligen Potenzen, Klammern und `ANS` werden exakt verarbeitet.
Der gemeinsame Praezisionszustand begrenzt die Ausgabe auf 40, 80 oder 128
signifikante Stellen. Wissenschaftliche Funktionen, allgemeine Potenzen,
Benutzerfunktionen und mathematische Konstanten laufen passend dazu mit 192,
320 oder 512 Bit ueber LibBF. `ANS`, A-F, M und der Verlauf bewahren den
vollstaendigen Text. TinyExpr bleibt fuer schnelle Graphabtastung und die
bestehenden `double`-Datenmodelle erhalten; dafuer wird parallel eine
Double-Nachbildung gehalten.

Die letzten vier 4-KiB-Sektoren des 2-MiB-Flashs sind linker-seitig von der
Firmware getrennt und bilden zwei redundante 8-KiB-Slots. Ein neuer Zustand
wird immer in den jeweils anderen Slot geschrieben und erst nach erfolgreicher
CRC-Pruefung aktiviert. Bei defekten oder unbekannten Daten startet der Rechner
mit sicheren Werkseinstellungen.
Flashformat 6 speichert den Praezisionsmodus sowie `ANS`, M, A-F und die acht
Verlaufsergebnisse BCD-komprimiert. Der Decoder akzeptiert ausschliesslich
Format 6. Aeltere oder unbekannte Datensaetze werden nicht migriert, sondern
durch Werkseinstellungen ersetzt.
