"""Testable synchronization model shared by the calculator desktop GUI."""

from __future__ import annotations

from dataclasses import dataclass
import math
import time
from typing import Any, Callable

from pico_calc_cli import (
    ProtocolError,
    export_state,
    fields,
    import_state,
    normalize_basic_program,
    read_basic_output,
    synchronize_basic_program,
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
    if protocol_version < 4:
        raise ProtocolError(
            "Firmware 1.6.0 mit USB-Protokoll 4 oder neuer erforderlich"
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
    if not expression.strip() or not 0 <= assignment < 64:
        raise ProtocolError("Ungueltiger Logikausdruck oder Belegung")
    return parse_properties(client.command(
        f"MODULE LOGIC EVAL {assignment} {expression}"), "LOGIC_VALUE")


def read_truth_table(client: Any, expression: str) -> dict[str, Any]:
    if not expression.strip():
        raise ProtocolError("Logikausdruck darf nicht leer sein")
    info = parse_properties(client.command(
        f"MODULE LOGIC INFO {expression}"), "LOGIC_INFO")
    mask = int(info["mask"])
    row_count = int(info["rows"])
    variables = [chr(ord("A") + index) for index in range(6)
                 if mask & (1 << index)]
    rows = []
    for row in range(row_count):
        result = parse_properties(client.command(
            f"MODULE LOGIC ROW {row} {expression}"), "LOGIC_VALUE")
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
    normalized = kind.upper()
    if normalized not in ("DNF", "KNF") or not expression.strip():
        raise ProtocolError("Ungueltige Normalform")
    style = "SIMPLE" if simplified else "CANONICAL"
    offset = 0
    chunks = []
    total = None
    while total is None or offset < total:
        result = parse_properties(client.command(
            f"MODULE LOGIC FORM {normalized} {style} {offset} {expression}"),
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
    return "".join(chunks)


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
