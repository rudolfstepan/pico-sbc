# LAFVIN Pico Development Kit – Hardwarebeschreibung

## 1. Überblick

Das **LAFVIN Pico Development Kit** ist ein Trägerboard für Raspberry-Pi-Pico-kompatible Mikrocontrollerboards. Es kombiniert einen Raspberry Pi Pico mit einem 3,5-Zoll-TFT-Display, kapazitivem Touchscreen, Joystick, Tasten, LEDs und einem Buzzer.

Das Board eignet sich insbesondere für:

- eigenständige Embedded-Anwendungen
- grafische Benutzeroberflächen
- kleine Terminal- oder Mini-Computer-Projekte
- Spiele und Menüsysteme
- Steuerungs- und Messgeräte

Auf dem vorliegenden Board ist ein **Raspberry Pi Pico mit RP2040** montiert. Es handelt sich damit um ein Mikrocontrollersystem und nicht um einen klassischen Linux-Einplatinencomputer.

---

## 2. Zentrale Recheneinheit

| Eigenschaft | Wert |
|---|---:|
| Mikrocontroller | RP2040 |
| CPU | 2 × Arm Cortex-M0+ |
| Maximale Taktfrequenz | 133 MHz |
| SRAM | 264 KiB |
| Flash-Speicher des Pico | 2 MiB QSPI-Flash |
| GPIOs des Pico | 26 Multifunktions-GPIOs |
| ADC | 3 × 12-Bit-ADC |
| UART | 2 |
| SPI | 2 |
| I²C | 2 |
| PWM | 16 Kanäle |
| PIO | 8 State Machines |
| USB | USB 1.1, Device- und Host-Unterstützung im RP2040 |

Der RP2040 besitzt keine MMU und keinen externen Hauptspeicher auf dem Kit. Die Firmware läuft direkt auf dem Mikrocontroller, beispielsweise mit dem Pico C/C++ SDK, FreeRTOS oder MicroPython.

---

## 3. Display und kapazitiver Touchscreen

Das Board enthält ein **3,5-Zoll-TFT-LCD mit kapazitiver Touchschicht**.

| Eigenschaft | Wert |
|---|---:|
| Auflösung | 320 × 480 Pixel |
| Displaycontroller | ST7796U |
| Farbdarstellung | üblicherweise RGB565, 16 Bit pro Pixel |
| Display-Schnittstelle | SPI0 |
| Touchcontroller | GT911 |
| Touch-Technik | kapazitiv |

Die Rechnerfirmware kann den ST7796U zur Laufzeit zwischen Landscape
(`480 x 320`) und Portrait (`320 x 480`) umschalten. Der GT911-Treiber bildet
die Rohkoordinaten passend auf die jeweils aktive Orientierung ab.
| Touch-Schnittstelle | I²C0 |
| Touch-Unterstützung | Multi-Touch laut Hersteller |

Wichtig ist die begriffliche Trennung:

- Das **LCD** erzeugt das Bild.
- Die darüberliegende **kapazitive Touchschicht** erfasst Berührungen.
- Der ST7796U steuert das Display.
- Der GT911 verarbeitet die Touchkoordinaten.

### Speicherbedarf eines Vollbild-Framebuffers

Ein vollständiger RGB565-Framebuffer benötigt:

```text
320 × 480 × 2 Byte = 307.200 Byte
```

Damit ist ein vollständiger 16-Bit-Framebuffer größer als der gesamte SRAM des RP2040. Für eine effiziente Firmware sind daher folgende Verfahren sinnvoll:

- zeilenweises Rendering
- Kachel- oder Tile-Rendering
- Dirty-Rectangle-Updates
- kleine DMA-Übertragungspuffer
- Zeichenpuffer für Terminaldarstellung

---

## 4. Display- und Touch-Pinbelegung

### TFT-Display ST7796U

