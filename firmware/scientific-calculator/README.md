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
- K1 berechnet, K2 loescht das letzte Zeichen

Die Seitentaste wechselt
`BASIC -> SCIENTIFIC -> PROGRAMMER -> FORMAT -> TOOLS -> BASIC`.
Der Graphenmodus wird von `TOOLS` aus geoeffnet. Der Joystick bewegt im
Editor den Cursor und im Graphenmodus den sichtbaren Ausschnitt.
Funktionen setzen automatisch eine oeffnende Klammer. Die schliessende Klammer
wird ueber `)` eingegeben. Fuer `nCr` und `nPr` trennt `,` die Argumente, zum
Beispiel `ncr(6,2)`.

## Build

Vom Workspace-Stammverzeichnis:

```powershell
$env:PICO_SDK_PATH="C:\path\to\pico-sdk"
cmake -S . -B out/firmware -G Ninja
cmake --build out/firmware --target lafvin_scientific_calculator
```

Die flashbare Datei liegt danach unter
`out/firmware/bin/lafvin_scientific_calculator.uf2`.

## Rechenkern

Der Ausdrucksparser ist TinyExpr von Lewis Van Winkle und steht unter der
Zlib-Lizenz. Der unveraenderte Quelltext (Revision
`4a7456e2eab88b4c76053c1c4157639ccb930e2b`) und die Lizenz befinden sich unter
`../../third_party/tinyexpr`.
