"""Testable synchronization model shared by the calculator desktop GUI."""

from __future__ import annotations

from dataclasses import dataclass
import math
import re
import time
from typing import Any, Callable

from pico_calc_cli import (
    CIRCUIT_GATE_INPUTS,
    CIRCUIT_GATE_TYPES,
    CIRCUIT_NODE_CAPACITY,
    CIRCUIT_WIRE_CAPACITY,
    CIRCUIT_WORLD_HEIGHT,
    CIRCUIT_WORLD_WIDTH,
    CIRCUIT_ZOOM_LEVELS,
    ProtocolError,
    export_state,
    fields,
    import_state,
    normalize_circuit,
    normalize_basic_program,
    read_basic_output,
    read_circuit,
    synchronize_basic_program,
    synchronize_circuit,
)


@dataclass(frozen=True)
class DeviceSnapshot:
    info: dict[str, str]
    diagnostics: dict[str, str]
    state: dict[str, Any]


@dataclass(frozen=True)
class BasicRunResult:
    status: dict[str, str]
    output: list[str]


def parse_properties(response: str, expected: str) -> dict[str, str]:
    properties: dict[str, str] = {}
    for field in fields(response, expected):
        if "=" not in field:
            raise ProtocolError(f"Ungueltiges {expected}-Feld: {field}")
        name, value = field.split("=", 1)
        if not name or name in properties:
            raise ProtocolError(f"Ungueltiges {expected}-Feld: {field}")
        properties[name] = value
    return properties


def read_device_snapshot(client: Any) -> DeviceSnapshot:
    info = parse_properties(client.command("INFO"), "INFO")
    try:
        protocol_version = int(info.get("protocol", "0"))
    except ValueError as error:
        raise ProtocolError("Ungueltige Protokollversion") from error
    if protocol_version < 6:
        raise ProtocolError(
            "Firmware 2.3.0 mit USB-Protokoll 6 oder neuer erforderlich"
        )
    diagnostics = parse_properties(client.command("DIAG"), "DIAG")
    return DeviceSnapshot(
        info=info,
        diagnostics=diagnostics,
        state=export_state(client),
    )


def evaluate_expression(client: Any, expression: str) -> str:
    if not expression.strip():
        raise ProtocolError("Ausdruck darf nicht leer sein")
    result = fields(client.command(f"EVAL {expression}"), "RESULT")
    if len(result) != 1:
        raise ProtocolError("Ungueltige RESULT-Antwort")
    return result[0]


def synchronize_symbols(client: Any, variables: dict[str, Any],
                        functions: dict[str, str]) -> None:
    import_state(client, {
        "variables": variables,
        "functions": functions,
    })


def synchronize_statistics(client: Any, mode: int,
                           values: list[list[Any]]) -> None:
    import_state(client, {
        "statistics": {
            "mode": mode,
            "values": values,
        },
    })


def read_basic_status(client: Any) -> dict[str, str]:
    status = parse_properties(client.command("GET BASIC STATUS"),
                              "BASIC_STATUS")
    required = {"state", "status", "steps", "output"}
    if not required.issubset(status):
        raise ProtocolError("Unvollstaendige BASIC_STATUS-Antwort")
    if status["state"] not in {
            "STOPPED", "RUNNING", "INPUT", "FINISHED", "ERROR"}:
        raise ProtocolError("Ungueltiger BASIC-Laufzustand")
    int(status["steps"])
    int(status["output"])
    return status


def wait_for_basic_program(
        client: Any, timeout: float = 8.0,
        cancel_check: Callable[[], bool] | None = None) -> BasicRunResult:
    deadline = time.monotonic() + timeout
    while True:
        if cancel_check is not None and cancel_check():
            client.command("BASIC STOP")
        status = read_basic_status(client)
        if status["state"] != "RUNNING":
            return BasicRunResult(status=status,
                                  output=read_basic_output(client))
        if time.monotonic() >= deadline:
            client.command("BASIC STOP")
            raise ProtocolError("BASIC-Programm antwortet nicht")
        time.sleep(0.05)


