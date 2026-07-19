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
| 5 | Programmer-Erweiterungen | Phase 0 | geplant |
| 6 | Einheiten und Konstanten | Phase 3 | geplant |
| 7 | Komplexe Zahlen | Phase 3 | geplant |
| 8 | Statistikmodus | Phasen 2 und 3 | geplant |
| 9 | USB-Datenaustausch | Phasen 4 und 8 | geplant |

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

- [ ] Zwischen Signed- und Unsigned-Darstellung umschalten.
- [ ] Einzelne Bits anzeigen, setzen, loeschen und umschalten.
- [ ] Rotate Left und Rotate Right fuer alle Wortbreiten implementieren.
- [ ] Little-Endian- und Big-Endian-Umwandlung anbieten.
- [ ] Einen IEEE-754-Inspector fuer Vorzeichen, Exponent und Mantisse bauen.
- [ ] Ueberlauf- und Carry-Informationen sichtbar ausgeben.

**Fertig, wenn:** Alle Operationen fuer 8, 16, 32 und 64 Bit getestet sind und
die Bitansicht auf dem LCD ohne horizontales Abschneiden funktioniert.

## Phase 6: Einheiten und Konstanten

- [ ] Kategorien fuer Laenge, Flaeche, Volumen, Masse und Zeit anlegen.
- [ ] Temperatur, Winkel, Druck, Energie und Leistung ergaenzen.
- [ ] Umrechnungen ueber eine gemeinsame Basiseinheit modellieren.
- [ ] Physikalische Konstanten mit Einheit und Quellenhinweis hinterlegen.
- [ ] Ergebnisse direkt an `ANS` oder den Ausdruckseditor uebergeben.

**Fertig, wenn:** Hin- und Rueckumwandlungen innerhalb definierter Toleranzen
getestet sind und inkompatible Einheiten nicht kombiniert werden.

## Phase 7: Komplexe Zahlen

- [ ] Einen komplexen Zahlentyp fuer den Rechenkern einfuehren.
- [ ] Eingaben wie `3+4i` sowie Polarform unterstuetzen.
- [ ] Betrag, Phase, Konjugation und Grundrechenarten implementieren.
- [ ] Zwischen kartesischer und polarer Darstellung umschalten.
- [ ] Komplexe Ergebnisse eindeutig von reellen Fehlerwerten unterscheiden.

**Fertig, wenn:** Parser, Anzeige und Verlauf komplexe Zahlen verlustfrei
verarbeiten und die wichtigsten Operationen durch Host-Tests abgesichert sind.

## Phase 8: Statistikmodus

- [ ] Einen touchfreundlichen Editor fuer Zahlenlisten erstellen.
- [ ] Mittelwert, Median, Minimum, Maximum und Standardabweichung berechnen.
- [ ] Ein- und Zwei-Variablen-Statistik unterstuetzen.
- [ ] Lineare Regression mit Korrelationskoeffizient implementieren.
- [ ] Histogramm und Streudiagramm im Graphbereich darstellen.
- [ ] Listen in Verlauf und persistente Speicherung integrieren.

**Fertig, wenn:** Listen bearbeitet, ausgewertet und grafisch dargestellt
werden koennen und Ergebnisse gegen bekannte Datensaetze getestet sind.

## Phase 9: USB-Datenaustausch

- [ ] Ein einfaches, dokumentiertes USB-CDC-Befehlsformat definieren.
- [ ] Ausdruecke, Variablen, Funktionen und Wertelisten importieren.
- [ ] Ergebnisse, Verlauf und Statistikdaten exportieren.
- [ ] Diagnoseausgaben und Firmwareinformationen abrufbar machen.
- [ ] Eingaben begrenzen und ungueltige Befehle ohne Absturz ablehnen.
- [ ] Ein kleines plattformunabhaengiges Kommandozeilenwerkzeug bereitstellen.

**Fertig, wenn:** Daten zwischen Rechner und PC in beide Richtungen uebertragen
werden koennen, ohne Touchbedienung oder Berechnungen zu blockieren.

## Arbeitsweise pro Phase

- [ ] Umfang und Bedienablauf vor der Implementierung festlegen.
- [ ] Hardwareunabhaengige Logik mit Unit-Tests entwickeln.
- [ ] Touchoberflaeche fuer 480 x 320 Pixel implementieren.
- [ ] Host-Tests und beide Firmware-Ziele bauen.
- [ ] Touch, Lesbarkeit und Reaktionszeit auf dem Board pruefen.
- [ ] README und Roadmap aktualisieren.
- [ ] Phase in einem eigenen Git-Commit abschliessen.