| Pico-GPIO | Funktion |
|---:|---|
| GP2 | SPI0 SCLK |
| GP3 | SPI0 MOSI |
| GP4 | SPI0 MISO |
| GP5 | LCD Chip Select |
| GP6 | LCD Data/Command |
| GP7 | LCD Reset |

### Kapazitiver Touchcontroller GT911

| Pico-GPIO | Funktion |
|---:|---|
| GP8 | I²C0 SDA |
| GP9 | I²C0 SCL |
| GP10 | Touch Reset |
| GP11 | Touch Interrupt |

Bei Querformatdarstellung müssen die vom Touchcontroller gelieferten Koordinaten entsprechend gedreht und auf die Displaykoordinaten abgebildet werden.

---

## 5. Bedienelemente

### Analoger Joystick

Das Board besitzt einen zweiachsigen Mini-Joystick im PSP-Stil.

| Pico-GPIO | Funktion |
|---:|---|
| GP26 / ADC0 | X-Achse |
| GP27 / ADC1 | Y-Achse |

Die beiden Achsen liefern analoge Spannungswerte. Für eine zuverlässige Auswertung sollten Mittelpunkt, Totzone und Maximalwerte beim Start kalibriert werden.

### Tasten

| Bezeichnung | Pico-GPIO |
|---|---:|
| K1 / BTN1 | GP15 |
| K2 / BTN2 | GP14 |

Die Tasten können beispielsweise für `Enter`, `Escape`, Menüfunktionen oder Systemaktionen verwendet werden.

---

## 6. Ausgabe- und Signalkomponenten

### RGB-LED

| Komponente | Pico-GPIO |
|---|---:|
| WS2812-RGB-LED | GP12 |

Die RGB-LED besitzt einen integrierten seriellen Controller. Sie kann über ein zeitkritisches GPIO-Signal oder komfortabel über eine PIO-State-Machine angesteuert werden.

### Buzzer

| Komponente | Pico-GPIO |
|---|---:|
| Buzzer | GP13 |

Der Buzzer kann per PWM angesteuert werden und eignet sich für Tastenfeedback, Signaltöne und einfache Tonfolgen.

### Status-LEDs

| LED | Zuordnung |
|---|---|
| D1 | GP16 |
| D2 | GP17 |
| D3 | 3,3-V-Versorgungsanzeige |
| D4 | 5-V-Versorgungsanzeige |

---

## 7. Zusammengefasste GPIO-Belegung

| GPIO | Verwendung |
|---:|---|
| GP0 | frei, typischerweise UART0 TX |
| GP1 | frei, typischerweise UART0 RX |
| GP2 | LCD SCLK |
| GP3 | LCD MOSI |
| GP4 | LCD MISO |
| GP5 | LCD CS |
| GP6 | LCD DC |
| GP7 | LCD Reset |
| GP8 | Touch SDA |
| GP9 | Touch SCL |
| GP10 | Touch Reset |
| GP11 | Touch Interrupt |
| GP12 | WS2812 RGB-LED |
| GP13 | Buzzer |
| GP14 | Taste K2 |
| GP15 | Taste K1 |
| GP16 | LED D1 |
| GP17 | LED D2 |
| GP18 | frei |
| GP19 | frei |
| GP20 | frei |
| GP21 | frei |
| GP22 | frei |
| GP26 | Joystick X / ADC0 |
| GP27 | Joystick Y / ADC1 |
| GP28 | frei / ADC2 |

Die als frei markierten Pins sind nach der veröffentlichten Standardbelegung nicht durch die integrierten Komponenten belegt. Vor dem Anschluss zusätzlicher Hardware sollte dennoch die konkrete Boardrevision beziehungsweise ein Schaltplan geprüft werden.

---

## 8. Erweiterungs- und Anschlussmöglichkeiten

Das Carrier-Board führt zahlreiche Pico-Pins an seitlichen Stiftleisten heraus. Dadurch lassen sich zusätzliche Komponenten anbinden, beispielsweise:

