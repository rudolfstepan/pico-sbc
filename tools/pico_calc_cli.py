#!/usr/bin/env python3
"""Command-line client for the pico-sbc scientific calculator USB protocol."""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
import time
from pathlib import Path
from typing import Any

LINE_CAPACITY = 256
BASIC_MAX_LINES = 20
BASIC_LINE_TEXT_CAPACITY = 64
CIRCUIT_NODE_CAPACITY = 24
CIRCUIT_WIRE_CAPACITY = 48
CIRCUIT_WORLD_WIDTH = 1600
CIRCUIT_WORLD_HEIGHT = 1200
CIRCUIT_ZOOM_LEVELS = (100, 150, 200)
CIRCUIT_GATE_TYPES = (
    "INPUT", "OUTPUT", "NOT", "AND", "OR", "XOR", "NAND", "NOR",
    "IMPLIES", "XNOR",
)
CIRCUIT_GATE_INPUTS = {
    "INPUT": 0,
    "OUTPUT": 1,
    "NOT": 1,
    "AND": 2,
    "OR": 2,
    "XOR": 2,
    "NAND": 2,
    "NOR": 2,
    "IMPLIES": 2,
    "XNOR": 2,
}


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
            raise ProtocolError("Befehl ist laenger als 255 Zeichen")
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


def property_fields(response: str, expected: str) -> dict[str, str]:
    properties: dict[str, str] = {}
    for field in fields(response, expected):
        if "=" not in field:
            raise ProtocolError(f"Ungueltiges {expected}-Feld: {field}")
        name, value = field.split("=", 1)
        if not name or name in properties:
            raise ProtocolError(f"Ungueltiges {expected}-Feld: {field}")
        properties[name] = value
    return properties


def _circuit_boolean(value: Any, field: str) -> bool:
    if isinstance(value, bool):
        return value
    if value in (0, "0"):
        return False
    if value in (1, "1"):
        return True
    raise ProtocolError(f"{field} muss 0 oder 1 sein")


def _circuit_integer(value: Any, field: str) -> int:
    try:
        parsed = int(value)
    except (TypeError, ValueError) as error:
        raise ProtocolError(f"{field} muss eine Ganzzahl sein") from error
    if isinstance(value, float) and not value.is_integer():
        raise ProtocolError(f"{field} muss eine Ganzzahl sein")
    return parsed


