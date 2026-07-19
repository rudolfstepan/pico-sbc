# USB-CDC-Protokoll

Der Scientific Calculator stellt ueber seinen normalen USB-Anschluss eine
serielle CDC-Schnittstelle bereit. Pro Zeile wird genau ein ASCII-Befehl
gesendet und genau eine Antwort empfangen. Das Protokoll ist ab
Firmware `1.1.0` verfuegbar.
Seit Firmware `1.3.0` erweitert Protokollversion 2 die Schnittstelle um
BASIC-Programme, nicht blockierende Ausfuehrung, Ausgabe und `INPUT`.
Seit Firmware `1.4.0` liefert Protokollversion 3 Dezimalergebnisse und `ANS`
verlustfrei als Text mit bis zu 80 Stellen.
Seit Firmware `1.6.0` stellt Protokollversion 4 alle Rechner-Module fuer die
Desktop-Anwendung bereit und synchronisiert auch Winkelmodus, Speicher,
Favoriten, Programmer-, Zahlenformat- und Graphzustand.

## Rahmenformat

- Befehle enden mit `LF` oder `CRLF`.
- Befehle duerfen maximal 191 druckbare ASCII-Zeichen enthalten.
- Befehlswoerter sind nicht von Gross-/Kleinschreibung abhaengig.
- Nutzdaten wie Ausdruecke bleiben unveraendert.
- Erfolgreiche Antworten beginnen mit `OK`, Fehler mit `ERR`.
- Antwortfelder sind durch Tabulatoren getrennt.
- Antworten sind einschliesslich Abschlusszeichen auf 256 Byte begrenzt.
- Verlauf und Statistikdaten werden einzeln per Index gelesen.

Beispiel, wobei `<TAB>` jeweils ein Tabulatorzeichen bezeichnet:

```text
> EVAL sqrt(4)
< OK RESULT<TAB>2
```

## Befehle

| Befehl | Funktion |
|---|---|
| `PING` | Verbindung mit `OK PONG` pruefen |
| `INFO` | Protokoll-, Firmware- und Modellversion lesen |
| `DIAG` | Seite, Winkelmodus sowie Verlauf- und Statistikstatus lesen |
| `GET RESULT` | Aktuellen Wert von `ANS` lesen |
| `GET EXPR` | Ausdruck im sichtbaren Editor lesen |
| `GET VAR A` ... `GET VAR F` | Variable lesen |
| `GET FUNC F1` ... `GET FUNC F3` | Benutzerfunktion lesen |
| `GET ANGLE` / `SET ANGLE DEG|RAD` | Winkelmodus lesen oder setzen |
| `GET MEMORY` / `SET MEMORY wert` | Speicher M lesen oder setzen |
| `GET FAVORITE 1` ... `GET FAVORITE 6` | Favoritentaste lesen |
| `SET FAVORITE 1 token` ... `SET FAVORITE 6 token` | Favoritentaste setzen |
| `GET PROGRAMMER` / `SET PROGRAMMER wert basis signed bit` | Programmer-Zustand uebertragen |
| `GET FORMAT` / `SET FORMAT bits fraction` | Wortbreite und Q-Nachkommabits uebertragen |
| `GET GRAPH` / `SET GRAPH xmin xmax ymin ymax` | Graphbereich lesen oder setzen |
| `GET HISTORY` | Anzahl der Verlaufseintraege lesen |
| `GET HISTORY index` | Einen Verlaufseintrag lesen |
| `GET STATS` | Statistikmodus und Anzahl der Zeilen lesen |
| `GET STATS index` | Eine Statistikzeile lesen |
| `GET BASIC` | Anzahl der gespeicherten BASIC-Zeilen lesen |
| `GET BASIC index` | BASIC-Zeile mit Nummer und Anweisung lesen |
| `GET BASIC STATUS` | Laufzustand, Ergebnisstatus und Schrittzahl lesen |
| `GET BASIC OUTPUT` | Anzahl der aktuellen Ausgabezeilen lesen |
| `GET BASIC OUTPUT index` | Eine Ausgabezeile lesen |
| `SET EXPR ausdruck` | Sichtbaren Ausdruck ersetzen |
| `SET VAR A wert` ... `SET VAR F wert` | Variable setzen |
| `SET FUNC F1 ausdruck` ... `SET FUNC F3 ausdruck` | Funktion validieren und setzen |
| `EVAL [ausdruck]` | Argument oder aktuellen Editor auswerten |
| `STAT MODE 1` / `STAT MODE 2` | Ein- oder Zwei-Variablen-Modus waehlen |
| `STAT CLEAR` | Statistikliste leeren |
| `STAT ADD x` | Zeile im 1VAR-Modus anfuegen |
| `STAT ADD x y` | Wertepaar im 2VAR-Modus anfuegen |
| `STAT SUMMARY X|Y` | Kennwerte einer Statistikspalte berechnen |
| `STAT REGRESSION` | Lineare Regression im 2VAR-Modus berechnen |
| `STAT HISTOGRAM` | Acht Histogrammklassen berechnen |
| `BASIC LINE zeile` | Zeile speichern oder nur per Nummer loeschen |
| `BASIC CLEAR` | BASIC-Programm und Ausgabe leeren |
| `BASIC RUN` / `BASIC STOP` | Programm nicht blockierend starten oder stoppen |
| `BASIC INPUT wert` | Angeforderten Wert oder Ausdruck an `INPUT` liefern |

