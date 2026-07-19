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
- Zehn Touch-Ebenen: `BASIC`, `SCIENTIFIC`, `PROGRAMMER`, `FORMAT`, `TOOLS`,
  `SYMBOLS`, `LOGIC`, `UNITS`, `COMPLEX` und `STATS`
- `PROGRAMMER`-Ebene fuer exakte 64-Bit-Werte
- Direkte Umwandlung zwischen Dezimal, Hexadezimal und Binaer
- Bitoperationen `AND`, `OR`, `XOR`, `NOT`, Schieben und Zweierkomplement
- Signed-/Unsigned-Anzeige, Einzelbitbearbeitung, Rotate Left/Right,
  Byte-Reihenfolge sowie sichtbare Carry- und Overflowflags
- Zweierkomplement-Auswertung fuer 8, 16, 32 und 64 Bit
- Q-Festkommaformate mit frei waehlbaren Nachkommabits
- IEEE-754-Umwandlung fuer Float32 und Float64, auch von und zu `ANS`
- IEEE-754-Inspector fuer Vorzeichen, Roh- und Arbeitsexponent, Mantisse und
  Klassifikation als Normal, Subnormal, Null, Unendlich oder NaN
- Schaltalgebra mit Wahrheitstabelle, vereinfachter und kanonischer KNF/DNF
  sowie Live-Simulation fuer bis zu sechs Eingaenge
- 68 Einheiten in zehn Kategorien sowie zwoelf physikalische Konstanten mit
  Einheit und Quellenangabe
- Komplexer Rechner mit kartesischer und polarer Anzeige, Betrag, Phase,
  Konjugation und eigenem verlustfreien Verlauf
- Ein- und Zwei-Variablen-Statistik mit editierbarer Zahlenliste,
  Standardabweichung, linearer Regression, Histogramm und Streudiagramm
- Automatisch maximal skalierte Tastenbeschriftungen
- Ausdruckseditor mit sichtbarem Cursor und Joystick-Navigation
- Acht Eintraege Rechenverlauf mit erneutem Laden
- Speicherregister mit `M+`, `M-`, `MR` und `MC`
- Sechs Variablen `A` bis `F`, drei Benutzerfunktionen `F1(x)` bis `F3(x)`
  und sechs frei belegbare Favoritentasten
- Automatische, stromausfallsichere Speicherung von Einstellungen, Verlauf,
  Symbolen und Graphbereichen
- Graphenmodus fuer Ausdruecke mit `x`, inklusive dynamischem Koordinatengitter,
  Achsenbeschriftung, Verschieben und Zoom
- Drei einzeln aktivierbare Graphfunktionen `F1`, `F2` und `F3` in Cyan,
  Gelb und Magenta
- Trace-Cursor, scrollbare Wertetabelle, automatische Skalierung sowie Marker
  fuer Nullstellen und Schnittpunkte
- K1 berechnet beziehungsweise fuegt Statistikdaten hinzu; K2 schaltet das
  Displaylayout um
- USB-CDC-Protokoll und Python-CLI fuer Berechnung, Diagnose sowie
  JSON-Import und -Export

Die Seitentaste wechselt
`BASIC -> SCIENTIFIC -> PROGRAMMER -> FORMAT -> TOOLS -> SYMBOLS -> LOGIC -> UNITS -> COMPLEX -> STATS -> BASIC`.
Der Graphenmodus wird von `TOOLS` aus geoeffnet. Der Joystick bewegt im
Editor den Cursor und im Graphenmodus den sichtbaren Ausschnitt.
Funktionen setzen automatisch eine oeffnende Klammer. Die schliessende Klammer
wird ueber `)` eingegeben. Fuer `nCr` und `nPr` trennt `,` die Argumente, zum
Beispiel `ncr(6,2)`.

## Displaylayout

K2 wechselt jederzeit zwischen zwei Ansichten. Die Standardansicht verwendet
einen 84 Pixel hohen Datenbereich und grosse Touch-Tasten. Im Datenfokus ist
der Datenbereich mit 168 Pixeln doppelt so hoch; Inhalte, Diagramme und
Datenschrift nutzen den zusaetzlichen Platz. Normale Datenzeilen wachsen von
Schriftfaktor 1 auf 2, grosse Eingaben und Ergebnisse von Faktor 2 auf 3. Das
fuenfzeilige Tastenfeld ist mit 113 statt 226 Pixeln halb so hoch. Zeichnung
und Touch-Hitboxen werden gemeinsam umgeschaltet.