def run_basic_program(
        client: Any, source: str | list[Any],
        cancel_check: Callable[[], bool] | None = None) -> BasicRunResult:
    program = normalize_basic_program(source)
    if not program:
        raise ProtocolError("BASIC-Programm darf nicht leer sein")
    synchronize_basic_program(client, program)
    client.command("BASIC RUN")
    return wait_for_basic_program(client, cancel_check=cancel_check)


def continue_basic_program(
        client: Any, value: str,
        cancel_check: Callable[[], bool] | None = None) -> BasicRunResult:
    if not value.strip():
        raise ProtocolError("BASIC-Eingabe darf nicht leer sein")
    client.command(f"BASIC INPUT {value.strip()}")
    return wait_for_basic_program(client, cancel_check=cancel_check)


def stop_basic_program(client: Any) -> BasicRunResult:
    client.command("BASIC STOP")
    return BasicRunResult(status=read_basic_status(client),
                          output=read_basic_output(client))


def format_number(value: Any) -> str:
    if isinstance(value, str):
        return value
    return f"{float(value):.12g}"


_LOGIC_SYMBOL_TO_TOKEN = {
    "¬": "!",
    "∧": "&",
    "∨": "|",
    "⊕": "^",
    "↑": " NAND ",
    "↓": " NOR ",
    "→": " IMPLIES ",
    "↔": " XNOR ",
}
_LOGIC_TOKEN_TO_SYMBOL = {
    "NOT": "¬",
    "AND": "∧",
    "OR": "∨",
    "XOR": "⊕",
    "NAND": "↑",
    "NOR": "↓",
    "IMPLIES": "→",
    "IMP": "→",
    "XNOR": "↔",
    "IFF": "↔",
    "->": "→",
    "!": "¬",
    "~": "¬",
    "&": "∧",
    "*": "∧",
    "|": "∨",
    "+": "∨",
    "^": "⊕",
}
_LOGIC_TOKEN_PATTERN = re.compile(
    r"\b(?:IMPLIES|XNOR|NAND|NOR|XOR|NOT|AND|OR|IFF|IMP)\b|->|[!~&*|+^]",
    re.IGNORECASE,
)


def normalize_logic_expression(expression: str) -> str:
    """Translate display symbols to the ASCII calculator protocol syntax."""
    normalized = str(expression)
    for symbol, token in _LOGIC_SYMBOL_TO_TOKEN.items():
        normalized = normalized.replace(symbol, token)
    return normalized.strip()


def format_logic_expression(expression: str) -> str:
    """Render every supported logical connective with its standard symbol."""
    return _LOGIC_TOKEN_PATTERN.sub(
        lambda match: _LOGIC_TOKEN_TO_SYMBOL[match.group(0).upper()],
        str(expression),
    )


def circuit_from_logic(client: Any, expression: str) -> dict[str, Any]:
    normalized = normalize_logic_expression(expression)
    if not normalized:
        raise ProtocolError("Logikausdruck darf nicht leer sein")
    result = parse_properties(
        client.command(f"MODULE CIRCUIT FROM {normalized}"),
        "CIRCUIT_FROM",
    )
    if not {"nodes", "wires"}.issubset(result):
        raise ProtocolError("Unvollstaendige CIRCUIT_FROM-Antwort")
    node_count = int(result["nodes"])
    wire_count = int(result["wires"])
    circuit = read_circuit(client)
    if (node_count != len(circuit["nodes"]) or
            wire_count != len(circuit["wires"])):
        raise ProtocolError("Widerspruechliche CIRCUIT_FROM-Antwort")
    return circuit


