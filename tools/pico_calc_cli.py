#!/usr/bin/env python3
"""Command-line client for the pico-sbc scientific calculator USB protocol."""

from __future__ import annotations

import argparse
import json
import math
import sys
import time
from pathlib import Path
from typing import Any

LINE_CAPACITY = 192


class ProtocolError(RuntimeError):
    pass


class SerialClient:
    def __init__(self, port: str, baudrate: int, timeout: float) -> None:
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial = None

    @property
    def connected(self) -> bool:
        return self.serial is not None and self.serial.is_open

    def open(self) -> "SerialClient":
        if self.connected:
            return self
        try:
            import serial
        except ImportError as error:
            raise ProtocolError(
                "pyserial fehlt; installieren mit: "
                "python -m pip install -r tools/requirements.txt"
            ) from error
        self.serial = serial.Serial(
            self.port, self.baudrate, timeout=self.timeout,
            write_timeout=self.timeout,
        )
        time.sleep(0.15)
        self.serial.reset_input_buffer()
        return self

    def close(self) -> None:
        if self.serial is not None:
            self.serial.close()
            self.serial = None

    def __enter__(self) -> "SerialClient":
        return self.open()

    def __exit__(self, exc_type: Any, exc: Any, traceback: Any) -> None:
        self.close()

    def command(self, command: str) -> str:
        if any(ord(character) < 0x20 or ord(character) > 0x7E
               for character in command):
            raise ProtocolError("Befehl enthaelt ungueltige Zeichen")
        encoded = command.encode("ascii")
        if len(encoded) >= LINE_CAPACITY:
            raise ProtocolError("Befehl ist laenger als 191 Zeichen")
        assert self.serial is not None
        self.serial.write(encoded + b"\n")
        self.serial.flush()
        response = self.serial.readline()
        if not response:
            raise ProtocolError("Keine Antwort vom Pico")
        decoded = response.decode("ascii", errors="strict").strip()
        if decoded.startswith("ERR ") or decoded == "ERR":
            raise ProtocolError(decoded)
        if not decoded.startswith("OK"):
            raise ProtocolError(f"Ungueltige Antwort: {decoded}")
        return decoded


def fields(response: str, expected: str) -> list[str]:
    parts = response.split("\t")
    if not parts or parts[0] != f"OK {expected}":
        raise ProtocolError(f"Erwartet OK {expected}, erhalten: {response}")
    return parts[1:]


def export_state(client: Any) -> dict[str, Any]:
    result_fields = fields(client.command("GET RESULT"), "RESULT")
    expression_fields = fields(client.command("GET EXPR"), "EXPR")
    variables: dict[str, float] = {}
    for name in "ABCDEF":
        item = fields(client.command(f"GET VAR {name}"), "VAR")
        variables[item[0]] = float(item[1])
    functions: dict[str, str] = {}
    for number in range(1, 4):
        item = fields(client.command(f"GET FUNC F{number}"), "FUNC")
        functions[item[0]] = item[1] if len(item) > 1 else ""

    history_meta = fields(client.command("GET HISTORY"), "HISTORY")
    history = []
    for index in range(int(history_meta[0])):
        item = fields(client.command(f"GET HISTORY {index}"), "HISTORY")
        history.append({
            "index": int(item[0]),
            "value": float(item[1]),
            "expression": item[2],
            "result": item[3],
        })

    stats_meta = fields(client.command("GET STATS"), "STATS")
    mode = int(stats_meta[0])
    values = []
    for index in range(int(stats_meta[1])):
        item = fields(client.command(f"GET STATS {index}"), "STATS")
        row = [float(item[1])]
        if mode == 2:
            row.append(float(item[2]))
        values.append(row)

    return {
        "format": "pico-sbc-calculator-state",
        "version": 1,
        "result": float(result_fields[0]),
        "expression": expression_fields[0] if expression_fields else "",
        "variables": variables,
        "functions": functions,
        "history": history,
        "statistics": {"mode": mode, "values": values},
    }


