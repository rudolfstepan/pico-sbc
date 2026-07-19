# Erweiterungs-Roadmap

Diese Liste beschreibt die geplanten Erweiterungen des wissenschaftlichen
Taschenrechners in ihrer empfohlenen Umsetzungsreihenfolge. Eine Phase gilt
erst als abgeschlossen, wenn Implementierung, Host-Tests, Firmware-Build und
ein Test auf dem echten LCD erfolgreich sind.

## Uebersicht

| Phase | Erweiterung | Abhaengigkeit | Status |
|---:|---|---|---|
| 0 | Technische Grundlage | keine | abgeschlossen |
| 1 | Graph 2.0 | Phase 0 | abgeschlossen |
| 2 | Numerische Analyse | Phase 1 | abgeschlossen |
| 3 | Variablen und Benutzerfunktionen | Phase 2 | abgeschlossen |
| 4 | Permanente Speicherung | Phase 3 | Hardwaretest offen |
| 5 | Programmer-Erweiterungen | Phase 0 | Hardwaretest offen |
| 6 | Schaltalgebra und Gatter | Phase 5 | abgeschlossen |
| 7 | Einheiten und Konstanten | Phase 3 | abgeschlossen |
| 8 | Komplexe Zahlen | Phase 3 | abgeschlossen |
| 9 | Statistikmodus | Phasen 2 und 3 | Hardwaretest offen |
| 10 | USB-Datenaustausch | Phasen 4 und 9 | Hardwaretest offen |
| 11 | BASIC-Programmiermodus | Phasen 0 und 4 | Hardwaretest offen |

## Phase 0: Technische Grundlage

- [x] `calculator_ui.c` in Seiten, Widgets und Navigation aufteilen.
- [x] Gemeinsame Eingabe-, Dialog- und Listenkomponenten bereitstellen.
- [x] Rechenlogik konsequent von LCD-, Touch- und Boardcode trennen.
- [x] Einheitliche Fehlercodes fuer Zahlenbereich, Konvergenz und Syntax nutzen.
- [x] Host-Tests fuer jede neue hardwareunabhaengige Komponente vorbereiten.

**Fertig, wenn:** Die vorhandene Firmware unveraendert bedienbar bleibt, beide
Firmware-Ziele bauen und alle bestehenden Tests weiterhin bestehen.

Softwarestand: Neun Host-Tests und beide Firmware-Ziele bauen erfolgreich.
Anzeige, Touch und Reaktion wurden anschliessend auf dem Pico geprueft.

## Phase 1: Graph 2.0

- [x] Drei Funktionsplaetze `F1`, `F2` und `F3` verwalten.
- [x] Funktionen einzeln aktivieren und in unterscheidbaren Farben zeichnen.
- [x] Trace-Cursor mit numerischer `x`- und `y`-Anzeige integrieren.
- [x] Eine scrollbare Wertetabelle fuer den aktuellen Graphbereich anzeigen.
- [x] Automatische Skalierung fuer den sichtbaren Wertebereich implementieren.
- [x] Nullstellen und Schnittpunkte im sichtbaren Bereich markieren.
- [x] Graphbereiche und Schrittweite direkt am Touchscreen bearbeiten.

**Fertig, wenn:** Drei Funktionen gleichzeitig dargestellt, verfolgt und als
Wertetabelle angezeigt werden koennen, ohne dass Beschriftungen ueberlappen.

Abschlussstand: Graphmodell und LCD-Renderer sind durch Host-Tests fuer
Funktionsspeicher, Viewport, Auto-Skalierung, Nullstellen, Schnittpunkte,
Radiant-Auswertung und Zeichenbereich abgesichert. Darstellung und
Touchbedienung wurden auf dem 480-x-320-LCD erfolgreich geprueft.

## Phase 2: Numerische Analyse

- [x] Nullstellensuche mit Startwert oder Intervall implementieren.
- [x] Schnittpunkte zweier gespeicherter Funktionen berechnen.
- [x] Numerische Ableitung an einer frei waehlbaren Stelle anbieten.
- [x] Bestimmte Integrale ueber ein eingegebenes Intervall berechnen.
- [x] Lokale Minima und Maxima im sichtbaren Bereich suchen.
- [x] Iterationsgrenzen, Toleranzen und Konvergenzfehler sichtbar behandeln.