def circuit_to_logic(client: Any, circuit: Any,
                     node_id: int | None = None) -> dict[str, Any]:
    normalized = normalize_circuit(circuit)
    target = None
    if node_id is not None:
        target = next((index for index, node in enumerate(normalized["nodes"])
                       if node["id"] == node_id), None)
        if target is None:
            raise ProtocolError("Unbekannter Schaltplanknoten")
    synchronize_circuit(client, normalized)
    command = "MODULE CIRCUIT EXPR"
    if target is not None:
        command += f" {target}"
    result = parse_properties(client.command(command), "CIRCUIT_EXPR")
    if not {"assignment", "expression"}.issubset(result):
        raise ProtocolError("Unvollstaendige CIRCUIT_EXPR-Antwort")
    assignment = int(result["assignment"])
    if not 0 <= assignment < 64:
        raise ProtocolError("Ungueltige CIRCUIT_EXPR-Belegung")
    return {
        "assignment": assignment,
        "ascii_expression": result["expression"],
        "expression": format_logic_expression(result["expression"]),
    }


def create_circuit(demo: bool = False) -> dict[str, Any]:
    circuit: dict[str, Any] = {
        "world_width": CIRCUIT_WORLD_WIDTH,
        "world_height": CIRCUIT_WORLD_HEIGHT,
        "viewport_x": 0,
        "viewport_y": 0,
        "zoom": 150,
        "next_input": 0,
        "next_output": 0,
        "next_gate": 0,
        "nodes": [],
        "wires": [],
    }
    if not demo:
        return normalize_circuit(circuit)
    circuit.update({
        "next_input": 2,
        "next_output": 1,
        "next_gate": 1,
        "nodes": [
            {"id": 0, "type": "INPUT", "x": 24, "y": 72,
             "input": False, "label": "A"},
            {"id": 1, "type": "INPUT", "x": 24, "y": 198,
             "input": False, "label": "B"},
            {"id": 2, "type": "AND", "x": 190, "y": 136,
             "input": False, "label": "G1"},
            {"id": 3, "type": "OUTPUT", "x": 358, "y": 136,
             "input": False, "label": "Y"},
        ],
        "wires": [
            {"id": 0, "source": 0, "destination": 2, "input": 0},
            {"id": 1, "source": 1, "destination": 2, "input": 1},
            {"id": 2, "source": 2, "destination": 3, "input": 0},
        ],
    })
    return normalize_circuit(circuit)


def _free_circuit_id(items: list[dict[str, Any]], capacity: int) -> int:
    used = {int(item["id"]) for item in items}
    for candidate in range(capacity):
        if candidate not in used:
            return candidate
    raise ProtocolError("Schaltplankapazitaet erreicht")


def _next_circuit_label(circuit: dict[str, Any], gate_type: str) -> str:
    if gate_type == "INPUT":
        index = int(circuit["next_input"])
        circuit["next_input"] = index + 1
        return chr(ord("A") + index) if index < 26 else f"I{index + 1}"
    if gate_type == "OUTPUT":
        index = int(circuit["next_output"])
        circuit["next_output"] = index + 1
        return chr(ord("Y") + index) if index < 2 else f"O{index + 1}"
    index = int(circuit["next_gate"])
    circuit["next_gate"] = index + 1
    return f"G{index + 1}"


def add_circuit_node(circuit: Any, gate_type: str,
                     x: int, y: int) -> tuple[dict[str, Any], int]:
    updated = normalize_circuit(circuit)
    normalized_type = gate_type.upper()
    if normalized_type not in CIRCUIT_GATE_TYPES:
        raise ProtocolError(f"Unbekannter Gattertyp: {gate_type}")
    node_id = _free_circuit_id(updated["nodes"], CIRCUIT_NODE_CAPACITY)
    label = _next_circuit_label(updated, normalized_type)
    if len(label) > 7:
        raise ProtocolError("Keine weitere kurze Gatterbezeichnung verfuegbar")
    updated["nodes"].append({
        "id": node_id,
        "type": normalized_type,
        "x": int(x),
        "y": int(y),
        "input": False,
        "label": label,
    })
    return normalize_circuit(updated), node_id


def move_circuit_node(circuit: Any, node_id: int,
                      x: int, y: int) -> dict[str, Any]:
    updated = normalize_circuit(circuit)
    node = next((item for item in updated["nodes"]
                 if item["id"] == node_id), None)
    if node is None:
        raise ProtocolError("Unbekannter Schaltplanknoten")
    node["x"] = max(0, min(CIRCUIT_WORLD_WIDTH, int(x)))
    node["y"] = max(0, min(CIRCUIT_WORLD_HEIGHT, int(y)))
    return normalize_circuit(updated)


