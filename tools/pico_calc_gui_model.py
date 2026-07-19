"""Testable synchronization model shared by the calculator desktop GUI."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from pico_calc_cli import ProtocolError, export_state, fields, import_state


@dataclass(frozen=True)
class DeviceSnapshot:
    info: dict[str, str]
    diagnostics: dict[str, str]
    state: dict[str, Any]


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
    diagnostics = parse_properties(client.command("DIAG"), "DIAG")
    return DeviceSnapshot(
        info=info,
        diagnostics=diagnostics,
        state=export_state(client),
    )


def evaluate_expression(client: Any, expression: str) -> float:
    if not expression.strip():
        raise ProtocolError("Ausdruck darf nicht leer sein")
    result = fields(client.command(f"EVAL {expression}"), "RESULT")
    if len(result) != 1:
        raise ProtocolError("Ungueltige RESULT-Antwort")
    return float(result[0])


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


def format_number(value: Any) -> str:
    return f"{float(value):.12g}"