**Fertig, wenn:** Solver, Ableitung und Integral reproduzierbare Ergebnisse
liefern und Grenzfaelle durch automatisierte Tests abgedeckt sind.

Abschlussstand: Referenzwerte, exakte A/B-Intervalle, fehlende
Nullstellenintervalle, Iterationsabbrueche, TinyExpr-Anbindung und
LCD-Zeichenbereich werden durch Host-Tests abgedeckt. Ergebnisse, Touchwege,
Lesbarkeit und Laufzeit wurden anschliessend erfolgreich auf dem Pico geprueft.

## Phase 3: Variablen und Benutzerfunktionen

- [x] Variablen `A` bis `F` speichern und in Ausdruecken verwenden.
- [x] Benutzerfunktionen wie `F1(x)=...` bearbeiten und aufrufen.
- [x] Favoritentasten mit haeufig verwendeten Funktionen belegen.
- [x] Variablen und Funktionen in einer uebersichtlichen Liste verwalten.
- [x] Rekursive oder ungueltige Funktionsdefinitionen sicher ablehnen.

**Fertig, wenn:** Variablen und Funktionen in BASIC, SCIENTIFIC, TOOLS und
GRAPH konsistent verwendet werden koennen.

Abschlussstand: Das Symbolmodell, die TinyExpr-Anbindung, Navigation,
Tastenbelegung und LCD-Grenzen werden durch Host-Tests abgedeckt. Beide
Firmware-Ziele bauen. Variablen, Benutzerfunktionen, Favoriten, Graphanbindung,
Fehlerbehandlung und Touchbedienung wurden auf dem Pico erfolgreich geprueft.

## Phase 4: Permanente Speicherung

- [x] Ein versioniertes Speicherformat mit Pruefsumme definieren.
- [x] Winkelmodus, Speicherregister und zuletzt verwendete Seite sichern.
- [x] Verlauf, Benutzerfunktionen und Graphbereiche persistent speichern.
- [x] Schreibzugriffe zusammenfassen und Flash-Verschleiss begrenzen.
- [x] Werkseinstellungen und Wiederherstellung bei defekten Daten anbieten.

**Fertig, wenn:** Einstellungen einen Neustart ueberstehen und eine
unterbrochene Speicherung nicht zu einem unbenutzbaren System fuehrt.

Softwarestand: Das versionierte Format, CRC32, Sequenzueberlauf sowie die
Auswahl der neuesten gueltigen Kopie werden durch Host-Tests abgedeckt. Zwei
wechselnde 4-KiB-Flashsektoren schuetzen den vorherigen Zustand bei einem
unterbrochenen Schreibvorgang. Der abschliessende Neustart-, Touch- und
Werksreset-Test auf dem Pico steht noch aus.

## Phase 5: Programmer-Erweiterungen

- [x] Zwischen Signed- und Unsigned-Darstellung umschalten.
- [x] Einzelne Bits anzeigen, setzen, loeschen und umschalten.
- [x] Rotate Left und Rotate Right fuer alle Wortbreiten implementieren.
- [x] Little-Endian- und Big-Endian-Umwandlung anbieten.
- [x] Einen IEEE-754-Inspector fuer Vorzeichen, Exponent und Mantisse bauen.
- [x] Ueberlauf- und Carry-Informationen sichtbar ausgeben.

**Fertig, wenn:** Alle Operationen fuer 8, 16, 32 und 64 Bit getestet sind und
die Bitansicht auf dem LCD ohne horizontales Abschneiden funktioniert.

Softwarestand: Wortbreiten, Rotationen, Schieben, Einzelbitoperationen,
Byte-Reihenfolge, Signed-Overflow, Carry und IEEE-754-Klassen werden durch
Host-Tests abgedeckt. Die Programmer-, Bit- und IEEE-LCD-Ansichten werden auf
Zeichenbereichsfehler geprueft. Der abschliessende Touch- und Lesbarkeitstest
auf dem Pico steht noch aus.