def remove_circuit_node(circuit: Any, node_id: int) -> dict[str, Any]:
    updated = normalize_circuit(circuit)
    if not any(node["id"] == node_id for node in updated["nodes"]):
        raise ProtocolError("Unbekannter Schaltplanknoten")
    updated["nodes"] = [node for node in updated["nodes"]
                        if node["id"] != node_id]
    updated["wires"] = [wire for wire in updated["wires"]
                        if wire["source"] != node_id and
                        wire["destination"] != node_id]
    for index, wire in enumerate(updated["wires"]):
        wire["id"] = index
    return normalize_circuit(updated)


def set_circuit_node_type(circuit: Any, node_id: int,
                          gate_type: str) -> dict[str, Any]:
    updated = normalize_circuit(circuit)
    normalized_type = gate_type.upper()
    if normalized_type not in CIRCUIT_GATE_TYPES:
        raise ProtocolError(f"Unbekannter Gattertyp: {gate_type}")
    node = next((item for item in updated["nodes"]
                 if item["id"] == node_id), None)
    if node is None:
        raise ProtocolError("Unbekannter Schaltplanknoten")
    if node["type"] == normalized_type:
        return updated
    node["type"] = normalized_type
    node["label"] = _next_circuit_label(updated, normalized_type)
    input_count = CIRCUIT_GATE_INPUTS[normalized_type]
    updated["wires"] = [
        wire for wire in updated["wires"]
        if not (wire["destination"] == node_id and
                wire["input"] >= input_count) and
        not (wire["source"] == node_id and normalized_type == "OUTPUT")
    ]
    for index, wire in enumerate(updated["wires"]):
        wire["id"] = index
    return normalize_circuit(updated)


def set_circuit_input(circuit: Any, node_id: int,
                      value: bool) -> dict[str, Any]:
    updated = normalize_circuit(circuit)
    node = next((item for item in updated["nodes"]
                 if item["id"] == node_id), None)
    if node is None or node["type"] != "INPUT":
        raise ProtocolError("Nur INPUT-Knoten besitzen einen Schalter")
    node["input"] = bool(value)
    return normalize_circuit(updated)


def set_circuit_label(circuit: Any, node_id: int,
                      label: str) -> dict[str, Any]:
    updated = normalize_circuit(circuit)
    node = next((item for item in updated["nodes"]
                 if item["id"] == node_id), None)
    if node is None:
        raise ProtocolError("Unbekannter Schaltplanknoten")
    node["label"] = label.strip()
    return normalize_circuit(updated)


def connect_circuit_nodes(circuit: Any, source: int, destination: int,
                          destination_input: int) -> dict[str, Any]:
    updated = normalize_circuit(circuit)
    nodes = {node["id"]: node for node in updated["nodes"]}
    if source not in nodes or destination not in nodes:
        raise ProtocolError("Unbekannter Schaltplanknoten")
    if (source == destination or nodes[source]["type"] == "OUTPUT" or
            not 0 <= destination_input <
            CIRCUIT_GATE_INPUTS[nodes[destination]["type"]]):
        raise ProtocolError("Ungueltige Gatterverbindung")
    updated["wires"] = [
        wire for wire in updated["wires"]
        if not (wire["destination"] == destination and
                wire["input"] == destination_input)
    ]
    wire_id = _free_circuit_id(updated["wires"], CIRCUIT_WIRE_CAPACITY)
    updated["wires"].append({
        "id": wire_id,
        "source": source,
        "destination": destination,
        "input": destination_input,
    })
    return normalize_circuit(updated)