def normalize_circuit(data: Any) -> dict[str, Any]:
    """Validate a circuit and return a canonical, evaluated representation."""
    if not isinstance(data, dict):
        raise ProtocolError("circuit muss ein Objekt sein")

    world_width = _circuit_integer(
        data.get("world_width", CIRCUIT_WORLD_WIDTH), "world_width")
    world_height = _circuit_integer(
        data.get("world_height", CIRCUIT_WORLD_HEIGHT), "world_height")
    if world_width != CIRCUIT_WORLD_WIDTH or world_height != CIRCUIT_WORLD_HEIGHT:
        raise ProtocolError("Ungueltige Schaltplanflaeche")

    raw_nodes = data.get("nodes", [])
    if not isinstance(raw_nodes, list) or len(raw_nodes) > CIRCUIT_NODE_CAPACITY:
        raise ProtocolError("Schaltplan darf maximal 24 Knoten enthalten")
    nodes: list[dict[str, Any]] = []
    node_ids: set[int] = set()
    for fallback_id, raw_node in enumerate(raw_nodes):
        if not isinstance(raw_node, dict):
            raise ProtocolError("Jeder Schaltplanknoten muss ein Objekt sein")
        node_id = _circuit_integer(raw_node.get("id", fallback_id), "node.id")
        gate_type = str(raw_node.get("type", "")).upper()
        x = _circuit_integer(raw_node.get("x", 0), "node.x")
        y = _circuit_integer(raw_node.get("y", 0), "node.y")
        input_value = _circuit_boolean(raw_node.get("input", 0), "node.input")
        label = str(raw_node.get("label", ""))
        if node_id < 0 or node_id >= CIRCUIT_NODE_CAPACITY or node_id in node_ids:
            raise ProtocolError("Ungueltige oder doppelte Schaltplan-Knoten-ID")
        if gate_type not in CIRCUIT_GATE_TYPES:
            raise ProtocolError(f"Unbekannter Gattertyp: {gate_type}")
        if not 0 <= x <= world_width or not 0 <= y <= world_height:
            raise ProtocolError("Schaltplanknoten liegt ausserhalb der Flaeche")
        try:
            encoded_label = label.encode("ascii")
        except UnicodeEncodeError as error:
            raise ProtocolError("Gatterbezeichnung muss ASCII sein") from error
        if (not re.fullmatch(r"[A-Za-z0-9_-]{1,7}", label) or
                len(encoded_label) > 7):
            raise ProtocolError("Ungueltige Gatterbezeichnung")
        node_ids.add(node_id)
        nodes.append({
            "id": node_id,
            "type": gate_type,
            "x": x,
            "y": y,
            "input": input_value,
            "output": False,
            "label": label,
        })
    nodes.sort(key=lambda node: node["id"])
    nodes_by_id = {node["id"]: node for node in nodes}

    raw_wires = data.get("wires", [])
    if not isinstance(raw_wires, list) or len(raw_wires) > CIRCUIT_WIRE_CAPACITY:
        raise ProtocolError("Schaltplan darf maximal 48 Leitungen enthalten")
    wires: list[dict[str, int]] = []
    wire_ids: set[int] = set()
    occupied_inputs: set[tuple[int, int]] = set()
    for fallback_id, raw_wire in enumerate(raw_wires):
        if not isinstance(raw_wire, dict):
            raise ProtocolError("Jede Schaltplanleitung muss ein Objekt sein")
        wire_id = _circuit_integer(raw_wire.get("id", fallback_id), "wire.id")
        source = _circuit_integer(raw_wire.get("source"), "wire.source")
        destination = _circuit_integer(
            raw_wire.get("destination"), "wire.destination")
        destination_input = _circuit_integer(
            raw_wire.get("input", 0), "wire.input")
        if wire_id < 0 or wire_id >= CIRCUIT_WIRE_CAPACITY or wire_id in wire_ids:
            raise ProtocolError("Ungueltige oder doppelte Leitungs-ID")
        if source not in nodes_by_id or destination not in nodes_by_id:
            raise ProtocolError("Leitung verweist auf unbekannten Knoten")
        if source == destination or nodes_by_id[source]["type"] == "OUTPUT":
            raise ProtocolError("Ungueltige Leitungsquelle")
        input_count = CIRCUIT_GATE_INPUTS[nodes_by_id[destination]["type"]]
        input_key = (destination, destination_input)
        if not 0 <= destination_input < input_count:
            raise ProtocolError("Leitung verweist auf ungueltigen Eingang")
        if input_key in occupied_inputs:
            raise ProtocolError("Ein Gattereingang darf nur eine Leitung haben")
        wire_ids.add(wire_id)
        occupied_inputs.add(input_key)
        wires.append({
            "id": wire_id,
            "source": source,
            "destination": destination,
            "input": destination_input,
        })
    wires.sort(key=lambda wire: wire["id"])

    wires_by_input = {
        (wire["destination"], wire["input"]): wire for wire in wires
    }
    visit_state: dict[int, int] = {}

    def evaluate(node_id: int) -> bool:
        state = visit_state.get(node_id, 0)
        if state == 1:
            raise ProtocolError("Schaltplan enthaelt einen Zyklus")
        if state == 2:
            return bool(nodes_by_id[node_id]["output"])
        visit_state[node_id] = 1
        node = nodes_by_id[node_id]
        inputs = [False, False]
        for input_index in range(CIRCUIT_GATE_INPUTS[node["type"]]):
            wire = wires_by_input.get((node_id, input_index))
            if wire is not None:
                inputs[input_index] = evaluate(wire["source"])
        gate_type = node["type"]
        if gate_type == "INPUT":
            output = node["input"]
        elif gate_type == "OUTPUT":
            output = inputs[0]
        elif gate_type == "NOT":
            output = not inputs[0]
        elif gate_type == "AND":
            output = inputs[0] and inputs[1]
        elif gate_type == "OR":
            output = inputs[0] or inputs[1]
        elif gate_type == "XOR":
            output = inputs[0] != inputs[1]
        elif gate_type == "NAND":
            output = not (inputs[0] and inputs[1])
        elif gate_type == "NOR":
            output = not (inputs[0] or inputs[1])
        elif gate_type == "IMPLIES":
            output = not inputs[0] or inputs[1]
        else:
            output = inputs[0] == inputs[1]
        node["output"] = bool(output)
        visit_state[node_id] = 2
        return bool(output)

    for node in nodes:
        evaluate(node["id"])

    viewport_x = _circuit_integer(data.get("viewport_x", 0), "viewport_x")
    viewport_y = _circuit_integer(data.get("viewport_y", 0), "viewport_y")
    zoom = _circuit_integer(data.get("zoom", 150), "zoom")
    if not 0 <= viewport_x <= world_width or not 0 <= viewport_y <= world_height:
        raise ProtocolError("Ungueltiger Schaltplan-Ausschnitt")
    if zoom not in CIRCUIT_ZOOM_LEVELS:
        raise ProtocolError("Schaltplan-Zoom muss 100, 150 oder 200 sein")

    default_counters = {
        "next_input": sum(node["type"] == "INPUT" for node in nodes),
        "next_output": sum(node["type"] == "OUTPUT" for node in nodes),
        "next_gate": sum(node["type"] not in ("INPUT", "OUTPUT")
                         for node in nodes),
    }
    counters = {}
    for name, default in default_counters.items():
        counters[name] = _circuit_integer(data.get(name, default), name)
        if not default <= counters[name] <= 255:
            raise ProtocolError("Ungueltiger Schaltplan-Zaehler")

    return {
        "world_width": world_width,
        "world_height": world_height,
        "viewport_x": viewport_x,
        "viewport_y": viewport_y,
        "zoom": zoom,
        "next_input": counters["next_input"],
        "next_output": counters["next_output"],
        "next_gate": counters["next_gate"],
        "nodes": nodes,
        "wires": wires,
    }