## Phase 6: Schaltalgebra und Gatter

- [x] Boolesche Ausdruecke mit Variablen und Klammern parsen.
- [x] `NOT`, `AND`, `OR`, `XOR`, `NAND`, `NOR` und `XNOR` unterstuetzen.
- [x] Wahrheitstabellen fuer bis zu sechs Eingangsvariablen erzeugen.
- [x] Kanonische KNF und DNF berechnen und lesbar anzeigen.
- [x] Boolesche Ausdruecke zu vereinfachten KNF-/DNF-Formen reduzieren.
- [x] Eine touchbedienbare Gatter-Simulation mit schaltbaren Eingaengen bauen.
- [x] Syntaxfehler sicher ablehnen; der Ausdrucksbaum kann keine offenen oder
  zyklischen Netze erzeugen.

**Fertig, wenn:** Ausdruck, Wahrheitstabelle, KNF/DNF und Gattersimulation fuer
dieselben Eingangswerte identische Ergebnisse liefern und die wichtigsten
Gatterkombinationen durch Host-Tests abgesichert sind.

Abschlussstand: Der getrennte Logikkern, Editor, Wahrheitstabelle, KNF/DNF und
die Live-Gatteransicht sind implementiert und durch Host-Tests abgesichert.
Touchbedienung, Wahrheitstabelle und Gattereingaben wurden anschliessend auf
dem Pico erfolgreich geprueft. Firmware 1.4.1 ergaenzt die fehlenden
LCD-Zeichen fuer `&` und `|` sowie die direkte Ausgangsanzeige nach `CHECK`.

## Phase 7: Einheiten und Konstanten

- [x] Kategorien fuer Laenge, Flaeche, Volumen, Masse und Zeit anlegen.
- [x] Temperatur, Winkel, Druck, Energie und Leistung ergaenzen.
- [x] Umrechnungen ueber eine gemeinsame Basiseinheit modellieren.
- [x] Physikalische Konstanten mit Einheit und Quellenhinweis hinterlegen.
- [x] Ergebnisse direkt an `ANS` oder den Ausdruckseditor uebergeben.

**Fertig, wenn:** Hin- und Rueckumwandlungen innerhalb definierter Toleranzen
getestet sind und inkompatible Einheiten nicht kombiniert werden.

Abschlussstand: Zehn Kategorien, affine Temperaturumrechnung, 68 Einheiten und
zwoelf Konstanten sind implementiert. Hin- und Rueckumwandlungen,
Kategoriefehler, absoluter Nullpunkt, Touchbelegung und LCD-Grenzen werden von
Host-Tests abgedeckt. Umrechnung, `SWAP`, Konstanten und Wertuebergabe wurden
anschliessend auf dem Pico erfolgreich geprueft.

## Phase 8: Komplexe Zahlen

- [x] Einen komplexen Zahlentyp fuer den Rechenkern einfuehren.
- [x] Eingaben wie `3+4i` sowie Polarform unterstuetzen.
- [x] Betrag, Phase, Konjugation und Grundrechenarten implementieren.
- [x] Zwischen kartesischer und polarer Darstellung umschalten.
- [x] Komplexe Ergebnisse eindeutig von reellen Fehlerwerten unterscheiden.

**Fertig, wenn:** Parser, Anzeige und Verlauf komplexe Zahlen verlustfrei
verarbeiten und die wichtigsten Operationen durch Host-Tests abgesichert sind.

Abschlussstand: Ein eigener Parser unterstuetzt kartesische Zahlen, implizite
Multiplikation, Polarkonstruktion, Grundrechenarten, Potenzen, Betrag, Phase,
Konjugation, Real-/Imaginaerteil, Wurzel, Exponentialfunktion und Logarithmus.
Ein acht Eintraege grosser Verlauf speichert Ausdruck und beide
Double-Komponenten verlustfrei. Alle 21 Host-Tests und der Pico-Firmware-Build
laufen erfolgreich. Parser, Berechnungen, Verlauf, Touchbedienung und
Lesbarkeit wurden anschliessend auf dem Pico erfolgreich geprueft.

