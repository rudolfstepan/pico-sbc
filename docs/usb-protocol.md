# USB-CDC-Protokoll

Der Scientific Calculator stellt ueber seinen normalen USB-Anschluss eine
serielle CDC-Schnittstelle bereit. Pro Zeile wird genau ein ASCII-Befehl
gesendet und genau eine Antwort empfangen. Das Protokoll ist ab
Firmware `1.1.0` verfuegbar.

## Rahmenformat

- Befehle enden mit `LF` oder `CRLF`.
- Befehle duerfen maximal 191 druckbare ASCII-Zeichen enthalten.
- Befehlswoerter sind nicht von Gross-/Kleinschreibung abhaengig.
- Nutzdaten wie Ausdruecke bleiben unveraendert.
- Erfolgreiche Antworten beginnen mit `OK`, Fehler mit `ERR`.
- Antwortfelder sind durch Tabulatoren getrennt.
- Verlauf und Statistik werden einzeln per Index gelesen, damit jede Antwort
  kleiner als 192 Byte bleibt.

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
| `GET HISTORY` | Anzahl der Verlaufseintraege lesen |
| `GET HISTORY index` | Einen Verlaufseintrag lesen |
| `GET STATS` | Statistikmodus und Anzahl der Zeilen lesen |
| `GET STATS index` | Eine Statistikzeile lesen |
| `SET EXPR ausdruck` | Sichtbaren Ausdruck ersetzen |
| `SET VAR A wert` ... `SET VAR F wert` | Variable setzen |
| `SET FUNC F1 ausdruck` ... `SET FUNC F3 ausdruck` | Funktion validieren und setzen |
| `EVAL [ausdruck]` | Argument oder aktuellen Editor auswerten |
| `STAT MODE 1` / `STAT MODE 2` | Ein- oder Zwei-Variablen-Modus waehlen |
| `STAT CLEAR` | Statistikliste leeren |
| `STAT ADD x` | Zeile im 1VAR-Modus anfuegen |
| `STAT ADD x y` | Wertepaar im 2VAR-Modus anfuegen |

`EVAL` verwendet den am Rechner aktiven DEG- oder RAD-Modus und aktualisiert
`ANS`, den sichtbaren Editor und den Verlauf. Variablen, Funktionen,
Ergebnisse, Verlauf und Statistikdaten werden wie bei einer Touch-Eingabe
persistent gespeichert. Eine ungueltige Funktionsdefinition veraendert den
bisherigen Zustand nicht.

Typische Antworten:

```text
OK INFO<TAB>protocol=1<TAB>firmware=1.2.0<TAB>model=scientific-calculator
OK DIAG<TAB>page=0<TAB>angle=DEG<TAB>history=2<TAB>stats=3<TAB>mode=1
OK VAR<TAB>A<TAB>3.5
OK HISTORY<TAB>0<TAB>42<TAB>6*7<TAB>42
OK STATS<TAB>0<TAB>1<TAB>3
ERR PARSE 5
ERR LINE_TOO_LONG
```

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

Der JSON-Export enthaelt Ausdruck, Ergebnis, A-F, F1-F3, Verlauf und die
Statistikliste. Beim Import werden Ausdruck, Variablen, Funktionen und
Statistikdaten uebertragen. Abhaengige Benutzerfunktionen werden automatisch
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

- `Rechner` fuehrt wissenschaftliche Ausdruecke aus und zeigt die Sitzung.
- `Speicher` bearbeitet A-F und F1-F3 gemeinsam.
- `Statistik` verwaltet bis zu 32 lokale Werte oder Wertepaare und uebertraegt
  sie gesammelt zum Rechner.
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