def read_circuit(client: Any) -> dict[str, Any]:
    metadata = property_fields(
        client.command("MODULE CIRCUIT INFO"), "CIRCUIT_INFO")
    node_capacity = _circuit_integer(
        metadata.get("node_capacity"), "node_capacity")
    wire_capacity = _circuit_integer(
        metadata.get("wire_capacity"), "wire_capacity")
    if (node_capacity != CIRCUIT_NODE_CAPACITY or
            wire_capacity != CIRCUIT_WIRE_CAPACITY):
        raise ProtocolError("Nicht unterstuetzte Schaltplankapazitaet")

    nodes = []
    for index in range(node_capacity):
        item = property_fields(
            client.command(f"MODULE CIRCUIT NODE {index}"), "CIRCUIT_NODE")
        if _circuit_integer(item.get("index"), "node.index") != index:
            raise ProtocolError("Ungueltige CIRCUIT_NODE-Antwort")
        if not _circuit_boolean(item.get("used"), "node.used"):
            continue
        nodes.append({
            "id": index,
            "type": item.get("type", ""),
            "x": item.get("x"),
            "y": item.get("y"),
            "input": item.get("input"),
            "output": item.get("output"),
            "label": item.get("label", ""),
        })

    wires = []
    for index in range(wire_capacity):
        item = property_fields(
            client.command(f"MODULE CIRCUIT WIRE {index}"), "CIRCUIT_WIRE")
        if _circuit_integer(item.get("index"), "wire.index") != index:
            raise ProtocolError("Ungueltige CIRCUIT_WIRE-Antwort")
        if not _circuit_boolean(item.get("used"), "wire.used"):
            continue
        wires.append({
            "id": index,
            "source": item.get("source"),
            "destination": item.get("destination"),
            "input": item.get("input"),
        })

    return normalize_circuit({
        "world_width": metadata.get("world_width"),
        "world_height": metadata.get("world_height"),
        "viewport_x": metadata.get("viewport_x"),
        "viewport_y": metadata.get("viewport_y"),
        "zoom": metadata.get("zoom"),
        "next_input": metadata.get("next_input"),
        "next_output": metadata.get("next_output"),
        "next_gate": metadata.get("next_gate"),
        "nodes": nodes,
        "wires": wires,
    })