## Phase 9: Statistikmodus

- [x] Einen touchfreundlichen Editor fuer Zahlenlisten erstellen.
- [x] Mittelwert, Median, Minimum, Maximum und Standardabweichung berechnen.
- [x] Ein- und Zwei-Variablen-Statistik unterstuetzen.
- [x] Lineare Regression mit Korrelationskoeffizient implementieren.
- [x] Histogramm und Streudiagramm im Graphbereich darstellen.
- [x] Zahlenlisten persistent speichern und beim Start wiederherstellen.
- [x] Datenfokus mit doppelter Anzeigehoehe und groesserer Datenschrift per
  Hardwaretaste bereitstellen.

**Fertig, wenn:** Listen bearbeitet, ausgewertet und grafisch dargestellt
werden koennen und Ergebnisse gegen bekannte Datensaetze getestet sind.

Softwarestand: Ein Editor fuer bis zu 32 Werte oder Wertepaare, Kennwerte fuer
X und Y, lineare Regression mit `r` und `r^2`, Histogramm und Streudiagramm
sind implementiert. Datensaetze werden im CRC-gesicherten Flashformat
gespeichert; Version 2 liest bestehende Version-1-Daten weiterhin. Alle 23
Host-Tests und der RP2040-Release-Build laufen erfolgreich. Touchbedienung,
Lesbarkeit der Diagramme und Wiederherstellung nach einem Neustart stehen noch
als Hardwaretest aus. K2 durchlaeuft Standardlayout, Datenfokus und Vollbild;
im Vollbild wird die Tastatur samt Touch-Hitboxen ausgeblendet. Doppelte
Anzeigehoehe und vergroesserte Datenschrift wurden auf dem Pico erfolgreich
bestaetigt. Firmware 1.5.0 ergaenzt eine per langem K2-Druck umschaltbare
Landscape-/Portrait-Darstellung mit gemeinsam gedrehten Touchkoordinaten.

## Phase 10: USB-Datenaustausch

- [x] Ein einfaches, dokumentiertes USB-CDC-Befehlsformat definieren.
- [x] Ausdruecke, Variablen, Funktionen und Wertelisten importieren.
- [x] Ergebnisse, Verlauf und Statistikdaten exportieren.
- [x] Diagnoseausgaben und Firmwareinformationen abrufbar machen.
- [x] Eingaben begrenzen und ungueltige Befehle ohne Absturz ablehnen.
- [x] Ein kleines plattformunabhaengiges Kommandozeilenwerkzeug bereitstellen.
- [x] Eine grafische Desktop-Anwendung fuer Steuerung und Synchronisation
  bereitstellen.
- [x] BASIC-Programme laden, speichern, synchronisieren und ausfuehren.

**Fertig, wenn:** Daten zwischen Rechner und PC in beide Richtungen uebertragen
werden koennen, ohne Touchbedienung oder Berechnungen zu blockieren.

Softwarestand: Protokollversion 3 und Firmware 1.5.0 stellen einen begrenzten
ASCII-Zeilenparser, verlustfreie Dezimalergebnisse, transaktionale
Funktionsimporte, Statistik- und BASIC-Programmlisten sowie Firmware- und
Diagnoseinformationen bereit. CLI und
`Pico Calculator Link` unter `tools/` unterstuetzen Portsuche, Einzelbefehle,
Berechnung, Speicher-/Statistiksynchronisation, BASIC-Steuerung und
JSON-Import/-Export. Pro Hauptschleife werden hoechstens 32 USB-Zeichen und ein
Befehl bearbeitet; die Ausgabe wartet maximal 2 ms. Die Reaktionszeit bei
gleichzeitiger Touchbedienung steht noch als Hardwaretest aus.

## Phase 11: BASIC-Programmiermodus