def disconnect_circuit_input(circuit: Any, destination: int,
                             destination_input: int) -> dict[str, Any]:
    updated = normalize_circuit(circuit)
    original_count = len(updated["wires"])
    updated["wires"] = [
        wire for wire in updated["wires"]
        if not (wire["destination"] == destination and
                wire["input"] == destination_input)
    ]
    if len(updated["wires"]) == original_count:
        raise ProtocolError("Dieser Eingang ist nicht verbunden")
    for index, wire in enumerate(updated["wires"]):
        wire["id"] = index
    return normalize_circuit(updated)


def set_circuit_view(circuit: Any, viewport_x: int, viewport_y: int,
                     zoom: int) -> dict[str, Any]:
    updated = normalize_circuit(circuit)
    if zoom not in CIRCUIT_ZOOM_LEVELS:
        raise ProtocolError("Ungueltiger Schaltplan-Zoom")
    updated["viewport_x"] = max(
        0, min(CIRCUIT_WORLD_WIDTH, int(viewport_x)))
    updated["viewport_y"] = max(
        0, min(CIRCUIT_WORLD_HEIGHT, int(viewport_y)))
    updated["zoom"] = zoom
    return normalize_circuit(updated)


def parse_integer(text: str, base: str = "DEC") -> int:
    cleaned = text.strip().replace("_", "")
    bases = {"BIN": 2, "DEC": 10, "HEX": 16}
    if base not in bases or not cleaned:
        raise ProtocolError("Ungueltige Ganzzahl")
    prefixes = {"BIN": "0b", "HEX": "0x"}
    prefix = prefixes.get(base)
    if prefix and cleaned.lower().startswith(prefix):
        cleaned = cleaned[len(prefix):]
    try:
        value = int(cleaned, bases[base])
    except ValueError as error:
        raise ProtocolError(f"Ungueltige {base}-Zahl") from error
    if value < 0 or value > (1 << 64) - 1:
        raise ProtocolError("Ganzzahl liegt ausserhalb von 64 Bit")
    return value


def programmer_operation(client: Any, action: str, value: int, bits: int,
                         operand: int | None = None) -> dict[str, str]:
    normalized = action.upper()
    allowed = {
        "VIEW", "AND", "OR", "XOR", "NOT", "NEG", "SHL", "SHR",
        "ROL", "ROR", "SWAP", "INC", "DEC", "BSET", "BCLEAR",
        "BTOGGLE",
    }
    if normalized not in allowed or bits not in (8, 16, 32, 64):
        raise ProtocolError("Ungueltige Programmer-Operation")
    command = f"MODULE PROGRAMMER {normalized} {int(value)} {bits}"
    if operand is not None:
        command += f" {int(operand)}"
    result = parse_properties(client.command(command), "PROGRAMMER")
    required = {"value", "signed", "hex", "bin", "carry", "overflow"}
    if not required.issubset(result):
        raise ProtocolError("Unvollstaendige PROGRAMMER-Antwort")
    int(result["value"])
    return result


def number_theory_operation(client: Any, action: str, a: int,
                            b: int | None = None,
                            modulus: int | None = None) -> dict[str, str]:
    normalized = action.upper()
    allowed = {"GCD", "LCM", "PRIME", "NEXT", "PREV",
               "FACTOR", "PHI", "MOD", "POW"}
    maximum = (1 << 64) - 1
    try:
        a_value = int(a)
    except (TypeError, ValueError) as error:
        raise ProtocolError("Ungueltiger Operand A") from error
    if normalized not in allowed or not 0 <= a_value <= maximum:
        raise ProtocolError("Ungueltige Zahlentheorie-Operation")
    arguments = ["MODULE", "NUMBER", normalized, str(a_value)]
    if normalized in {"GCD", "LCM", "MOD", "POW"}:
        try:
            b_value = int(b) if b is not None else -1
        except (TypeError, ValueError) as error:
            raise ProtocolError("Ungueltiger Operand B") from error
        if not 0 <= b_value <= maximum:
            raise ProtocolError("Operand B fehlt oder ist ungueltig")
        arguments.append(str(b_value))
    if normalized == "POW":
        try:
            modulus_value = int(modulus) if modulus is not None else 0
        except (TypeError, ValueError) as error:
            raise ProtocolError("Ungueltiger Modulus") from error
        if not 1 <= modulus_value <= maximum:
            raise ProtocolError("Modulus muss zwischen 1 und 2^64-1 liegen")
        arguments.append(str(modulus_value))
    result = parse_properties(client.command(" ".join(arguments)), "NUMBER")
    if result.get("action", "").upper() != normalized or "value" not in result:
        raise ProtocolError("Unvollstaendige NUMBER-Antwort")
    return result


