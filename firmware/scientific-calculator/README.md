# Scientific Calculator Firmware

Eigenstaendige Taschenrechner-Firmware fuer das LAFVIN Pico Development Kit.

## Funktionen

- Grundrechenarten mit korrekter Operatorrangfolge
- Klammern, Potenzen, Prozent/Modulo und `ANS`
- Grad- und Radiant-Modus
- `sin`, `cos`, `tan`, `asin`, `acos`, `atan`
- `sinh`, `cosh`, `tanh`, `ln`, `log`, `exp`, `sqrt`
- Betrag, Abrunden, Fakultaet, Kombinationen und Permutationen
- Konstanten Pi und e
- Fuenf Touch-Ebenen: `BASIC`, `SCIENTIFIC`, `PROGRAMMER`, `FORMAT` und `TOOLS`
- `PROGRAMMER`-Ebene fuer exakte 64-Bit-Werte
- Direkte Umwandlung zwischen Dezimal, Hexadezimal und Binaer
- Bitoperationen `AND`, `OR`, `XOR`, `NOT`, Schieben und Zweierkomplement
- Zweierkomplement-Auswertung fuer 8, 16, 32 und 64 Bit
- Q-Festkommaformate mit frei waehlbaren Nachkommabits
- IEEE-754-Umwandlung fuer Float32 und Float64, auch von und zu `ANS`
- Automatisch maximal skalierte Tastenbeschriftungen
- Ausdruckseditor mit sichtbarem Cursor und Joystick-Navigation
- Acht Eintraege Rechenverlauf mit erneutem Laden
- Speicherregister mit `M+`, `M-`, `MR` und `MC`
- Graphenmodus fuer Ausdruecke mit `x`, inklusive dynamischem Koordinatengitter,
  Achsenbeschriftung, Verschieben und Zoom
- Drei einzeln aktivierbare Graphfunktionen `F1`, `F2` und `F3` in Cyan,
  Gelb und Magenta
- Trace-Cursor, scrollbare Wertetabelle, automatische Skalierung sowie Marker
  fuer Nullstellen und Schnittpunkte
- K1 berechnet, K2 loescht das letzte Zeichen

Die Seitentaste wechselt
`BASIC -> SCIENTIFIC -> PROGRAMMER -> FORMAT -> TOOLS -> BASIC`.
Der Graphenmodus wird von `TOOLS` aus geoeffnet. Der Joystick bewegt im
Editor den Cursor und im Graphenmodus den sichtbaren Ausschnitt.
Funktionen setzen automatisch eine oeffnende Klammer. Die schliessende Klammer
wird ueber `)` eingegeben. Fuer `nCr` und `nPr` trennt `,` die Argumente, zum
Beispiel `ncr(6,2)`.

## Graph 2.0

Auf `TOOLS` stehen `X`, trigonometrische Funktionen, Klammern, Potenzen und
Operatoren direkt bereit. `GRAPH` speichert den aktuellen Ausdruck im
ausgewaehlten Funktionsplatz und skaliert die y-Achse automatisch. Ein
schneller Test ist `AC -> SIN -> X -> ) -> GRAPH`.

Die Graph-x-Achse wird immer in Radiant ausgewertet. Dadurch zeigt `sin(x)` im
Standardbereich mehrere vollstaendige Perioden, unabhaengig vom DEG/RAD-Modus
des normalen Rechners.

- `F1`, `F2`, `F3`: Funktionsplatz auswaehlen; `ON/OFF` schaltet ihn sichtbar.
- `TOOLS`: ausgewaehlte Funktion bearbeiten und mit `GRAPH` uebernehmen.
- `MORE -> TRACE`: Cursor aktivieren; Joystick links/rechts bewegt ihn.
- `MORE -> TABLE`: Wertetabelle; `X-/X+` scrollt und `STEP-/STEP+` aendert
  die Schrittweite.
- `MORE -> AUTO`: sichtbare Funktionen automatisch in y skalieren.
- `MORE -> RANGE`: x- und y-Spanne getrennt per Touch aendern oder
  mit `RESET` auf den Standardbereich zuruecksetzen.

Farbige Kreuze markieren Nullstellen. Weisse Kreuze markieren Schnittpunkte
aktiver Funktionen.

## Build

Vom Workspace-Stammverzeichnis:

```powershell
$env:PICO_SDK_PATH="C:\path\to\pico-sdk"
cmake -S . -B out/firmware -G Ninja
cmake --build out/firmware --target lafvin_scientific_calculator
```

Die flashbare Datei liegt danach unter
`out/firmware/bin/lafvin_scientific_calculator.uf2`.

## Quellstruktur

Die Bedienlogik in `calculator_ui.c` koordiniert nur noch den
Anwendungszustand und Hardwareereignisse. Seiten, Tastengeometrie, Graph,
Navigation, Ausdruckseditor, Listen und Dialogzustaende liegen in getrennten
Modulen. Hardwareunabhaengige Teile werden durch die Host-Tests unter
`../../tests/calculator` abgedeckt.

## Rechenkern

Der Ausdrucksparser ist TinyExpr von Lewis Van Winkle und steht unter der
Zlib-Lizenz. Der unveraenderte Quelltext (Revision
`4a7456e2eab88b4c76053c1c4157639ccb930e2b`) und die Lizenz befinden sich unter
`../../third_party/tinyexpr`.
