"""Testable synchronization model shared by the calculator desktop GUI."""

from __future__ import annotations

from dataclasses import dataclass
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
    if protocol_version < 2:
        raise ProtocolError(
            "Firmware 1.3.0 mit USB-Protokoll 2 oder neuer erforderlich"
        )
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
    return f"{float(value):.12g}"