- [x] Einen zeilennummernbasierten BASIC-Interpreter bereitstellen.
- [x] `PRINT`, `LET`, `INPUT`, `IF/THEN`, `GOTO`, `FOR/TO/STEP`, `NEXT`,
  `CLS`, `REM` und `END` implementieren.
- [x] Eine QWERTZ-Tastatur fuer freie Zeicheneingabe integrieren.
- [x] Eine eigene Token-Ebene fuer BASIC-Woerter und Operatoren anbieten.
- [x] Programmliste, Eingabe und Programmausgabe auf dem LCD darstellen.
- [x] Laufende Programme in begrenzten Paketen ausfuehren und Endlosschleifen
  sicher abbrechen.
- [x] Programme im versionierten Flashformat persistent speichern.
- [x] Programm, Ausgabe, Start/Stop und `INPUT` ueber USB und Python-GUI
  bedienen.

**Fertig, wenn:** Programme vollstaendig per Touch eingegeben, bearbeitet,
ausgefuehrt und nach einem Neustart wieder geladen werden koennen.

Softwarestand: 20 Programmzeilen, 26 Variablen, 16 gepufferte Ausgabezeilen,
eine QWERTZ- und eine dedizierte `TOK`-Tastatur sind implementiert. Die
CODE-Tastatur folgt der dreistufigen K2-Umschaltung; im Vollbild stehen alle
16 Ausgabezeilen zur Verfuegung. Die Python-GUI verwaltet `.bas`-Dateien,
Geraetesynchronisation, Run/Stop, Ausgabe und `INPUT`. Das Flashformat Version
4 migriert bestehende Version-1- bis Version-3-Daten. Alle 29 Host-Tests und
der RP2040-Release-Build laufen erfolgreich. Touchpositionen, Lesbarkeit,
`INPUT` und Wiederherstellung nach einem Neustart stehen noch als Hardwaretest
aus.

## Phase 12: Exakte Dezimalarithmetik

- [x] Einen begrenzten Multipraezisions-Dezimalkern implementieren.
- [x] Addition, Subtraktion, Multiplikation, Division, Modulo und ganzzahlige
  Potenzen mit Vorzeichen und Klammern unterstuetzen.
- [x] Exaktes `ANS` in Anzeige, Verlauf und USB-Protokoll weiterreichen.
- [x] Dezimalwerte BCD-komprimiert im versionierten Flashformat speichern.
- [x] Periodische Divisionen reproduzierbar mit Round-to-even runden.
- [x] Wissenschaftliche Funktionen kontrolliert auf TinyExpr/`double`
  zurueckfallen lassen.
- [x] Praezision, Rundung, Fehlerfaelle, Migration und USB mit Host-Tests
  abdecken.

**Fertig, wenn:** Reine Dezimalrechnungen bis 80 Stellen ohne unbemerkten
Genauigkeitsverlust funktionieren und exakte Ergebnisse ueber `ANS`, Verlauf,
USB und Neustarts erhalten bleiben.

Softwarestand: Der hybride Rechenkern verarbeitet Dezimalarithmetik mit bis zu
80 Stellen. Endliche Divisionen sind exakt, periodische Ergebnisse werden nach
80 Stellen gerundet und gekennzeichnet. Funktionen, Variablen, Statistik,
Graphen und BASIC-Zahlen bleiben fuer Geschwindigkeit und Kompatibilitaet
`double`-basiert.
Firmware 1.4.0, USB-Protokoll 3 und Flashformat 4 transportieren die neuen
Ergebnisstrings. Der Hardwaretest von Anzeige und Neustart steht noch aus.

## Arbeitsweise pro Phase

- [ ] Umfang und Bedienablauf vor der Implementierung festlegen.
- [ ] Hardwareunabhaengige Logik mit Unit-Tests entwickeln.
- [ ] Touchoberflaeche fuer Landscape und Portrait implementieren.
- [ ] Host-Tests und beide Firmware-Ziele bauen.
- [ ] Touch, Lesbarkeit und Reaktionszeit auf dem Board pruefen.
- [ ] README und Roadmap aktualisieren.
- [ ] Phase in einem eigenen Git-Commit abschliessen.