### Modulbefehle

Die `MODULE`-Befehle verwenden direkt dieselben Firmware-Engines wie die
LCD-Oberflaeche. Ganzzahlen und rohe IEEE-Bitmuster werden dezimal uebertragen;
die Desktop-App nimmt BIN-/HEX-Eingaben an und konvertiert sie verlustfrei.

| Befehl | Funktion |
|---|---|
| `MODULE PROGRAMMER aktion wert bits [operand]` | `VIEW`, `AND`, `OR`, `XOR`, `NOT`, `NEG`, Shift, Rotate, Endian, Inkrement und Bitoperationen |
| `MODULE FORMAT wert bits fraction` | Unsigned, 2er-Komplement, Q-Fixpunkt sowie Float32-/Float64-Bits |
| `MODULE IEEE 32|64 raw` | IEEE-Vorzeichen, Exponent, Mantisse, Klasse und Wert |
| `MODULE GRAPH EVAL F1 x` | Eine Benutzerfunktion an einer Stelle auswerten |
| `MODULE GRAPH ROOT|DERIV|INTEGR|XING|EXTREMA ...` | Numerische Graphanalyse |
| `MODULE LOGIC INFO|EVAL|ROW ...` | Logikausdruck kompilieren, Gatter simulieren oder Wahrheitstabellenzeile lesen |
| `MODULE LOGIC FORM DNF|KNF SIMPLE|CANONICAL offset ausdruck` | Normalform stueckweise lesen |
| `MODULE UNIT CATEGORY|ITEM|CONVERT ...` | Einheitenkatalog und Umrechnung |
| `MODULE CONSTANT COUNT|index` | Physikalische Konstanten lesen |
| `MODULE COMPLEX DEG|RAD ausdruck` | Komplexen Ausdruck kartesisch und polar berechnen |

Lange KNF-/DNF-Ausgaben werden in maximal 112 Zeichen grossen Teilen gelesen.
`total` und `offset` in `OK LOGIC_FORM` erlauben dem Client, die Antwort ohne
Verlust zusammenzusetzen.

`EVAL` verwendet den am Rechner aktiven DEG- oder RAD-Modus und aktualisiert
`ANS`, den sichtbaren Editor und den Verlauf. Reine Dezimalarithmetik wird
verlustfrei als Dezimaltext uebertragen; die PC-Werkzeuge wandeln diesen Wert
nicht in binaeres Gleitkomma um. Variablen, Funktionen,
Ergebnisse, Verlauf und Statistikdaten werden wie bei einer Touch-Eingabe
persistent gespeichert. BASIC-Programme werden ebenfalls persistent
gespeichert. Eine ungueltige Funktionsdefinition veraendert den bisherigen
Zustand nicht.

Typische Antworten:

```text
OK INFO<TAB>protocol=4<TAB>firmware=1.6.0<TAB>model=scientific-calculator
OK DIAG<TAB>page=0<TAB>angle=DEG<TAB>history=2<TAB>stats=3<TAB>mode=1<TAB>basic=4<TAB>basic_state=STOPPED
OK VAR<TAB>A<TAB>3.5
OK HISTORY<TAB>0<TAB>42<TAB>6*7<TAB>42
OK STATS<TAB>0<TAB>1<TAB>3
OK RESULT<TAB>2.000000000000000000000000000000000000000000002
ERR PARSE 5
ERR LINE_TOO_LONG
```