def inspect_number_format(client: Any, value: int, bits: int,
                          fraction: int) -> dict[str, str]:
    if bits not in (8, 16, 32, 64) or not 0 <= fraction < bits:
        raise ProtocolError("Ungueltiges Zahlenformat")
    result = parse_properties(
        client.command(f"MODULE FORMAT {int(value)} {bits} {fraction}"),
        "FORMAT",
    )
    required = {"unsigned", "signed", "fixed", "float32", "float64"}
    if not required.issubset(result):
        raise ProtocolError("Unvollstaendige FORMAT-Antwort")
    return result


def inspect_ieee(client: Any, width: int, raw: int) -> dict[str, str]:
    if width not in (32, 64) or raw < 0 or raw >= 1 << width:
        raise ProtocolError("Ungueltiges IEEE-Bitmuster")
    result = parse_properties(
        client.command(f"MODULE IEEE {width} {raw}"), "IEEE")
    required = {"sign", "rawexp", "exponent", "mantissa", "class", "value"}
    if not required.issubset(result):
        raise ProtocolError("Unvollstaendige IEEE-Antwort")
    return result


def sample_graph(client: Any, function: str, x_min: float, x_max: float,
                 count: int = 81) -> list[tuple[float, float]]:
    normalized = function.upper()
    if normalized not in ("F1", "F2", "F3") or count < 2 or count > 201:
        raise ProtocolError("Ungueltige Graph-Abtastung")
    if not math.isfinite(x_min) or not math.isfinite(x_max) or x_min >= x_max:
        raise ProtocolError("Ungueltiger Graphbereich")
    points = []
    for index in range(count):
        x = x_min + (x_max - x_min) * index / (count - 1)
        try:
            result = parse_properties(
                client.command(f"MODULE GRAPH EVAL {normalized} {x:.17g}"),
                "GRAPH_VALUE",
            )
            points.append((float(result["x"]), float(result["y"])))
        except ProtocolError as error:
            if "ERR GRAPH EVAL" not in str(error):
                raise
            points.append((x, math.nan))
    return points


def analyze_graph(client: Any, action: str, first: str,
                  values: list[Any], second: str | None = None
                  ) -> dict[str, str]:
    normalized = action.upper()
    if normalized not in ("ROOT", "DERIV", "INTEGR", "XING", "EXTREMA"):
        raise ProtocolError("Ungueltige Graphanalyse")
    arguments = [f"MODULE GRAPH {normalized}", first.upper()]
    if normalized == "XING":
        arguments.append((second or "F2").upper())
    arguments.extend(f"{float(value):.17g}" for value in values)
    expected = "GRAPH_EXTREMA" if normalized == "EXTREMA" else "GRAPH_ANALYSIS"
    return parse_properties(client.command(" ".join(arguments)), expected)


def evaluate_logic(client: Any, expression: str,
                   assignment: int) -> dict[str, str]:
    normalized = normalize_logic_expression(expression)
    if not normalized or not 0 <= assignment < 64:
        raise ProtocolError("Ungueltiger Logikausdruck oder Belegung")
    return parse_properties(client.command(
        f"MODULE LOGIC EVAL {assignment} {normalized}"), "LOGIC_VALUE")