def synchronize_circuit(client: Any, data: Any) -> dict[str, Any]:
    circuit = normalize_circuit(data)
    client.command("MODULE CIRCUIT CLEAR")
    device_ids: dict[int, int] = {}
    for node in circuit["nodes"]:
        result = property_fields(client.command(
            "MODULE CIRCUIT ADD "
            f"{node['type']} {node['x']} {node['y']} "
            f"{1 if node['input'] else 0} {node['label']}"
        ), "CIRCUIT_NODE")
        device_ids[node["id"]] = _circuit_integer(
            result.get("index"), "node.index")
    for wire in circuit["wires"]:
        client.command(
            "MODULE CIRCUIT CONNECT "
            f"{device_ids[wire['source']]} {device_ids[wire['destination']]} "
            f"{wire['input']}"
        )
    client.command(
        "MODULE CIRCUIT COUNTERS "
        f"{circuit['next_input']} {circuit['next_output']} "
        f"{circuit['next_gate']}"
    )
    client.command(
        "MODULE CIRCUIT VIEW "
        f"{circuit['viewport_x']} {circuit['viewport_y']} {circuit['zoom']}"
    )
    return circuit


def normalize_basic_program(source: str | list[Any]) -> list[str]:
    if isinstance(source, list):
        raw_lines = [str(line) for line in source]
    elif isinstance(source, str):
        raw_lines = source.splitlines()
    else:
        raise ProtocolError("BASIC-Programm muss Text oder eine Zeilenliste sein")

    normalized: list[tuple[int, str]] = []
    numbers: set[int] = set()
    for raw_line in raw_lines:
        line = raw_line.strip()
        if not line:
            continue
        if any(ord(character) < 0x20 or ord(character) > 0x7E
               for character in line):
            raise ProtocolError("BASIC-Programm enthaelt ungueltige Zeichen")
        match = re.fullmatch(r"([0-9]+) +(.*\S)", line)
        if match is None:
            raise ProtocolError(f"Ungueltige BASIC-Zeile: {line}")
        number = int(match.group(1))
        statement = match.group(2)
        if number < 1 or number > 65535:
            raise ProtocolError(f"Ungueltige BASIC-Zeilennummer: {number}")
        if len(statement.encode("ascii")) >= BASIC_LINE_TEXT_CAPACITY:
            raise ProtocolError(f"BASIC-Zeile {number} ist zu lang")
        if number in numbers:
            raise ProtocolError(f"Doppelte BASIC-Zeilennummer: {number}")
        numbers.add(number)
        normalized.append((number, statement))
    if len(normalized) > BASIC_MAX_LINES:
        raise ProtocolError("BASIC-Programm darf maximal 20 Zeilen enthalten")
    normalized.sort(key=lambda item: item[0])
    return [f"{number} {statement}" for number, statement in normalized]


def read_basic_program(client: Any) -> list[str]:
    metadata = fields(client.command("GET BASIC"), "BASIC")
    if len(metadata) != 1:
        raise ProtocolError("Ungueltige BASIC-Antwort")
    count = int(metadata[0])
    if count < 0 or count > BASIC_MAX_LINES:
        raise ProtocolError("Ungueltige BASIC-Zeilenanzahl")
    program = []
    for index in range(count):
        item = fields(client.command(f"GET BASIC {index}"), "BASIC")
        if len(item) != 3 or int(item[0]) != index:
            raise ProtocolError("Ungueltige BASIC-Zeilenantwort")
        program.append(f"{int(item[1])} {item[2]}")
    return normalize_basic_program(program)