Die aktive Ansicht bleibt bis zum naechsten Druck auf K2 erhalten und startet
nach einem Neustart wieder im Standardlayout. `DEL` auf dem Touchscreen
uebernimmt in beiden Ansichten das Loeschen. K1 bleibt fuer `=`, Programmer-
Operationen und `ADD` im Statistikmodus reserviert.

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

## Numerische Analyse

`MORE -> ANALYZE` oeffnet die numerischen Werkzeuge. Im Kopf des Graphen
stehen die verwendete Toleranz `1e-9`, die Grenze von `64` Iterationen und das
aktuelle Intervall.

- `ROOT`: Nullstelle der ausgewaehlten Funktion. Der Trace-x-Wert ist der
  Startwert; falls Newton dort nicht konvergiert, wird der sichtbare Bereich
  nach einem Intervall mit Vorzeichenwechsel durchsucht.
- `XING`: Schnittpunkt der ausgewaehlten mit der naechsten aktiven Funktion.
- `DERIV`: numerische Ableitung der ausgewaehlten Funktion am Trace-x-Wert.
- `INTEGR`: bestimmtes Integral ueber den sichtbaren x-Bereich.
- `EXTREM`: lokale Minima und Maxima im sichtbaren x-Bereich.

`ANALYZE -> MORE` verwaltet die Rechenparameter:

- `A=ANS` und `B=ANS` uebernehmen den aktuellen Rechnerwert als exakte
  Intervallgrenze. Die Reihenfolge ist beliebig; intern wird die kleinere
  Grenze zuerst verwendet.
- `VIEW` verwendet wieder den aktuell sichtbaren x-Bereich als Intervall.
- `TOL` schaltet zyklisch zwischen `1e-6`, `1e-9` und `1e-12` um.

Zum Setzen einer exakten Grenze wird der gewuenschte Wert zuerst im normalen
Rechner berechnet. Danach wird im Graph unter `ANALYZE -> MORE` die passende
`A=ANS`- oder `B=ANS`-Taste gedrueckt. Der Graphkopf kennzeichnet ein solches
Intervall mit `A/B`, andernfalls mit `VIEW`.

Ergebnisse enthalten die benoetigte Iterationszahl. Fehler wie ungueltige
Wertebereiche oder fehlende Konvergenz werden direkt im Graphkopf angezeigt.
Mit `RANGE` und dem Joystick lassen sich Intervall und Startwert vor der
Berechnung anpassen.

## Schaltalgebra und Gatter

Auf `LOGIC` werden Boolesche Ausdruecke mit den Variablen `A` bis `F`
eingegeben. Unterstuetzt werden `NOT`, `AND`, `OR`, `XOR`, `NAND`, `NOR`,
`XNOR`, Konstanten `0` und `1` sowie Klammern. Ein schneller Test fuer ein
Exklusiv-Oder ist `A -> XOR -> B -> CHECK`.

- `TABLE` zeigt vier Zeilen der Wahrheitstabelle; `UP` und `DOWN` blaettern.
- `DNF` und `KNF` zeigen zuerst die vereinfachte Form. Erneutes Antippen
  derselben Taste schaltet auf die kanonische Form um.
- `USE` laedt eine angezeigte Form in den Editor, sofern sie hineinpasst.
- `GATES` oeffnet die Live-Simulation. `A` bis `F` schalten die verwendeten
  Eingaenge; helle Tasten bedeuten logisch `1`. Ausgang und Gatteranzahl werden
  sofort aktualisiert.
- `CHECK` prueft Syntax und Gatterbaum. Die Rangfolge ist
  `NOT`, `AND/NAND`, `XOR/XNOR`, `OR/NOR`.

Die Logik wird als Ausdrucksbaum aufgebaut. Dadurch sind offene Verbindungen
und zyklische Netze nicht darstellbar; unvollstaendige Ausdruecke werden mit
der Fehlerposition abgelehnt.

## Einheiten und Konstanten

Auf `UNITS` stehen Laenge, Flaeche, Volumen, Masse, Zeit, Temperatur, Winkel,
Druck, Energie und Leistung zur Wahl. Die aktuelle Quell- und Zieleinheit
steht ausgeschrieben im Display.