- PS/2-Tastatur
- UART-Geräte
- I²C-Sensoren
- SPI-Speicher oder SD-Kartenleser
- RTC-Modul
- externe Audio-Hardware
- zusätzliche Tasten und LEDs

Für einen Mini-Computer sind besonders GP0/GP1 als UART sowie GP18 bis GP22 und GP28 als Erweiterungspins interessant.

Zu beachten ist, dass sich zusätzliche Geräte einen vorhandenen SPI- oder I²C-Bus teilen können, jedoch jeweils eigene Chip-Select-Leitungen beziehungsweise eindeutige I²C-Adressen benötigen.

---

## 9. Stromversorgung

Das Kit wird normalerweise über den USB-Anschluss des Raspberry Pi Pico mit 5 V versorgt. Das Board stellt außerdem 5 V, 3,3 V und GND an beschrifteten Anschlüssen bereit.

Laut Hersteller liegt die typische Stromaufnahme des Gesamtsystems abhängig von Displayhelligkeit und Nutzung ungefähr im Bereich von 200 bis 500 mA.

Bei Erweiterungen sind insbesondere folgende Punkte zu beachten:

- Pico-GPIOs arbeiten mit 3,3-V-Logik.
- GPIO-Eingänge sind nicht 5-V-tolerant.
- Externe Verbraucher dürfen den 3,3-V-Regler nicht überlasten.
- Größere Lasten sollten eine eigene Stromversorgung erhalten und eine gemeinsame Masse verwenden.

---

## 10. Relevante Einschränkungen für einen Mini-Computer

Das Board ist gut für einen kleinen eigenständigen Computer geeignet, besitzt jedoch einige architekturelle Grenzen:

- nur 264 KiB SRAM
- kein externer RAM auf dem Carrier-Board
- kein integrierter Massenspeicher außer dem Pico-Flash
- kein hardwareseitiges Betriebssystem oder BIOS
- kein MMU-basierter Linux-Betrieb
- SPI-Display statt direkt angebundenem Video-Framebuffer
- im abgebildeten Pico keine integrierte Funkverbindung

Daraus ergibt sich eine sinnvolle Zielarchitektur mit:

- eigener Firmware
- Terminal- oder Fenstersystem
- Touch-Bedienung
- kleinem Dateisystem im Flash oder auf externer SD-Karte
- kooperativem Scheduler oder FreeRTOS
- speichersparendem Kachelrenderer

---

## 11. Empfohlene Firmware-Basis

Für eine robuste Mini-Computer-Firmware bietet sich folgende Grundlage an:

- Raspberry Pi Pico C/C++ SDK
- C oder C++
- ST7796U-Treiber mit SPI und DMA
- GT911-Treiber über I²C und Interrupt
- eigener Terminalrenderer oder LVGL für grafische Oberflächen
- optional FreeRTOS für Tasks und Ereignisverarbeitung
- optional LittleFS im Flash oder FatFS auf SD-Karte

---

## 12. Quellen

1. LAFVIN, [*LAFVIN Pico Development Kit – About This Kit*](https://lafvin-pico-development-kit.readthedocs.io/en/latest/about_this_kit.html)

2. LAFVIN, [*LAFVIN-PICO-Development-Kit GitHub Repository*](https://github.com/lafvintech/LAFVIN-PICO-Development-Kit)

3. Raspberry Pi, [*Pico microcontroller boards – Documentation*](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)

4. Raspberry Pi, [*Raspberry Pi Pico – Technical specifications*](https://www.raspberrypi.com/products/raspberry-pi-pico/)

---

**Hinweis:** Die Dokumentation beschreibt die auf dem Foto sichtbare Bestückung mit einem originalen Raspberry Pi Pico sowie die vom Hersteller veröffentlichte Standardbelegung. Bei anderen Pico-Varianten oder abweichenden Boardrevisionen müssen CPU-, Speicher- und Funkmerkmale erneut geprüft werden.