def synchronize_basic_program(client: Any,
                              source: str | list[Any]) -> list[str]:
    program = normalize_basic_program(source)
    client.command("BASIC CLEAR")
    for line in program:
        client.command(f"BASIC LINE {line}")
    return program


def read_basic_output(client: Any) -> list[str]:
    metadata = fields(client.command("GET BASIC OUTPUT"), "BASIC_OUTPUT")
    if len(metadata) != 1:
        raise ProtocolError("Ungueltige BASIC_OUTPUT-Antwort")
    count = int(metadata[0])
    if count < 0 or count > 6:
        raise ProtocolError("Ungueltige BASIC-Ausgabeanzahl")
    output = []
    for index in range(count):
        item = fields(client.command(f"GET BASIC OUTPUT {index}"),
                      "BASIC_OUTPUT")
        if len(item) < 1 or int(item[0]) != index:
            raise ProtocolError("Ungueltige BASIC-Ausgabezeile")
        output.append(item[1] if len(item) > 1 else "")
    return output


def export_state(client: Any) -> dict[str, Any]:
    result_fields = fields(client.command("GET RESULT"), "RESULT")
    expression_fields = fields(client.command("GET EXPR"), "EXPR")
    variables: dict[str, str] = {}
    for name in "ABCDEF":
        item = fields(client.command(f"GET VAR {name}"), "VAR")
        variables[item[0]] = item[1]
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

    program = read_basic_program(client)
    circuit = read_circuit(client)

    angle_fields = fields(client.command("GET ANGLE"), "ANGLE")
    precision_fields = fields(client.command("GET PRECISION"), "PRECISION")
    memory_fields = fields(client.command("GET MEMORY"), "MEMORY")
    programmer = property_fields(
        client.command("GET PROGRAMMER"), "PROGRAMMER_STATE")
    number_format = property_fields(
        client.command("GET FORMAT"), "FORMAT_STATE")
    graph = property_fields(client.command("GET GRAPH"), "GRAPH")
    favorites: dict[str, str] = {}
    for index in range(1, 7):
        item = fields(client.command(f"GET FAVORITE {index}"), "FAVORITE")
        if len(item) != 2 or int(item[0]) != index:
            raise ProtocolError("Ungueltige FAVORITE-Antwort")
        favorites[f"FAV{index}"] = item[1]

    return {
        "format": "pico-sbc-calculator-state",
        "version": 6,
        "result": float(result_fields[0]),
        "result_text": result_fields[0],
        "expression": expression_fields[0] if expression_fields else "",
        "variables": variables,
        "functions": functions,
        "history": history,
        "statistics": {"mode": mode, "values": values},
        "program": program,
        "angle": angle_fields[0],
        "precision": precision_fields[0],
        "memory": memory_fields[0],
        "favorites": favorites,
        "programmer": {
            "value": int(programmer["value"]),
            "base": programmer["base"],
            "signed": programmer["signed"] == "1",
            "selected_bit": int(programmer["bit"]),
        },
        "number_format": {
            "bits": int(number_format["bits"]),
            "fraction": int(number_format["fraction"]),
        },
        "graph": {name: float(value) for name, value in graph.items()},
        "circuit": circuit,
    }