`BASIC RUN` antwortet sofort. Der Interpreter wird danach in kleinen Paketen
in der normalen Firmware-Hauptschleife ausgefuehrt. Ein PC fragt mit
`GET BASIC STATUS` weiter ab und liest nach `FINISHED`, `ERROR` oder `INPUT`
die Ausgabe. Touch, Hardwaretasten und LCD bleiben dabei aktiv.

## Kommandozeilenwerkzeug

Das plattformunabhaengige Python-Werkzeug benoetigt Python 3.9 oder neuer und
PySerial:

```sh
python -m pip install -r tools/requirements.txt
python tools/pico_calc_cli.py ports
```

Danach wird der vom Betriebssystem angezeigte Port verwendet, unter Windows
zum Beispiel `COM5`, unter Linux typischerweise `/dev/ttyACM0`:

```sh
python tools/pico_calc_cli.py --port COM5 info
python tools/pico_calc_cli.py --port COM5 eval "sqrt(4)"
python tools/pico_calc_cli.py --port COM5 command "SET VAR A 12.5"
python tools/pico_calc_cli.py --port COM5 export calculator-state.json
python tools/pico_calc_cli.py --port COM5 import calculator-state.json
```

Der JSON-Export enthaelt Ausdruck, Ergebnis, A-F, F1-F3, M, Favoriten,
Winkelmodus, Programmer- und Zahlenformatzustand, Graphbereich, Verlauf,
BASIC-Programm und Statistikliste. Beim Import werden diese persistenten
Daten wieder uebertragen. Abhaengige
Benutzerfunktionen werden automatisch
in einer gueltigen Reihenfolge wiederholt; Rekursionen werden abgelehnt.

## Desktop-Anwendung

`Pico Calculator Link` stellt das gleiche Protokoll als grafischen
Geraetemanager bereit:

```sh
python -m pip install -r tools/requirements.txt
python tools/pico_calc_gui.py
```

Die Anwendung erkennt serielle Ports und haelt die ausgewaehlte Verbindung bis
zum Trennen offen. Alle USB-Operationen laufen in einem Hintergrund-Worker,
damit Fenster und Eingaben auch bei einem Timeout bedienbar bleiben.

- `Rechner` fuehrt wissenschaftliche Ausdruecke aus und schaltet DEG/RAD.
- `Code` bietet Programmer-, Bit-, 2er-Komplement-, Fixpunkt- und
  IEEE-Gleitkommafunktionen.
- `Graph` plottet F1-F3 und fuehrt Nullstellen-, Schnittpunkt-, Ableitungs-,
  Integral- und Extremwertanalyse auf dem Pico aus.
- `Logik` simuliert Gatter und erzeugt Wahrheitstabelle, DNF und KNF.
- `Einheiten` verwendet den vollstaendigen Einheiten- und Konstantenkatalog.
- `Komplex` wertet komplexe Ausdruecke kartesisch und polar aus.
- `Statistik` verwaltet Datensaetze und zeigt Summary, Regression und
  Histogramm.
- `Speicher` bearbeitet A-F, F1-F3, M und sechs Favoritentasten gemeinsam.
- `BASIC` laedt und speichert `.bas`-Dateien, synchronisiert den Programmspeicher,
  startet oder stoppt Programme und zeigt Ausgabe sowie `INPUT`-Zustand.
- `Verlauf` liest die acht persistenten Eintraege und uebernimmt Ausdruecke
  wieder in den Rechner.
- `Protokoll` sendet einzelne Rohbefehle und protokolliert Antworten.
- Die Geraeteleiste liest Firmware, Protokoll, Winkelmodus, Seite und
  Datenzaehler und bietet JSON-Import/-Export.

Die App verwendet nur Python, Tkinter und PySerial. Tkinter ist in den
offiziellen Python-Paketen fuer Windows und macOS enthalten. Unter Linux muss
gegebenenfalls das Paket `python3-tk` der Distribution installiert werden.

## Reaktionszeit

Die Firmware verarbeitet pro Hauptschleife hoechstens 32 empfangene Zeichen
und genau einen vollstaendigen Befehl. Lesen erfolgt ohne Wartezeit; die
USB-Ausgabe besitzt ein Timeout von 2 ms. Touch, Hardwaretasten und laufende
Berechnungen werden deshalb nicht durch einen offenen oder langsam lesenden
seriellen Port angehalten.
