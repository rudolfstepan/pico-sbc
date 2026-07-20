# Firmware-Architektur

Stand: Scientific-Calculator-Firmware `2.2.0`, USB-Protokoll `5` und
Flashformat `8`.

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
- `calculator_storage`: RP2040-Flash-Backend mit zwei wechselnden 8-KiB-Slots,
  Schreibvermeidung fuer unveraenderte Daten und Ruecklesepruefung
- `calculator_precision`: gemeinsame Definition der Modi NORMAL, HIGH und
  ULTRA sowie ihrer Dezimalstellen und LibBF-Arbeitsgenauigkeit
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
- `calculator_logic` und `logic_engine`: Logikeditor, Wahrheitstabelle,
  KNF/DNF und Gatter-Simulation
- `circuit_model`: hardwareunabhaengiges Netzmodell mit stabilen Gate-Slots,
  Portverbindungen, Zyklenerkennung und boolescher Simulation
- `calculator_circuit`: Vollbildrenderer, Touch-Drag, Portverdrahtung,
  Werkzeugleiste sowie skalierter und gescrollter Weltkoordinaten-Viewport
- `calculator_units` und `unit_engine`: Einheitenkatalog, Umrechnung und
  physikalische Konstanten
- `calculator_complex` und `complex_engine`: komplexer Editor, kartesische
  und polare Auswertung sowie eigener Verlauf
- `calculator_statistics` und `statistics_engine`: persistente Datensaetze,
  Kennwerte, Regression, Histogramm und Streudiagramm
- `calculator_usb_protocol` und `calculator_usb_cdc`: testbarer ASCII-Parser
  sowie nicht blockierende USB-CDC-Anbindung
- `programmer_engine`: wortbreitenabhaengige Zahlenbasen, Einzelbitoperationen,
  Rotationen sowie Carry- und Overflowstatus
- `calculator_number_theory` und `number_theory`: 64-Bit-GGT/KGV,
  deterministischer Primzahltest, Pollard-Rho-Faktorisierung, Euler-Phi und
  modulare Potenz
- `number_formats`: Zweierkomplement, Festkomma, Byte-Reihenfolge und
  IEEE-754-Zerlegung

## PC-Werkzeuge

- `pico_calc_cli`: serielle Verbindung, Rohbefehle sowie JSON-Import und
  -Export im Format 6 einschliesslich Schaltplan
- `pico_calc_gui_model`: hardwareunabhaengige Ablaufe fuer alle
  Rechnermodule, Schaltplanvalidierung und Datensynchronisation
- `pico_calc_gui`: Tkinter-Oberflaeche Pico Calculator Link 2.2 mit 13 Tabs
  fuer Rechner, Code, Zahlentheorie, Graph, Logik, Gattereditor, Einheiten,
  Komplex, Statistik, Speicher, BASIC, Verlauf und USB-Konsole

Die PC-Anwendung nutzt fuer Modulberechnungen die jeweiligen USB-Befehle der
Firmware. Nur die boolesche Schaltplanauswertung wird zusaetzlich lokal
gespiegelt, damit Pegel waehrend der grafischen Bearbeitung sofort sichtbar
sind. Vor dem Schreiben werden Ports, Kapazitaeten und Zyklen validiert.
Exakte Werte werden fuer Anzeige und Synchronisation als Dezimaltext behandelt.

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

Die letzten vier 4-KiB-Sektoren des 2-MiB-Flashs sind per Linkerskript von der
Firmware getrennt und bilden zwei redundante 8-KiB-Slots. Ein neuer Zustand
wird immer in den jeweils anderen Slot geschrieben und erst nach erfolgreicher
CRC-Pruefung aktiviert. Bei defekten oder unbekannten Daten startet der Rechner
mit sicheren Werkseinstellungen.
Flashformat 8 speichert den Praezisionsmodus, `ANS`, M, A-F, die acht
Verlaufsergebnisse mit bis zu 128 Stellen BCD-komprimiert sowie Gates,
Leitungen, Eingangspegel und Viewport des Schaltplaneditors. Der Decoder
akzeptiert ausschliesslich Format 8. Aeltere oder unbekannte Datensaetze werden
nicht migriert, sondern durch Werkseinstellungen ersetzt.
