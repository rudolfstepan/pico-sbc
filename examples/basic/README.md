# BASIC-Testprogramme

Diese Programme koennen im BASIC-Tab von `Pico Calculator Link` mit
`Datei laden` geoeffnet, zum Pico uebertragen und mit `Ausfuehren` gestartet
werden. Die Sammlung bleibt innerhalb der Grenzen des Rechners: maximal
20 Zeilen, 63 Zeichen pro Anweisung und sechs sichtbare Ausgabezeilen.

| Datei | Test | Eingabe | Letzte Ausgabe |
|---|---|---:|---:|
| `01_hello.bas` | Textausgabe und `CLS` | - | `READY` |
| `02_arithmetic.bas` | Variablen, Produkt und Quadratwurzel | - | `42` |
| `03_for_loop.bas` | Aufsteigende `FOR`-Schleife | - | `25` |
| `04_input_branch.bas` | `INPUT`, `IF/THEN` und `ABS` | `-9` | `9` |
| `05_countdown.bas` | Rueckwaertszaehlen mit negativem `STEP` | - | `START` |
| `06_sum_goto.bas` | Zuweisung und bedingter Ruecksprung | - | `55` |
| `07_factorial.bas` | Eingabe und iterative Berechnung | `5` | `120` |
| `08_trigonometry.bas` | `SIN` und `PI()` | - | `1` |

Bei Programmen mit `INPUT` erscheint der Zustand `INPUT` in der PC-Oberflaeche.
Den Tabellenwert in das Eingabefeld schreiben und `Eingabe senden` waehlen.
Alle Dateien werden auch durch den Host-Test `basic_engine` geladen und
ausgefuehrt.