1. Den Ausgangswert im normalen Rechner berechnen.
2. Zu `UNITS` wechseln und die Kategorie auswaehlen.
3. Mit `<FROM`/`FROM>` und `<TO`/`TO>` die Einheiten einstellen.
4. `ANS>IN` und danach bei Bedarf `CONVERT` druecken.
5. Das Ergebnis mit `OUT>ANS` uebernehmen oder mit `OUT>EDIT` in einen neuen
   Rechenausdruck laden.

`SWAP` vertauscht Quell- und Zieleinheit. Temperaturen unter dem absoluten
Nullpunkt werden abgelehnt. `CONST` oeffnet die Konstantenliste; `C-` und `C+`
blaettern, `C>ANS` und `C>EDIT` uebernehmen den Wert. `INFO` zeigt die Quelle.

Exakte SI-Definitionswerte stammen aus der
[BIPM SI Brochure](https://www.bipm.org/en/publications/si-brochure/).
Weitere Werte folgen den
[NIST CODATA 2022 constants](https://physics.nist.gov/cuu/Constants/index.html).

## Komplexe Zahlen

`COMPLEX` besitzt einen eigenen Ausdruckseditor und Ergebniszustand. Eine
kartesische Zahl wird direkt als `3+4i` eingegeben; zwischen Zahl und `i` ist
kein Multiplikationszeichen erforderlich. Auch Ausdruecke wie
`(1+i)(1-i)` funktionieren durch implizite Multiplikation.

- `CONJ`, `ABS` und `ARG` fuegen Konjugation, Betrag und Phase ein.
- `POLAR` erzeugt `polar(Betrag,Winkel)`, zum Beispiel `polar(5,30)`.
- `CART/POLAR` schaltet nur die Ergebnisdarstellung um; der gespeicherte Wert
  bleibt unveraendert.
- `DEG/RAD` gilt fuer `ARG`, `POLAR` und die polare Anzeige.
- `HIST` oeffnet acht komplexe Verlaufseintraege. `PREV`, `NEXT` und `USE`
  navigieren und laden sie; `HCLR` loescht nur den komplexen Verlauf.

Nach `=` kann mit `+`, `-`, `*` oder `/` direkt am gesamten komplexen Ergebnis
weitergerechnet werden. Parserfehler, Division durch null, ungueltige
Polarwerte und Zahlenbereichsfehler erscheinen als getrennte Statusmeldungen.

## Statistik

`STATS` verwaltet bis zu 32 Zahlen oder Zahlenpaare. Im Modus `1VAR` wird der
X-Wert eingegeben und mit `ADD` angehaengt. Fuer `2VAR` zuerst X eingeben, mit
`X/Y` zum Y-Feld wechseln, Y eingeben und danach `ADD` druecken. `ANS` laedt
das letzte normale Rechenergebnis in das aktive Feld.

- `PREV` und `NEXT` waehlen eine Zeile aus.
- `EDIT` laedt die ausgewaehlte Zeile; `ADD` speichert die Aenderung.
- `DROP` entfernt die ausgewaehlte Zeile, `CLEAR` leert die Liste.
- `SUM` zeigt Anzahl, Mittelwert, Median, Minimum, Maximum sowie Populations-
  und Stichproben-Standardabweichung. In `2VAR` waehlt `X/Y` die Spalte.
- `REG` zeigt die lineare Funktion, den Korrelationskoeffizienten und dessen
  Quadrat. Dafuer werden mindestens zwei Zahlenpaare benoetigt.
- `PLOT` zeichnet in `1VAR` ein Histogramm und in `2VAR` ein Streudiagramm
  mit gestrichelter Regressionsgerade.

K1 entspricht `ADD`. K2 schaltet das Displaylayout um. Der Joystick waehlt
mit links/rechts eine Zeile und mit oben/unten das X- oder Y-Feld. Modus und
Zahlenliste werden automatisch im Flash gespeichert.

## USB-Datenaustausch

Ab Firmware `1.1.0` nimmt der Rechner ueber USB zeilenweise CDC-Befehle an.
Damit lassen sich Ausdruecke berechnen, Variablen und Funktionen setzen sowie
Verlauf und Statistikdaten lesen oder schreiben. `INFO` zeigt die Firmware- und
Protokollversion, `DIAG` den kompakten Laufzeitzustand.

```sh
python -m pip install -r tools/requirements.txt
python tools/pico_calc_cli.py ports
python tools/pico_calc_cli.py --port COM5 eval "sqrt(2)"
python tools/pico_calc_cli.py --port COM5 export calculator-state.json
python tools/pico_calc_gui.py
```

Die grafische Anwendung `Pico Calculator Link` bietet Rechnersteuerung,
Synchronisation von A-F, F1-F3 und Statistiklisten, Verlauf, Rohkonsole sowie
JSON-Sicherungen. Die vollstaendige Befehlsreferenz, Linux-Beispiele, Grenzen
und das JSON-Format stehen in
[docs/usb-protocol.md](../../docs/usb-protocol.md).

## Erweiterte Programmer-Werkzeuge

Auf `FORMAT` oeffnet `BITS` die wortbreitenabhaengigen Programmer-Werkzeuge.
Die gewaehlte Breite 8, 16, 32 oder 64 Bit gilt fuer Eingabe und Operationen.

- `SIGNED/UNSIGNED` schaltet die Dezimalinterpretation um.
- `BIT-8`, `BIT-1`, `BIT+1`, `BIT+8` waehlen eine Bitposition aus.
- `SET`, `CLR` und `TOGGLE` bearbeiten das ausgewaehlte Bit.
- `ROL`, `ROR`, `<<`, `>>`, `+1`, `-1` und `NEG` aktualisieren die sichtbaren
  Flags `C` fuer Carry und `V` fuer Signed-Overflow.
- `SWAP` kehrt die Byte-Reihenfolge innerhalb der aktuellen Wortbreite um.
- `IEEE32` und `IEEE64` oeffnen den Inspector. `S`, `E`, `X` und `M` zeigen
  Vorzeichen, Roh-Exponent, Arbeitsexponent und Mantisse.

Die Ansichten `CONV`, `BITS`, `F32` und `F64` koennen direkt ueber ihre
Modustasten gewechselt werden. Wortbreite, Signed-Modus und Bitposition werden
mit dem restlichen Rechnerzustand gespeichert.

## Variablen, Funktionen und Favoriten

Auf `SYMBOLS` werden A-F sowie die drei Funktionsdefinitionen gemeinsam
angezeigt.

- `A=ANS` bis `F=ANS` speichern das aktuelle Ergebnis in der Variablen.
- `A` bis `F` fuegen die Variable in den Ausdruck ein und wechseln zu `TOOLS`.
- `F1`, `F2` oder `F3` waehlt eine Benutzerfunktion aus.
- `EDIT` laedt ihre Definition nach `TOOLS`. Dort wird sie bearbeitet; danach
  wechselt `SYMBOLS` zurueck zur Liste und `SAVE` uebernimmt sie.
- Eine leere Definition kann mit `AC -> SYMBOLS -> SAVE` geloescht werden.
- Die sechs Favoritentasten fuegen ihren Text in den Ausdruck ein. `SET1` bis
  `SET6` belegen sie mit dem aktuellen Ausdruck neu.

Benutzerfunktionen duerfen A-F, `ANS`, `x`, alle normalen mathematischen
Funktionen und bereits definierte F1-F3 verwenden. Direkte und indirekte
Rekursion sowie Syntaxfehler werden beim Speichern abgelehnt. Gespeicherte
Funktionen koennen beispielsweise als `f1(2)` in BASIC, SCIENTIFIC und TOOLS
aufgerufen oder in weiteren Graphfunktionen verwendet werden.

## Permanente Speicherung

Der Rechner speichert Winkelmodus, `ANS`, Speicherregister, letzte Seite,
Programmer-Zustand, Verlauf, Variablen, Benutzerfunktionen, Favoriten,
Graphbereiche und Statistiklisten automatisch. Aenderungen werden drei
Sekunden gesammelt; ein
unveraenderter Zustand wird nicht erneut in den Flash geschrieben.

Zwei wechselnde, jeweils mit CRC32 geschuetzte Flashkopien verhindern, dass
ein Stromausfall waehrend des Speicherns den vorherigen Zustand zerstoert.
Defekte oder unbekannte Speicherdaten werden beim Start verworfen und durch
sichere Werkseinstellungen ersetzt.

Fuer einen Werksreset werden beide Hardwaretasten `K1` und `K2` beim
Einschalten beziehungsweise Reset gleichzeitig gehalten und nach etwa zwei
Sekunden losgelassen. Die Firmware bestaetigt dies mit `FACTORY RESET`.

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