def import_state(client: Any, data: dict[str, Any]) -> None:
    if data.get("format") not in (None, "pico-sbc-calculator-state"):
        raise ProtocolError("Unbekanntes JSON-Format")
    if "expression" in data:
        expression = str(data["expression"])
        client.command(f"SET EXPR {expression}")

    variables = data.get("variables", {})
    if not isinstance(variables, dict):
        raise ProtocolError("variables muss ein Objekt sein")
    normalized_variables = []
    for name, value in variables.items():
        normalized = str(name).upper()
        if normalized not in tuple("ABCDEF"):
            raise ProtocolError(f"Ungueltige Variable: {name}")
        number = float(value)
        if not math.isfinite(number):
            raise ProtocolError(f"Ungueltiger Variablenwert: {name}")
        normalized_variables.append((normalized, number))
    for name, value in normalized_variables:
        client.command(f"SET VAR {name} {value:.17g}")

    functions = data.get("functions", {})
    if not isinstance(functions, dict):
        raise ProtocolError("functions muss ein Objekt sein")
    pending_functions = []
    for name, expression_value in functions.items():
        normalized = str(name).upper()
        if normalized not in ("F1", "F2", "F3"):
            raise ProtocolError(f"Ungueltige Funktion: {name}")
        pending_functions.append((normalized, str(expression_value)))

    last_error = None
    while pending_functions:
        remaining = []
        imported = 0
        for name, expression_value in pending_functions:
            try:
                client.command(f"SET FUNC {name} {expression_value}")
                imported += 1
            except ProtocolError as error:
                last_error = error
                remaining.append((name, expression_value))
        if imported == 0:
            raise ProtocolError(
                f"Funktionsimport fehlgeschlagen: {last_error}"
            ) from last_error
        pending_functions = remaining

    statistics = data.get("statistics")
    if statistics is None:
        return
    if not isinstance(statistics, dict):
        raise ProtocolError("statistics muss ein Objekt sein")
    mode = int(statistics.get("mode", 1))
    if mode not in (1, 2):
        raise ProtocolError("Statistikmodus muss 1 oder 2 sein")
    values = statistics.get("values", [])
    if not isinstance(values, list) or len(values) > 32:
        raise ProtocolError("Statistikliste muss maximal 32 Zeilen enthalten")
    normalized_rows = []
    for row in values:
        if not isinstance(row, list) or len(row) != mode:
            raise ProtocolError(f"Statistikzeile passt nicht zu Modus {mode}")
        numbers = [float(value) for value in row]
        if not all(math.isfinite(value) for value in numbers):
            raise ProtocolError("Statistikwerte muessen endlich sein")
        normalized_rows.append(numbers)
    client.command(f"STAT MODE {mode}")
    client.command("STAT CLEAR")
    for row in normalized_rows:
        numbers = " ".join(f"{float(value):.17g}" for value in row)
        client.command(f"STAT ADD {numbers}")


def list_ports() -> int:
    try:
        from serial.tools import list_ports as serial_list_ports
    except ImportError as error:
        raise ProtocolError(
            "pyserial fehlt; installieren mit: "
            "python -m pip install -r tools/requirements.txt"
        ) from error
    ports = list(serial_list_ports.comports())
    if not ports:
        print("Keine seriellen Ports gefunden")
        return 1
    for port in ports:
        print(f"{port.device}\t{port.description}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="USB-CLI fuer den pico-sbc Scientific Calculator"
    )
    parser.add_argument("--port", help="Serieller Port, z.B. COM5 oder /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=2.0)
    subparsers = parser.add_subparsers(dest="action", required=True)
    subparsers.add_parser("ports", help="Verfuegbare serielle Ports anzeigen")
    raw = subparsers.add_parser("command", help="Einen rohen Protokollbefehl senden")
    raw.add_argument("line")
    evaluation = subparsers.add_parser("eval", help="Ausdruck auswerten")
    evaluation.add_argument("expression")
    subparsers.add_parser("info", help="Firmwareinformationen lesen")
    subparsers.add_parser("diag", help="Diagnosezustand lesen")
    export_parser = subparsers.add_parser("export", help="Zustand als JSON exportieren")
    export_parser.add_argument("path", type=Path)
    import_parser = subparsers.add_parser("import", help="JSON-Daten importieren")
    import_parser.add_argument("path", type=Path)
    return parser


def main(argv: list[str] | None = None) -> int:
    arguments = build_parser().parse_args(argv)
    try:
        if arguments.action == "ports":
            return list_ports()
        if not arguments.port:
            raise ProtocolError("--port ist fuer diesen Befehl erforderlich")
        with SerialClient(arguments.port, arguments.baud, arguments.timeout) as client:
            if arguments.action == "command":
                print(client.command(arguments.line))
            elif arguments.action == "eval":
                print(client.command(f"EVAL {arguments.expression}"))
            elif arguments.action == "info":
                print(client.command("INFO"))
            elif arguments.action == "diag":
                print(client.command("DIAG"))
            elif arguments.action == "export":
                state = export_state(client)
                arguments.path.write_text(
                    json.dumps(state, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8",
                )
                print(f"OK exportiert: {arguments.path}")
            elif arguments.action == "import":
                state = json.loads(arguments.path.read_text(encoding="utf-8"))
                if not isinstance(state, dict):
                    raise ProtocolError("JSON-Wurzel muss ein Objekt sein")
                import_state(client, state)
                print(f"OK importiert: {arguments.path}")
        return 0
    except (OSError, ValueError, json.JSONDecodeError, ProtocolError) as error:
        print(f"Fehler: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
