#!/usr/bin/env python3
"""Capture reproducible manual screenshots from a connected Pico calculator."""

from __future__ import annotations

import argparse
import sys
import time
import tkinter as tk
from pathlib import Path
from tkinter import ttk
from typing import Any

try:
    from PIL import ImageGrab
except ImportError as error:  # pragma: no cover - depends on the host setup
    raise SystemExit(
        "Pillow fehlt; installieren mit: "
        "python -m pip install -r tools/requirements-docs.txt"
    ) from error

import pico_calc_gui as gui


WINDOW_WIDTH = 1180
WINDOW_HEIGHT = 760


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Erzeugt Handbuch-Screenshots mit echten Ergebnissen eines "
            "angeschlossenen Pico Scientific Calculator."
        )
    )
    parser.add_argument(
        "--port",
        help="Serieller Port; bei genau einem Port wird er automatisch gewaehlt",
    )
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=2.0)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs/images/manual"),
        help="Zielverzeichnis fuer die PNG-Dateien",
    )
    return parser.parse_args()


def discover_port(selected: str | None) -> str:
    if selected:
        return selected
    try:
        from serial.tools import list_ports
    except ImportError as error:
        raise gui.ProtocolError("PySerial ist nicht installiert") from error
    ports = [port.device for port in list_ports.comports()]
    if len(ports) != 1:
        raise gui.ProtocolError(
            "Port mit --port angeben; erkannt: " + (", ".join(ports) or "keiner")
        )
    return ports[0]


def settle(root: tk.Tk, delay: float = 0.18) -> None:
    deadline = time.monotonic() + delay
    while time.monotonic() < deadline:
        root.update_idletasks()
        root.update()
        time.sleep(0.02)


def select_tab(app: gui.CalculatorLinkApp, tab: ttk.Frame) -> None:
    app.notebook.select(tab)
    settle(app.root)


def capture(app: gui.CalculatorLinkApp, output: Path, name: str) -> None:
    select_tab(app, app.notebook.nametowidget(app.notebook.select()))
    app.root.lift()
    app.root.attributes("-topmost", True)
    settle(app.root, 0.25)
    x = app.root.winfo_rootx()
    y = app.root.winfo_rooty()
    width = app.root.winfo_width()
    height = app.root.winfo_height()
    image = ImageGrab.grab(
        bbox=(x, y, x + width, y + height),
        all_screens=True,
    )
    image.save(output / name, format="PNG", optimize=True)


def replace_output(app: gui.CalculatorLinkApp, widget: tk.Text,
                   lines: list[str]) -> None:
    app._replace_text(widget, "\n".join(lines))


def find_notebook(parent: ttk.Frame) -> ttk.Notebook:
    for child in parent.winfo_children():
        if isinstance(child, ttk.Notebook):
            return child
    raise RuntimeError("Unterregister fuer CODE nicht gefunden")


def fill_unit_combos(app: gui.CalculatorLinkApp,
                     category: dict[str, Any]) -> tuple[int, int]:
    labels = [f"{unit['name']} ({unit['symbol']})"
              for unit in category["units"]]
    app.unit_from_combo["values"] = labels
    app.unit_to_combo["values"] = labels

    def index_for(symbol: str, fallback: int) -> int:
        for index, unit in enumerate(category["units"]):
            if str(unit.get("symbol", "")).lower() == symbol.lower():
                return index
        return min(fallback, len(labels) - 1)

    from_index = index_for("km", 0)
    to_index = index_for("mi", 1)
    app.unit_from_combo.current(from_index)
    app.unit_to_combo.current(to_index)
    return from_index, to_index


