# Contributing

Danke fuer dein Interesse an diesem Projekt.

## Ablauf

1. Erstelle einen kleinen, thematisch klaren Branch.
2. Halte Hardwaretreiber in `lib/board` und anwendungsspezifischen Code unter
   `firmware/<anwendung>`.
3. Fuege fuer Aenderungen am Rechenkern passende Tests hinzu.
4. Baue beide Firmware-Ziele und fuehre alle Host-Tests aus.
5. Beschreibe im Pull Request getestete Hardware und sichtbare UI-Aenderungen.

## Vor einem Pull Request

```sh
cmake -S tests -B out/tests
cmake --build out/tests
ctest --test-dir out/tests --output-on-failure
```

Firmware-Aenderungen muessen ausserdem mit dem Pico SDK konfiguriert und gebaut
werden. Hardwaretests lassen sich nicht vollstaendig durch CI ersetzen; bitte
nenne deshalb das verwendete Board und Display.