def import_state(client: Any, data: dict[str, Any]) -> None:
    if data.get("format") not in (None, "pico-sbc-calculator-state"):
        raise ProtocolError("Unbekanntes JSON-Format")
    normalized_circuit = None
    if "circuit" in data:
        normalized_circuit = normalize_circuit(data["circuit"])
    if "expression" in data:
        expression = str(data["expression"])
        client.command(f"SET EXPR {expression}")

    if "angle" in data:
        angle = str(data["angle"]).upper()
        if angle not in ("DEG", "RAD"):
            raise ProtocolError("Winkelmodus muss DEG oder RAD sein")
        client.command(f"SET ANGLE {angle}")

    if "precision" in data:
        precision = str(data["precision"]).upper()
        if precision not in ("NORMAL", "HIGH", "ULTRA"):
            raise ProtocolError(
                "Praezisionsmodus muss NORMAL, HIGH oder ULTRA sein")
        client.command(f"SET PRECISION {precision}")

    if "memory" in data:
        memory = str(data["memory"])
        try:
            finite_memory = math.isfinite(float(memory))
        except ValueError as error:
            raise ProtocolError("Speicherwert muss endlich sein") from error
        if not finite_memory:
            raise ProtocolError("Speicherwert muss endlich sein")
        client.command(f"SET MEMORY {memory}")

    favorites = data.get("favorites", {})
    if not isinstance(favorites, dict):
        raise ProtocolError("favorites muss ein Objekt sein")
    for name, token_value in favorites.items():
        normalized = str(name).upper()
        if not re.fullmatch(r"FAV[1-6]", normalized):
            raise ProtocolError(f"Ungueltiger Favorit: {name}")
        token = str(token_value)
        if not token or len(token.encode("ascii")) >= 24:
            raise ProtocolError(f"Ungueltiger Favorit: {name}")
        client.command(f"SET FAVORITE {normalized[3:]} {token}")

    number_format = data.get("number_format")
    if number_format is not None:
        if not isinstance(number_format, dict):
            raise ProtocolError("number_format muss ein Objekt sein")
        bits = int(number_format.get("bits", 64))
        fraction = int(number_format.get("fraction", 16))
        if bits not in (8, 16, 32, 64) or not 0 <= fraction < bits:
            raise ProtocolError("Ungueltiges Zahlenformat")
        client.command(f"SET FORMAT {bits} {fraction}")

    programmer = data.get("programmer")
    if programmer is not None:
        if not isinstance(programmer, dict):
            raise ProtocolError("programmer muss ein Objekt sein")
        value = int(programmer.get("value", 0))
        base = str(programmer.get("base", "DEC")).upper()
        signed = 1 if bool(programmer.get("signed", False)) else 0
        selected = int(programmer.get("selected_bit", 0))
        if value < 0 or value > (1 << 64) - 1 or base not in (
                "BIN", "DEC", "HEX") or selected < 0:
            raise ProtocolError("Ungueltiger Programmer-Zustand")
        client.command(
            f"SET PROGRAMMER {value} {base} {signed} {selected}")

    graph = data.get("graph")
    if graph is not None:
        if not isinstance(graph, dict):
            raise ProtocolError("graph muss ein Objekt sein")
        values = [float(graph[name]) for name in
                  ("xmin", "xmax", "ymin", "ymax")]
        if not all(math.isfinite(value) for value in values):
            raise ProtocolError("Graphbereich muss endlich sein")
        client.command("SET GRAPH " + " ".join(
            f"{value:.17g}" for value in values))

    variables = data.get("variables", {})
    if not isinstance(variables, dict):
        raise ProtocolError("variables muss ein Objekt sein")
    normalized_variables = []
    for name, value in variables.items():
        normalized = str(name).upper()
        if normalized not in tuple("ABCDEF"):
            raise ProtocolError(f"Ungueltige Variable: {name}")
        number = str(value)
        try:
            finite_number = math.isfinite(float(number))
        except ValueError as error:
            raise ProtocolError(
                f"Ungueltiger Variablenwert: {name}") from error
        if not finite_number:
            raise ProtocolError(f"Ungueltiger Variablenwert: {name}")
        normalized_variables.append((normalized, number))
    for name, value in normalized_variables:
        client.command(f"SET VAR {name} {value}")

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
    if statistics is not None:
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

    if "program" in data:
        synchronize_basic_program(client, data["program"])

    if normalized_circuit is not None:
        synchronize_circuit(client, normalized_circuit)


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