def capture_all(app: gui.CalculatorLinkApp, client: gui.SerialClient,
                output: Path) -> None:
    output.mkdir(parents=True, exist_ok=True)

    # Calculator: exact integer arithmetic beyond binary64 precision.
    for expression in ("0.1+0.2", "sqrt(2)", "9007199254740993+1"):
        value = gui.evaluate_expression(client, expression)
    app._apply_snapshot(gui.read_device_snapshot(client))
    app.expression_var.set(expression)
    app._set_result(value)
    select_tab(app, app.calculator_tab)
    capture(app, output, "01-calculator.png")

    # Programmer: 8-bit XOR with simultaneous DEC/HEX/BIN interpretation.
    app.programmer_value_var.set("A5")
    app.programmer_operand_var.set("0F")
    app.programmer_base_var.set("HEX")
    app.programmer_bits_var.set(8)
    app.programmer_signed_var.set(True)
    programmer = gui.programmer_operation(client, "XOR", 0xA5, 8, 0x0F)
    app.programmer_value_var.set(programmer["hex"])
    replace_output(app, app.programmer_output, [
        f"DEC unsigned  {programmer['value']}",
        f"DEC signed    {programmer['signed']}",
        f"HEX           {programmer['hex']}",
        f"BIN           {programmer['bin']}",
        f"Carry {programmer['carry']}    Overflow {programmer['overflow']}",
    ])
    code_pages = find_notebook(app.programmer_tab)
    code_pages.select(0)
    select_tab(app, app.programmer_tab)
    capture(app, output, "02-programmer.png")

    # Fixed point and IEEE-754 share the second CODE sub-page.
    app.programmer_value_var.set("98304")
    app.programmer_base_var.set("DEC")
    app.programmer_bits_var.set(32)
    app.format_fraction_var.set(16)
    number_format = gui.inspect_number_format(client, 98304, 32, 16)
    replace_output(app, app.format_output, [
        f"Unsigned        {number_format['unsigned']}",
        f"2er-Komplement {number_format['signed']}",
        f"Q15.16          {number_format['fixed']}",
        f"Float32 bits    0x{number_format['float32']}",
        f"Float64 bits    0x{number_format['float64']}",
    ])
    code_pages.select(1)
    select_tab(app, app.programmer_tab)
    capture(app, output, "03-fixed-point.png")

    app.ieee_width_var.set(32)
    app.ieee_raw_var.set("40490FDB")
    ieee = gui.inspect_ieee(client, 32, 0x40490FDB)
    replace_output(app, app.format_output, [
        f"IEEE {ieee.get('width', '32')}  {ieee['class']}",
        f"Wert          {ieee['value']}",
        f"Vorzeichen    {ieee['sign']}",
        f"Exponent raw  {ieee['rawexp']}",
        f"Exponent      {ieee['exponent']}",
        f"Mantisse      {ieee['mantissa']}",
    ])
    capture(app, output, "04-ieee754.png")

    # Number theory: factorization is performed by the firmware.
    app.number_a_var.set("360")
    app.number_b_var.set("168")
    app.number_modulus_var.set("17")
    factors = gui.number_theory_operation(client, "FACTOR", 360)
    app.number_result_var.set(
        "360 = " + factors["value"].replace("*", " * ")
    )
    select_tab(app, app.number_tab)
    capture(app, output, "05-number-theory.png")

    # Graph: every point is sampled through MODULE GRAPH EVAL on the Pico.
    client.command("SET ANGLE RAD")
    app.angle_var.set("RAD")
    app.device_vars["Winkel"].set("RAD")
    variables = {name: app.variable_vars[name].get() for name in "ABCDEF"}
    functions = {"F1": "sin(x)", "F2": "cos(x)", "F3": "0.1*x^2-1"}
    gui.synchronize_symbols(client, variables, functions)
    for name, expression in functions.items():
        app.function_vars[name].set(expression)
    ranges = {"xmin": -6.283185307179586, "xmax": 6.283185307179586,
              "ymin": -1.5, "ymax": 1.5}
    for name, value in ranges.items():
        app.graph_range_vars[name].set(gui.format_number(value))
    series = {
        name: gui.sample_graph(client, name, ranges["xmin"], ranges["xmax"], 81)
        for name in functions
    }
    app.graph_analysis_var.set("INTEGR")
    app.graph_function_var.set("F1")
    app.graph_left_var.set("0")
    app.graph_right_var.set("3.141592653589793")
    graph_analysis = gui.analyze_graph(
        client, "INTEGR", "F1", [0, 3.141592653589793]
    )
    select_tab(app, app.graph_tab)
    app._draw_graph(series, ranges)
    app.graph_analysis_result_var.set(
        "Integral F1 von 0 bis pi: "
        + "  ".join(f"{name}={value}"
                     for name, value in graph_analysis.items())
    )
    capture(app, output, "06-graph.png")
    client.command("SET ANGLE DEG")
    app.angle_var.set("DEG")
    app.device_vars["Winkel"].set("DEG")

    # Logic: use symbolic connectives in the UI and ASCII normalization on USB.
    logic_expression = "(A↑B)↔(¬A∨¬B)"
    app.logic_expression_var.set(logic_expression)
    table = gui.read_truth_table(client, logic_expression)
    logic_lines = ["  ".join(table["variables"] + ["OUT"]),
                   "--" * (len(table["variables"]) + 1)]
    for row in table["rows"]:
        logic_lines.append("  ".join(
            str(item) for item in row["inputs"] + [row["value"]]
        ))
    replace_output(app, app.logic_output, logic_lines)
    select_tab(app, app.logic_tab)
    capture(app, output, "07-logic.png")

    circuit_expression = "(A∧B)∨(¬C⊕D)"
    circuit = gui.circuit_from_logic(client, circuit_expression)
    circuit["zoom"] = 100
    circuit["viewport_x"] = 0
    circuit["viewport_y"] = 0
    app._load_circuit(gui.normalize_circuit(circuit),
                      "Aus Logikausdruck erzeugt")
    select_tab(app, app.circuit_tab)
    app._draw_circuit()
    capture(app, output, "08-circuit.png")

    # Units and constants are both read from firmware-owned catalogs.
    app.unit_category_var.set("LENGTH")
    unit_category = gui.read_unit_category(client, 0)
    app.unit_cache[0] = unit_category
    from_index, to_index = fill_unit_combos(app, unit_category)
    app.unit_input_var.set("5")
    converted = gui.convert_unit(client, 0, from_index, to_index, 5.0)
    app.unit_result_var.set(
        f"5 {converted['from']} = {converted['value']} {converted['to']}"
    )
    constants = gui.read_constants(client)
    for column, width in (("symbol", 65), ("name", 180), ("value", 225),
                          ("unit", 140), ("source", 120)):
        app.constants_tree.column(column, width=width, stretch=False)
    app.constants_tree.delete(*app.constants_tree.get_children())
    for item in constants:
        app.constants_tree.insert("", tk.END, values=(
            item["symbol"], item["name"], item["value"],
            item["unit"], item["source"],
        ))
    select_tab(app, app.units_tab)
    capture(app, output, "09-units-constants.png")

    # Complex arithmetic with cartesian and polar output.
    complex_expression = "(1+2i)*(3-i)"
    app.complex_expression_var.set(complex_expression)
    app.complex_angle_var.set("DEG")
    complex_result = gui.evaluate_complex(client, complex_expression, "DEG")
    app.complex_result_var.set(
        f"Kartesisch\n{complex_result['cart']}\n\n"
        f"Polar\n{complex_result['polar']}\n\n"
        f"Real = {complex_result['real']}\nImag = {complex_result['imag']}"
    )
    select_tab(app, app.complex_tab)
    capture(app, output, "10-complex.png")

    # Statistics: synchronize a perfectly linear 2VAR series and regress it.
    statistics = [[1, 3], [2, 5], [3, 7], [4, 9], [5, 11]]
    gui.synchronize_statistics(client, 2, statistics)
    app._apply_snapshot(gui.read_device_snapshot(client))
    regression = gui.analyze_statistics(client, "REGRESSION")
    replace_output(app, app.stats_analysis_output, [
        f"{name:<12} {value}" for name, value in regression.items()
    ])
    select_tab(app, app.statistics_tab)
    capture(app, output, "11-statistics.png")

    # Variables, functions, memory and favorites are synchronized together.
    symbol_state = {
        "variables": {
            "A": "9.81", "B": "299792458", "C": "6.67430e-11",
            "D": "1.602176634e-19", "E": "0", "F": "0",
        },
        "functions": functions,
        "memory": "42",
        "favorites": {
            "FAV1": "sqrt(", "FAV2": "sin(", "FAV3": "log(",
            "FAV4": "pi", "FAV5": "ANS*2", "FAV6": "1/",
        },
    }
    gui.import_state(client, symbol_state)
    app._apply_snapshot(gui.read_device_snapshot(client))
    select_tab(app, app.symbols_tab)
    capture(app, output, "12-memory-symbols.png")

    # BASIC: run the bundled text Mandelbrot example on the Pico interpreter.
    source_path = Path("examples/basic/09_mandelbrot_text.bas")
    source = source_path.read_text(encoding="ascii")
    basic_result = gui.run_basic_program(client, source)
    app._apply_snapshot(gui.read_device_snapshot(client))
    app._set_basic_source(gui.normalize_basic_program(source))
    app._apply_basic_result(basic_result)
    select_tab(app, app.basic_tab)
    capture(app, output, "13-basic.png")

    app._apply_snapshot(gui.read_device_snapshot(client))
    select_tab(app, app.history_tab)
    capture(app, output, "14-history.png")

    # Keep the USB page concise despite the operations logged above.
    app._replace_text(app.console_text, "")
    for command in ("INFO", "DIAG", "MODULE NUMBER GCD 84 30"):
        app._append_console(f"> {command}")
        app._append_console(f"< {client.command(command)}")
    app.raw_command_var.set("GET STATE")
    app.status_var.set("Dokumentationsbeispiele erfolgreich vom Pico gelesen")
    select_tab(app, app.console_tab)
    capture(app, output, "15-usb-console.png")