def read_truth_table(client: Any, expression: str) -> dict[str, Any]:
    normalized = normalize_logic_expression(expression)
    if not normalized:
        raise ProtocolError("Logikausdruck darf nicht leer sein")
    info = parse_properties(client.command(
        f"MODULE LOGIC INFO {normalized}"), "LOGIC_INFO")
    mask = int(info["mask"])
    row_count = int(info["rows"])
    variables = [chr(ord("A") + index) for index in range(6)
                 if mask & (1 << index)]
    rows = []
    for row in range(row_count):
        result = parse_properties(client.command(
            f"MODULE LOGIC ROW {row} {normalized}"), "LOGIC_VALUE")
        assignment = int(result["assignment"])
        rows.append({
            "assignment": assignment,
            "inputs": [1 if assignment & (1 << (ord(name) - ord("A"))) else 0
                       for name in variables],
            "value": int(result["value"]),
        })
    return {"variables": variables, "rows": rows}


def read_logic_form(client: Any, expression: str, kind: str,
                    simplified: bool = True) -> str:
    normalized_kind = kind.upper()
    normalized_expression = normalize_logic_expression(expression)
    if (normalized_kind not in ("DNF", "KNF") or
            not normalized_expression):
        raise ProtocolError("Ungueltige Normalform")
    style = "SIMPLE" if simplified else "CANONICAL"
    offset = 0
    chunks = []
    total = None
    while total is None or offset < total:
        result = parse_properties(client.command(
            f"MODULE LOGIC FORM {normalized_kind} {style} {offset} "
            f"{normalized_expression}"),
            "LOGIC_FORM",
        )
        response_offset = int(result["offset"])
        response_total = int(result["total"])
        data = result.get("data", "")
        if response_offset != offset or response_total < offset + len(data):
            raise ProtocolError("Ungueltige LOGIC_FORM-Antwort")
        chunks.append(data)
        total = response_total
        offset += len(data)
        if not data and offset < total:
            raise ProtocolError("Leere LOGIC_FORM-Teilantwort")
    return format_logic_expression("".join(chunks))


def read_unit_category(client: Any, category: int) -> dict[str, Any]:
    metadata = parse_properties(client.command(
        f"MODULE UNIT CATEGORY {category}"), "UNIT_CATEGORY")
    units = []
    for index in range(int(metadata["count"])):
        item = parse_properties(client.command(
            f"MODULE UNIT ITEM {category} {index}"), "UNIT")
        units.append(item)
    return {"index": category, "name": metadata["name"], "units": units}


def convert_unit(client: Any, category: int, from_index: int, to_index: int,
                 value: float) -> dict[str, str]:
    if not math.isfinite(value):
        raise ProtocolError("Umrechnungswert muss endlich sein")
    return parse_properties(client.command(
        f"MODULE UNIT CONVERT {category} {from_index} {to_index} "
        f"{value:.17g}"), "UNIT_RESULT")


def read_constants(client: Any) -> list[dict[str, str]]:
    count_fields = fields(client.command("MODULE CONSTANT COUNT"), "CONSTANTS")
    if len(count_fields) != 1:
        raise ProtocolError("Ungueltige CONSTANTS-Antwort")
    return [parse_properties(client.command(f"MODULE CONSTANT {index}"),
                             "CONSTANT")
            for index in range(int(count_fields[0]))]


def evaluate_complex(client: Any, expression: str,
                     angle: str) -> dict[str, str]:
    normalized = angle.upper()
    if normalized not in ("DEG", "RAD") or not expression.strip():
        raise ProtocolError("Ungueltiger komplexer Ausdruck")
    return parse_properties(client.command(
        f"MODULE COMPLEX {normalized} {expression}"), "COMPLEX")


def analyze_statistics(client: Any, action: str,
                       axis: str = "X") -> dict[str, str]:
    normalized = action.upper()
    if normalized == "SUMMARY":
        return parse_properties(client.command(
            f"STAT SUMMARY {axis.upper()}"), "STATS_SUMMARY")
    if normalized == "REGRESSION":
        return parse_properties(client.command(
            "STAT REGRESSION"), "STATS_REGRESSION")
    if normalized == "HISTOGRAM":
        return parse_properties(client.command(
            "STAT HISTOGRAM"), "STATS_HISTOGRAM")
    raise ProtocolError("Ungueltige Statistikauswertung")