def main() -> int:
    arguments = parse_arguments()
    port = discover_port(arguments.port)
    output = arguments.output.resolve()
    client: gui.SerialClient | None = None
    backup: dict[str, Any] | None = None
    root: tk.Tk | None = None
    app: gui.CalculatorLinkApp | None = None
    try:
        gui.enable_native_dpi()
        root = tk.Tk()
        root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}+20+20")
        app = gui.CalculatorLinkApp(root)
        settle(root)

        client = gui.SerialClient(port, arguments.baud, arguments.timeout).open()
        # Import restores every writable field. ANS, history, and the LCD page
        # are read-only protocol data and intentionally retain demo activity.
        backup = gui.export_state(client)
        app.client = client
        app._set_connected(True, port)
        app._apply_snapshot(gui.read_device_snapshot(client))
        app.status_var.set("Verbunden; Beispiele werden auf dem Pico ausgefuehrt")
        capture_all(app, client, output)
        print(f"15 Screenshots in {output} erzeugt")
        return 0
    except (OSError, RuntimeError, gui.ProtocolError, tk.TclError) as error:
        print(f"Fehler: {error}", file=sys.stderr)
        return 1
    finally:
        if client is not None and backup is not None:
            try:
                gui.import_state(client, backup)
            except (OSError, gui.ProtocolError) as error:
                print(f"Warnung: Pico-Zustand nicht wiederhergestellt: {error}",
                      file=sys.stderr)
        if app is not None:
            app.client = None
            app.tasks.put(None)
        if client is not None:
            client.close()
        if root is not None:
            try:
                root.destroy()
            except tk.TclError:
                pass


if __name__ == "__main__":
    raise SystemExit(main())
