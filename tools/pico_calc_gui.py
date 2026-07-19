#!/usr/bin/env python3
"""Desktop synchronization and control center for the Pico calculator."""

from __future__ import annotations

import json
import math
import queue
import sys
import threading
import time
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import Any, Callable

from pico_calc_cli import (
    ProtocolError,
    SerialClient,
    export_state,
    import_state,
    normalize_basic_program,
    read_basic_program,
    synchronize_basic_program,
)
from pico_calc_gui_model import (
    BasicRunResult,
    DeviceSnapshot,
    analyze_graph,
    analyze_statistics,
    continue_basic_program,
    convert_unit,
    evaluate_complex,
    evaluate_expression,
    evaluate_logic,
    format_number,
    inspect_ieee,
    inspect_number_format,
    parse_integer,
    programmer_operation,
    read_constants,
    read_device_snapshot,
    read_logic_form,
    read_truth_table,
    read_unit_category,
    run_basic_program,
    sample_graph,
    stop_basic_program,
    synchronize_statistics,
    synchronize_symbols,
)


APP_NAME = "Pico Calculator Link"
APP_VERSION = "2.1"
BG = "#eef1f4"
PANEL = "#ffffff"
INK = "#202428"
MUTED = "#626b73"
LINE = "#d5dae0"
HEADER = "#24272b"
ACCENT = "#b3262d"
ACCENT_ACTIVE = "#921f25"
TEAL = "#00796b"
AMBER = "#f2b84b"
DISPLAY = "#111418"


Task = tuple[
    str,
    Callable[[], Any],
    Callable[[Any], None] | None,
    Callable[[Exception], None] | None,
]


def enable_native_dpi() -> None:
    if sys.platform != "win32":
        return
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except (AttributeError, OSError):
        try:
            ctypes.windll.user32.SetProcessDPIAware()
        except (AttributeError, OSError):
            pass


class CalculatorLinkApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title(APP_NAME)
        self.root.geometry("1180x760")
        self.root.minsize(1080, 680)
        self.root.configure(bg=BG)

        self.client: SerialClient | None = None
        self.connected = False
        self.busy = False
        self.snapshot: DeviceSnapshot | None = None
        self.tasks: queue.Queue[Task | None] = queue.Queue()
        self.results: queue.Queue[tuple[Task, Any, Exception | None]] = queue.Queue()
        self.worker = threading.Thread(target=self._worker_loop, daemon=True)

        self.port_var = tk.StringVar()
        self.connection_var = tk.StringVar(value="Nicht verbunden")
        self.status_var = tk.StringVar(value="Bereit")
        self.expression_var = tk.StringVar()
        self.result_var = tk.StringVar(value="0")
        self.stats_mode_var = tk.IntVar(value=1)
        self.stats_x_var = tk.StringVar()
        self.stats_y_var = tk.StringVar()
        self.raw_command_var = tk.StringVar()
        self.basic_status_var = tk.StringVar(value="STOPPED")
        self.basic_input_var = tk.StringVar()
        self.basic_connection_buttons: list[ttk.Button] = []
        self.basic_running = False
        self.basic_cancel = threading.Event()
        self.device_vars = {
            name: tk.StringVar(value="-")
            for name in ("Modell", "Firmware", "Protokoll", "Winkel",
                         "Präzision", "Seite", "Verlauf", "Statistik",
                         "BASIC")
        }
        self.variable_vars = {
            name: tk.StringVar(value="0") for name in "ABCDEF"
        }
        self.function_vars = {
            name: tk.StringVar() for name in ("F1", "F2", "F3")
        }
        self.angle_var = tk.StringVar(value="DEG")
        self.precision_var = tk.StringVar(value="HIGH")
        self.memory_var = tk.StringVar(value="0")
        self.favorite_vars = {
            f"FAV{index}": tk.StringVar() for index in range(1, 7)
        }
        self.programmer_value_var = tk.StringVar(value="0")
        self.programmer_operand_var = tk.StringVar(value="0")
        self.programmer_base_var = tk.StringVar(value="DEC")
        self.programmer_bits_var = tk.IntVar(value=64)
        self.programmer_bit_var = tk.IntVar(value=0)
        self.programmer_signed_var = tk.BooleanVar(value=False)
        self.programmer_result_var = tk.StringVar(value="")
        self.format_fraction_var = tk.IntVar(value=16)
        self.ieee_width_var = tk.IntVar(value=32)
        self.ieee_raw_var = tk.StringVar(value="00000000")
        self.format_result_var = tk.StringVar(value="")
        self.graph_range_vars = {
            "xmin": tk.StringVar(value="-10"),
            "xmax": tk.StringVar(value="10"),
            "ymin": tk.StringVar(value="-10"),
            "ymax": tk.StringVar(value="10"),
        }
        self.graph_function_var = tk.StringVar(value="F1")
        self.graph_second_var = tk.StringVar(value="F2")
        self.graph_analysis_var = tk.StringVar(value="ROOT")
        self.graph_left_var = tk.StringVar(value="-10")
        self.graph_right_var = tk.StringVar(value="10")
        self.graph_analysis_result_var = tk.StringVar(value="")
        self.logic_expression_var = tk.StringVar(value="(A&B)|!C")
        self.logic_assignment_var = tk.IntVar(value=0)
        self.logic_simplified_var = tk.BooleanVar(value=True)
        self.unit_category_var = tk.StringVar(value="LENGTH")
        self.unit_input_var = tk.StringVar(value="1")
        self.unit_result_var = tk.StringVar(value="")
        self.unit_cache: dict[int, dict[str, Any]] = {}
        self.complex_expression_var = tk.StringVar(value="(1+2i)*(3-i)")
        self.complex_angle_var = tk.StringVar(value="DEG")
        self.complex_result_var = tk.StringVar(value="")

        self._configure_styles()
        self._build_ui()
        self._refresh_ports()
        self._set_connected(False)
        self.worker.start()
        self.root.after(50, self._poll_results)
        self.root.protocol("WM_DELETE_WINDOW", self._close)

    def _configure_styles(self) -> None:
        style = ttk.Style(self.root)
        if "clam" in style.theme_names():
            style.theme_use("clam")
        style.configure("TFrame", background=BG)
        style.configure("Panel.TFrame", background=PANEL)
        style.configure("TLabel", background=BG, foreground=INK,
                        font=("Segoe UI", 10))
        style.configure("Panel.TLabel", background=PANEL, foreground=INK,
                        font=("Segoe UI", 10))
        style.configure("Muted.Panel.TLabel", background=PANEL, foreground=MUTED,
                        font=("Segoe UI", 9))
        style.configure("Title.Panel.TLabel", background=PANEL, foreground=INK,
                        font=("Segoe UI Semibold", 12))
        style.configure("TButton", font=("Segoe UI Semibold", 9), padding=(10, 7))
        style.configure("Accent.TButton", background=ACCENT, foreground="#ffffff",
                        bordercolor=ACCENT, focuscolor=ACCENT)
        style.map("Accent.TButton",
                  background=[("active", ACCENT_ACTIVE), ("pressed", ACCENT_ACTIVE)])
        style.configure("Teal.TButton", background=TEAL, foreground="#ffffff",
                        bordercolor=TEAL, focuscolor=TEAL)
        style.map("Teal.TButton", background=[("active", "#00665b")])
        style.configure("Key.TButton", font=("Segoe UI Semibold", 11), padding=5)
        style.configure("Equals.TButton", background=ACCENT, foreground="#ffffff",
                        font=("Segoe UI Semibold", 11), padding=7)
        style.map("Equals.TButton", background=[("active", ACCENT_ACTIVE)])
        style.configure("TNotebook", background=BG, borderwidth=0)
        style.configure("TNotebook.Tab", font=("Segoe UI Semibold", 9),
                        padding=(11, 9), background="#dfe3e8")
        style.map("TNotebook.Tab",
                  background=[("selected", PANEL)],
                  foreground=[("selected", ACCENT)])
        style.configure("Treeview", font=("Segoe UI", 10), rowheight=28,
                        background=PANEL, fieldbackground=PANEL, foreground=INK,
                        bordercolor=LINE)
        style.configure("Treeview.Heading", font=("Segoe UI Semibold", 9),
                        background="#e7eaee", foreground=INK, padding=6)
        style.map("Treeview", background=[("selected", "#d9ece9")],
                  foreground=[("selected", INK)])
        style.configure("TLabelframe", background=PANEL, bordercolor=LINE,
                        relief="solid")
        style.configure("TLabelframe.Label", background=PANEL, foreground=INK,
                        font=("Segoe UI Semibold", 10))
        style.configure("Mode.Toolbutton", font=("Segoe UI Semibold", 9),
                        padding=(14, 6))
        style.map("Mode.Toolbutton",
                  background=[("selected", TEAL)],
                  foreground=[("selected", "#ffffff")])

    def _build_ui(self) -> None:
        self._build_header()

        footer = ttk.Frame(self.root, style="Panel.TFrame")
        footer.pack(fill=tk.X, side=tk.BOTTOM)
        ttk.Label(footer, textvariable=self.status_var,
                  style="Muted.Panel.TLabel").pack(side=tk.LEFT, padx=12, pady=6)
        self.progress = ttk.Progressbar(footer, mode="indeterminate", length=130)
        self.progress.pack(side=tk.RIGHT, padx=12, pady=6)

        body = ttk.Frame(self.root)
        body.pack(fill=tk.BOTH, expand=True)
        body.columnconfigure(1, weight=1)
        body.rowconfigure(0, weight=1)

        self._build_device_panel(body)
        self.notebook = ttk.Notebook(body)
        self.notebook.grid(row=0, column=1, sticky="nsew", padx=(0, 12), pady=12)

        self._build_calculator_tab()
        self._build_programmer_tab()
        self._build_graph_tab()
        self._build_logic_tab()
        self._build_units_tab()
        self._build_complex_tab()
        self._build_statistics_tab()
        self._build_symbols_tab()
        self._build_basic_tab()
        self._build_history_tab()
        self._build_console_tab()

    def _build_header(self) -> None:
        header = tk.Frame(self.root, bg=HEADER, height=62)
        header.pack(fill=tk.X)
        header.pack_propagate(False)

        tk.Label(header, text="PICO CALCULATOR LINK", bg=HEADER, fg="#ffffff",
                 font=("Segoe UI Semibold", 16)).pack(side=tk.LEFT, padx=(18, 8))
        tk.Label(header, text=f"v{APP_VERSION}", bg=HEADER, fg="#aeb5bd",
                 font=("Segoe UI", 9)).pack(side=tk.LEFT, pady=(6, 0))

        controls = tk.Frame(header, bg=HEADER)
        controls.pack(side=tk.RIGHT, padx=14, pady=12)
        self.port_combo = ttk.Combobox(controls, textvariable=self.port_var,
                                       state="readonly", width=24)
        self.port_combo.pack(side=tk.LEFT, padx=(0, 6))
        ttk.Button(controls, text="Ports neu laden",
                   command=self._refresh_ports).pack(side=tk.LEFT, padx=3)
        self.connect_button = ttk.Button(controls, text="Verbinden",
                                         style="Accent.TButton",
                                         command=self._connect)
        self.connect_button.pack(side=tk.LEFT, padx=3)
        self.disconnect_button = ttk.Button(controls, text="Trennen",
                                            command=self._disconnect)
        self.disconnect_button.pack(side=tk.LEFT, padx=3)

    def _build_device_panel(self, parent: ttk.Frame) -> None:
        panel = ttk.Frame(parent, style="Panel.TFrame", width=245)
        panel.grid(row=0, column=0, sticky="ns", padx=12, pady=12)
        panel.grid_propagate(False)
        panel.columnconfigure(0, weight=1)

        ttk.Label(panel, text="Gerät", style="Title.Panel.TLabel").grid(
            row=0, column=0, sticky="w", padx=16, pady=(18, 4))
        status_row = ttk.Frame(panel, style="Panel.TFrame")
        status_row.grid(row=1, column=0, sticky="ew", padx=16, pady=(0, 14))
        self.status_canvas = tk.Canvas(status_row, width=12, height=12,
                                       bg=PANEL, highlightthickness=0)
        self.status_canvas.pack(side=tk.LEFT, padx=(0, 7))
        self.status_dot = self.status_canvas.create_oval(2, 2, 10, 10,
                                                         fill=MUTED, outline="")
        ttk.Label(status_row, textvariable=self.connection_var,
                  style="Panel.TLabel").pack(side=tk.LEFT)

        row = 2
        for label, variable in self.device_vars.items():
            item = ttk.Frame(panel, style="Panel.TFrame")
            item.grid(row=row, column=0, sticky="ew", padx=16, pady=4)
            ttk.Label(item, text=label, style="Muted.Panel.TLabel").pack(side=tk.LEFT)
            ttk.Label(item, textvariable=variable,
                      style="Panel.TLabel").pack(side=tk.RIGHT)
            row += 1

        ttk.Separator(panel).grid(row=row, column=0, sticky="ew", padx=16, pady=14)
        row += 1
        self.sync_button = ttk.Button(panel, text="Vom Rechner laden",
                                      style="Teal.TButton",
                                      command=self._refresh_snapshot)
        self.sync_button.grid(row=row, column=0, sticky="ew", padx=16, pady=4)
        row += 1
        self.export_button = ttk.Button(panel, text="JSON exportieren",
                                        command=self._export_json)
        self.export_button.grid(row=row, column=0, sticky="ew", padx=16, pady=4)
        row += 1
        self.import_button = ttk.Button(panel, text="JSON importieren",
                                        command=self._import_json)
        self.import_button.grid(row=row, column=0, sticky="ew", padx=16, pady=4)

        panel.rowconfigure(row + 1, weight=1)
        ttk.Label(panel, text="USB CDC", style="Muted.Panel.TLabel").grid(
            row=row + 2, column=0, sticky="sw", padx=16, pady=16)

    def _new_tab(self, title: str) -> ttk.Frame:
        frame = ttk.Frame(self.notebook, style="Panel.TFrame")
        self.notebook.add(frame, text=title)
        return frame

    def _build_calculator_tab(self) -> None:
        self.calculator_tab = self._new_tab("Rechner")
        tab = self.calculator_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(2, weight=1)

        display = tk.Frame(tab, bg=DISPLAY, height=126)
        display.grid(row=0, column=0, sticky="ew", padx=18, pady=(18, 10))
        display.grid_propagate(False)
        display.columnconfigure(0, weight=1)
        self.expression_entry = tk.Entry(
            display, textvariable=self.expression_var, bg=DISPLAY, fg="#f5f7fa",
            insertbackground="#ffffff", selectbackground=TEAL, relief=tk.FLAT,
            font=("Consolas", 17), highlightthickness=0,
        )
        self.expression_entry.grid(row=0, column=0, sticky="ew", padx=18,
                                   pady=(16, 4))
        self.result_label = tk.Label(
            display, textvariable=self.result_var, bg=DISPLAY, fg=AMBER,
            anchor="e", font=("Consolas", 28, "bold"),
        )
        self.result_label.grid(row=1, column=0, sticky="ew", padx=18,
                               pady=(0, 12))
        self.expression_entry.bind("<Return>", lambda _event: self._evaluate())

        actions = ttk.Frame(tab, style="Panel.TFrame")
        actions.grid(row=1, column=0, sticky="ew", padx=18, pady=(0, 10))
        ttk.Button(actions, text="Editor zum Rechner",
                   command=self._send_expression).pack(side=tk.LEFT)
        ttk.Label(actions, text="Winkel", style="Panel.TLabel").pack(
            side=tk.LEFT, padx=(14, 5))
        angle = ttk.Combobox(actions, textvariable=self.angle_var,
                             values=("DEG", "RAD"), state="readonly", width=5)
        angle.pack(side=tk.LEFT)
        angle.bind("<<ComboboxSelected>>", lambda _event: self._set_angle())
        ttk.Label(actions, text="Präzision", style="Panel.TLabel").pack(
            side=tk.LEFT, padx=(14, 5))
        precision = ttk.Combobox(
            actions, textvariable=self.precision_var,
            values=("NORMAL", "HIGH", "ULTRA"), state="readonly", width=8)
        precision.pack(side=tk.LEFT)
        precision.bind("<<ComboboxSelected>>",
                       lambda _event: self._set_precision())
        ttk.Button(actions, text="Ausführen", style="Accent.TButton",
                   command=self._evaluate).pack(side=tk.RIGHT)

        content = ttk.Frame(tab, style="Panel.TFrame")
        content.grid(row=2, column=0, sticky="nsew", padx=18, pady=(0, 18))
        content.columnconfigure(0, weight=3)
        content.columnconfigure(1, weight=2)
        content.rowconfigure(0, weight=1)

        keypad = ttk.Frame(content, style="Panel.TFrame")
        keypad.grid(row=0, column=0, sticky="nsew", padx=(0, 14))
        keys = (
            ("sin(", "cos(", "tan(", "sqrt(", "ln(", "log("),
            ("7", "8", "9", "/", "(", ")"),
            ("4", "5", "6", "*", "^", "ANS"),
            ("1", "2", "3", "-", "pi", "e"),
            ("0", ".", ",", "+", "DEL", "AC"),
        )
        for column in range(6):
            keypad.columnconfigure(column, weight=1, uniform="key")
        for row in range(5):
            keypad.rowconfigure(row, weight=1, uniform="key")
        for row, labels in enumerate(keys):
            for column, label in enumerate(labels):
                ttk.Button(
                    keypad, text=label, style="Key.TButton",
                    width=5,
                    command=lambda value=label: self._press_key(value),
                ).grid(row=row, column=column, sticky="nsew", padx=3, pady=3)

        session = ttk.LabelFrame(content, text="Sitzung")
        session.grid(row=0, column=1, sticky="nsew")
        session.columnconfigure(0, weight=1)
        session.rowconfigure(0, weight=1)
        self.session_tree = ttk.Treeview(
            session, columns=("expression", "result"), show="headings",
            selectmode="browse",
        )
        self.session_tree.heading("expression", text="Ausdruck")
        self.session_tree.heading("result", text="Ergebnis")
        self.session_tree.column("expression", width=135, anchor=tk.W)
        self.session_tree.column("result", width=85, anchor=tk.E)
        self.session_tree.grid(row=0, column=0, sticky="nsew", padx=8, pady=8)
        self.session_tree.bind("<Double-1>", self._use_session_expression)

    def _build_programmer_tab(self) -> None:
        self.programmer_tab = self._new_tab("Code")
        tab = self.programmer_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(0, weight=1)
        pages = ttk.Notebook(tab)
        pages.grid(row=0, column=0, sticky="nsew", padx=18, pady=18)

        programmer = ttk.Frame(pages, style="Panel.TFrame")
        pages.add(programmer, text="Programmer")
        programmer.columnconfigure(0, weight=1)
        programmer.rowconfigure(4, weight=1)
        value_row = ttk.Frame(programmer, style="Panel.TFrame")
        value_row.grid(row=0, column=0, sticky="ew", padx=14, pady=(14, 8))
        ttk.Label(value_row, text="Wert", style="Panel.TLabel").pack(side=tk.LEFT)
        ttk.Entry(value_row, textvariable=self.programmer_value_var,
                  font=("Consolas", 12), width=30).pack(
                      side=tk.LEFT, padx=(6, 12), fill=tk.X, expand=True)
        ttk.Combobox(value_row, textvariable=self.programmer_base_var,
                     values=("BIN", "DEC", "HEX"), state="readonly",
                     width=5).pack(side=tk.LEFT)
        ttk.Combobox(value_row, textvariable=self.programmer_bits_var,
                     values=(8, 16, 32, 64), state="readonly",
                     width=4).pack(side=tk.LEFT, padx=6)
        ttk.Checkbutton(value_row, text="signed",
                        variable=self.programmer_signed_var).pack(side=tk.LEFT)
        ttk.Button(value_row, text="Anzeigen", style="Teal.TButton",
                   command=lambda: self._programmer_action("VIEW")).pack(
                       side=tk.LEFT, padx=(8, 0))

        operand_row = ttk.Frame(programmer, style="Panel.TFrame")
        operand_row.grid(row=1, column=0, sticky="ew", padx=14, pady=4)
        ttk.Label(operand_row, text="Operand", style="Panel.TLabel").pack(
            side=tk.LEFT)
        ttk.Entry(operand_row, textvariable=self.programmer_operand_var,
                  font=("Consolas", 10), width=20).pack(side=tk.LEFT, padx=6)
        ttk.Label(operand_row, text="Bitoperationen verwenden die gewählte Wortbreite.",
                  style="Muted.Panel.TLabel").pack(side=tk.LEFT, padx=8)
        operation_pad = ttk.Frame(programmer, style="Panel.TFrame")
        operation_pad.grid(row=2, column=0, sticky="ew", padx=14, pady=4)
        for action in ("AND", "OR", "XOR", "NOT", "NEG", "SHL", "SHR",
                       "ROL", "ROR", "SWAP", "INC", "DEC"):
            index = ("AND", "OR", "XOR", "NOT", "NEG", "SHL", "SHR",
                     "ROL", "ROR", "SWAP", "INC", "DEC").index(action)
            operation_pad.columnconfigure(index % 6, weight=1, uniform="op")
            ttk.Button(operation_pad, text=action, width=5,
                       command=lambda value=action:
                           self._programmer_action(value)).grid(
                               row=index // 6, column=index % 6,
                               sticky="ew", padx=2, pady=2)

        bit_row = ttk.Frame(programmer, style="Panel.TFrame")
        bit_row.grid(row=3, column=0, sticky="ew", padx=14, pady=4)
        ttk.Label(bit_row, text="Bit", style="Panel.TLabel").pack(side=tk.LEFT)
        ttk.Spinbox(bit_row, from_=0, to=63, width=5,
                    textvariable=self.programmer_bit_var).pack(
                        side=tk.LEFT, padx=6)
        for action, label in (("BSET", "Setzen"), ("BCLEAR", "Löschen"),
                              ("BTOGGLE", "Toggle")):
            ttk.Button(bit_row, text=label,
                       command=lambda value=action:
                           self._programmer_action(value)).pack(
                               side=tk.LEFT, padx=3)
        self.programmer_output = tk.Text(
            programmer, height=8, wrap=tk.WORD, bg=DISPLAY, fg="#f5f7fa",
            font=("Consolas", 12), relief=tk.FLAT, padx=12, pady=10)
        self.programmer_output.grid(row=4, column=0, sticky="nsew",
                                    padx=14, pady=(8, 14))
        self.programmer_output.configure(state=tk.DISABLED)

        formats = ttk.Frame(pages, style="Panel.TFrame")
        pages.add(formats, text="Fix / Gleitkomma")
        formats.columnconfigure(0, weight=1)
        controls = ttk.Frame(formats, style="Panel.TFrame")
        controls.grid(row=0, column=0, sticky="ew", padx=14, pady=14)
        ttk.Label(controls, text="Q-Nachkommabits",
                  style="Panel.TLabel").pack(side=tk.LEFT)
        ttk.Spinbox(controls, from_=0, to=63, width=5,
                    textvariable=self.format_fraction_var).pack(
                        side=tk.LEFT, padx=6)
        ttk.Button(controls, text="2er-Komplement / Fix / Float",
                   style="Teal.TButton", command=self._inspect_format).pack(
                       side=tk.LEFT, padx=(6, 18))
        ttk.Label(controls, text="IEEE", style="Panel.TLabel").pack(side=tk.LEFT)
        ttk.Combobox(controls, textvariable=self.ieee_width_var,
                     values=(32, 64), state="readonly", width=4).pack(
                         side=tk.LEFT, padx=5)
        ttk.Entry(controls, textvariable=self.ieee_raw_var,
                  font=("Consolas", 10), width=20).pack(side=tk.LEFT)
        ttk.Button(controls, text="Bitmuster prüfen",
                   command=self._inspect_ieee).pack(side=tk.LEFT, padx=6)
        self.format_output = tk.Text(
            formats, height=12, wrap=tk.WORD, bg=DISPLAY, fg="#f5f7fa",
            font=("Consolas", 12), relief=tk.FLAT, padx=12, pady=10)
        self.format_output.grid(row=1, column=0, sticky="nsew",
                                padx=14, pady=(0, 14))
        formats.rowconfigure(1, weight=1)
        self.format_output.configure(state=tk.DISABLED)

    def _build_graph_tab(self) -> None:
        self.graph_tab = self._new_tab("Graph")
        tab = self.graph_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(3, weight=1)
        functions = ttk.Frame(tab, style="Panel.TFrame")
        functions.grid(row=0, column=0, sticky="ew", padx=18, pady=(18, 6))
        for name in ("F1", "F2", "F3"):
            ttk.Label(functions, text=f"{name}(x)", style="Panel.TLabel").pack(
                side=tk.LEFT, padx=(0, 4))
            ttk.Entry(functions, textvariable=self.function_vars[name],
                      width=18, font=("Consolas", 10)).pack(
                          side=tk.LEFT, padx=(0, 10), fill=tk.X, expand=True)

        ranges = ttk.Frame(tab, style="Panel.TFrame")
        ranges.grid(row=1, column=0, sticky="ew", padx=18, pady=6)
        for name in ("xmin", "xmax", "ymin", "ymax"):
            ttk.Label(ranges, text=name, style="Panel.TLabel").pack(side=tk.LEFT)
            ttk.Entry(ranges, textvariable=self.graph_range_vars[name],
                      width=8, font=("Consolas", 9)).pack(
                          side=tk.LEFT, padx=(3, 7))
        ttk.Button(ranges, text="Plotten", style="Accent.TButton",
                   command=self._plot_graph).pack(side=tk.LEFT, padx=(8, 14))
        analysis = ttk.Frame(tab, style="Panel.TFrame")
        analysis.grid(row=2, column=0, sticky="ew", padx=18, pady=4)
        ttk.Label(analysis, text="Analyse", style="Panel.TLabel").pack(side=tk.LEFT)
        ttk.Combobox(analysis, textvariable=self.graph_analysis_var,
                     values=("ROOT", "DERIV", "INTEGR", "XING", "EXTREMA"),
                     state="readonly", width=9).pack(side=tk.LEFT)
        ttk.Combobox(analysis, textvariable=self.graph_function_var,
                     values=("F1", "F2", "F3"), state="readonly",
                     width=4).pack(side=tk.LEFT, padx=3)
        ttk.Combobox(analysis, textvariable=self.graph_second_var,
                     values=("F1", "F2", "F3"), state="readonly",
                     width=4).pack(side=tk.LEFT)
        ttk.Label(analysis, text="x / links", style="Panel.TLabel").pack(
            side=tk.LEFT, padx=(12, 3))
        ttk.Entry(analysis, textvariable=self.graph_left_var, width=9).pack(
            side=tk.LEFT, padx=(5, 2))
        ttk.Label(analysis, text="rechts", style="Panel.TLabel").pack(
            side=tk.LEFT, padx=(8, 3))
        ttk.Entry(analysis, textvariable=self.graph_right_var, width=9).pack(
            side=tk.LEFT, padx=2)
        ttk.Button(analysis, text="Analysieren",
                   command=self._analyze_graph).pack(side=tk.LEFT, padx=4)

        self.graph_canvas = tk.Canvas(tab, bg="#ffffff", highlightthickness=1,
                                      highlightbackground=LINE)
        self.graph_canvas.grid(row=3, column=0, sticky="nsew", padx=18, pady=8)
        ttk.Label(tab, textvariable=self.graph_analysis_result_var,
                  style="Muted.Panel.TLabel").grid(
                      row=4, column=0, sticky="w", padx=18, pady=(0, 14))

    def _build_logic_tab(self) -> None:
        self.logic_tab = self._new_tab("Logik")
        tab = self.logic_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(2, weight=1)
        controls = ttk.Frame(tab, style="Panel.TFrame")
        controls.grid(row=0, column=0, sticky="ew", padx=18, pady=(18, 8))
        ttk.Entry(controls, textvariable=self.logic_expression_var,
                  font=("Consolas", 12)).pack(side=tk.LEFT, fill=tk.X, expand=True)
        actions = ttk.Frame(tab, style="Panel.TFrame")
        actions.grid(row=1, column=0, sticky="ew", padx=18, pady=(0, 8))
        ttk.Label(actions, text="Belegung", style="Panel.TLabel").pack(
            side=tk.LEFT, padx=(12, 4))
        ttk.Spinbox(actions, from_=0, to=63, width=5,
                    textvariable=self.logic_assignment_var).pack(side=tk.LEFT)
        ttk.Button(actions, text="Gatter auswerten",
                   command=self._evaluate_logic).pack(side=tk.LEFT, padx=4)
        ttk.Button(actions, text="Wahrheitstabelle", style="Teal.TButton",
                   command=self._truth_table).pack(side=tk.LEFT, padx=4)
        ttk.Checkbutton(actions, text="vereinfacht",
                        variable=self.logic_simplified_var).pack(side=tk.LEFT)
        ttk.Button(actions, text="DNF",
                   command=lambda: self._logic_form("DNF")).pack(side=tk.LEFT)
        ttk.Button(actions, text="KNF",
                   command=lambda: self._logic_form("KNF")).pack(side=tk.LEFT, padx=4)
        self.logic_output = tk.Text(
            tab, wrap=tk.NONE, bg=DISPLAY, fg="#f5f7fa",
            font=("Consolas", 11), relief=tk.FLAT, padx=12, pady=10)
        self.logic_output.grid(row=2, column=0, sticky="nsew", padx=18, pady=(0, 18))
        self.logic_output.configure(state=tk.DISABLED)

    def _build_units_tab(self) -> None:
        self.units_tab = self._new_tab("Einheiten")
        tab = self.units_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(2, weight=1)
        controls = ttk.Frame(tab, style="Panel.TFrame")
        controls.grid(row=0, column=0, sticky="ew", padx=18, pady=(18, 8))
        categories = ("LENGTH", "AREA", "VOLUME", "MASS", "TIME",
                      "TEMPERATURE", "ANGLE", "PRESSURE", "ENERGY", "POWER")
        self.unit_category_combo = ttk.Combobox(
            controls, textvariable=self.unit_category_var, values=categories,
            state="readonly", width=15)
        self.unit_category_combo.pack(side=tk.LEFT)
        self.unit_category_combo.bind(
            "<<ComboboxSelected>>", lambda _event: self._load_units())
        ttk.Entry(controls, textvariable=self.unit_input_var,
                  font=("Consolas", 11), width=16).pack(side=tk.LEFT, padx=8)
        self.unit_from_combo = ttk.Combobox(controls, state="readonly", width=24)
        self.unit_from_combo.pack(side=tk.LEFT)
        ttk.Label(controls, text="nach", style="Panel.TLabel").pack(
            side=tk.LEFT, padx=6)
        self.unit_to_combo = ttk.Combobox(controls, state="readonly", width=24)
        self.unit_to_combo.pack(side=tk.LEFT)
        ttk.Button(controls, text="Umrechnen", style="Accent.TButton",
                   command=self._convert_unit).pack(side=tk.LEFT, padx=8)
        ttk.Label(tab, textvariable=self.unit_result_var,
                  style="Title.Panel.TLabel").grid(
                      row=1, column=0, sticky="w", padx=18, pady=6)
        constants = ttk.LabelFrame(tab, text="Physikalische Konstanten")
        constants.grid(row=2, column=0, sticky="nsew", padx=18, pady=(6, 18))
        constants.columnconfigure(0, weight=1)
        constants.rowconfigure(0, weight=1)
        self.constants_tree = ttk.Treeview(
            constants, columns=("symbol", "name", "value", "unit", "source"),
            show="headings")
        for name, label, width in (("symbol", "Symbol", 70),
                                   ("name", "Konstante", 190),
                                   ("value", "Wert", 150),
                                   ("unit", "Einheit", 120),
                                   ("source", "Quelle", 130)):
            self.constants_tree.heading(name, text=label)
            self.constants_tree.column(name, width=width,
                                       anchor=tk.E if name == "value" else tk.W)
        self.constants_tree.grid(row=0, column=0, sticky="nsew", padx=8, pady=8)
        ttk.Button(constants, text="Konstanten vom Rechner laden",
                   command=self._load_constants).grid(
                       row=1, column=0, sticky="e", padx=8, pady=(0, 8))

    def _build_complex_tab(self) -> None:
        self.complex_tab = self._new_tab("Komplex")
        tab = self.complex_tab
        tab.columnconfigure(0, weight=1)
        controls = ttk.Frame(tab, style="Panel.TFrame")
        controls.grid(row=0, column=0, sticky="ew", padx=18, pady=(18, 8))
        ttk.Entry(controls, textvariable=self.complex_expression_var,
                  font=("Consolas", 14)).pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Combobox(controls, textvariable=self.complex_angle_var,
                     values=("DEG", "RAD"), state="readonly", width=5).pack(
                         side=tk.LEFT, padx=8)
        ttk.Button(controls, text="Auswerten", style="Accent.TButton",
                   command=self._evaluate_complex).pack(side=tk.LEFT)
        display = tk.Label(tab, textvariable=self.complex_result_var,
                           bg=DISPLAY, fg=AMBER, anchor="nw", justify=tk.LEFT,
                           font=("Consolas", 20, "bold"), padx=18, pady=18)
        display.grid(row=1, column=0, sticky="nsew", padx=18, pady=(8, 18))
        tab.rowconfigure(1, weight=1)

    def _build_symbols_tab(self) -> None:
        self.symbols_tab = self._new_tab("Speicher")
        tab = self.symbols_tab
        tab.columnconfigure(0, weight=1)
        tab.columnconfigure(1, weight=2)
        tab.columnconfigure(2, weight=1)
        tab.rowconfigure(0, weight=1)

        variables = ttk.LabelFrame(tab, text="Variablen A-F")
        variables.grid(row=0, column=0, sticky="nsew", padx=(18, 8), pady=18)
        variables.columnconfigure(1, weight=1)
        for row, name in enumerate("ABCDEF"):
            ttk.Label(variables, text=name, style="Panel.TLabel").grid(
                row=row, column=0, padx=(14, 8), pady=8, sticky="w")
            ttk.Entry(variables, textvariable=self.variable_vars[name],
                      font=("Consolas", 11)).grid(
                row=row, column=1, sticky="ew", padx=(0, 14), pady=8)

        functions = ttk.LabelFrame(tab, text="Benutzerfunktionen")
        functions.grid(row=0, column=1, sticky="nsew", padx=(8, 18), pady=18)
        functions.columnconfigure(1, weight=1)
        for row, name in enumerate(("F1", "F2", "F3")):
            ttk.Label(functions, text=f"{name}(x)", style="Panel.TLabel").grid(
                row=row, column=0, padx=(14, 8), pady=12, sticky="w")
            ttk.Entry(functions, textvariable=self.function_vars[name],
                      font=("Consolas", 11)).grid(
                row=row, column=1, sticky="ew", padx=(0, 14), pady=12)

        tools = ttk.LabelFrame(tab, text="M und Favoriten")
        tools.grid(row=0, column=2, sticky="nsew", padx=(8, 18), pady=18)
        tools.columnconfigure(1, weight=1)
        ttk.Label(tools, text="M", style="Panel.TLabel").grid(
            row=0, column=0, padx=(12, 6), pady=7, sticky="w")
        ttk.Entry(tools, textvariable=self.memory_var, width=14,
                  font=("Consolas", 10)).grid(
            row=0, column=1, sticky="ew", padx=(0, 12), pady=7)
        for row, name in enumerate(self.favorite_vars, start=1):
            ttk.Label(tools, text=name, style="Panel.TLabel").grid(
                row=row, column=0, padx=(12, 6), pady=7, sticky="w")
            ttk.Entry(tools, textvariable=self.favorite_vars[name], width=14,
                      font=("Consolas", 10)).grid(
                row=row, column=1, sticky="ew", padx=(0, 12), pady=7)

        actions = ttk.Frame(tab, style="Panel.TFrame")
        actions.grid(row=1, column=0, columnspan=3, sticky="ew",
                     padx=18, pady=(0, 18))
        ttk.Button(actions, text="Neu laden",
                   command=self._refresh_snapshot).pack(side=tk.LEFT)
        ttk.Button(actions, text="Zum Rechner synchronisieren",
                   style="Accent.TButton", command=self._sync_symbols).pack(
                       side=tk.RIGHT)

    def _build_statistics_tab(self) -> None:
        self.statistics_tab = self._new_tab("Statistik")
        tab = self.statistics_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(1, weight=1)

        toolbar = ttk.Frame(tab, style="Panel.TFrame")
        toolbar.grid(row=0, column=0, sticky="ew", padx=18, pady=(18, 10))
        ttk.Radiobutton(toolbar, text="1VAR", value=1,
                        variable=self.stats_mode_var, style="Mode.Toolbutton",
                        command=self._stats_mode_changed).pack(side=tk.LEFT)
        ttk.Radiobutton(toolbar, text="2VAR", value=2,
                        variable=self.stats_mode_var, style="Mode.Toolbutton",
                        command=self._stats_mode_changed).pack(side=tk.LEFT, padx=(4, 14))
        ttk.Label(toolbar, text="X", style="Panel.TLabel").pack(side=tk.LEFT)
        ttk.Entry(toolbar, textvariable=self.stats_x_var, width=14,
                  font=("Consolas", 10)).pack(side=tk.LEFT, padx=(5, 10))
        ttk.Label(toolbar, text="Y", style="Panel.TLabel").pack(side=tk.LEFT)
        self.stats_y_entry = ttk.Entry(toolbar, textvariable=self.stats_y_var,
                                       width=14, font=("Consolas", 10))
        self.stats_y_entry.pack(side=tk.LEFT, padx=(5, 10))
        ttk.Button(toolbar, text="Zeile hinzufügen",
                   command=self._add_stat_row).pack(side=tk.LEFT)

        table_frame = ttk.Frame(tab, style="Panel.TFrame")
        table_frame.grid(row=1, column=0, sticky="nsew", padx=18)
        table_frame.columnconfigure(0, weight=3)
        table_frame.columnconfigure(2, weight=2)
        table_frame.rowconfigure(0, weight=1)
        self.stats_tree = ttk.Treeview(
            table_frame, columns=("index", "x", "y"), show="headings",
            selectmode="extended",
        )
        self.stats_tree.heading("index", text="#")
        self.stats_tree.heading("x", text="X")
        self.stats_tree.heading("y", text="Y")
        self.stats_tree.column("index", width=55, stretch=False, anchor=tk.CENTER)
        self.stats_tree.column("x", width=180, anchor=tk.E)
        self.stats_tree.column("y", width=180, anchor=tk.E)
        scrollbar = ttk.Scrollbar(table_frame, orient=tk.VERTICAL,
                                  command=self.stats_tree.yview)
        self.stats_tree.configure(yscrollcommand=scrollbar.set)
        self.stats_tree.grid(row=0, column=0, sticky="nsew")
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.stats_analysis_output = tk.Text(
            table_frame, width=34, wrap=tk.WORD, bg=DISPLAY, fg="#f5f7fa",
            font=("Consolas", 10), relief=tk.FLAT, padx=10, pady=8)
        self.stats_analysis_output.grid(row=0, column=2, sticky="nsew",
                                        padx=(10, 0))
        self.stats_analysis_output.configure(state=tk.DISABLED)

        actions = ttk.Frame(tab, style="Panel.TFrame")
        actions.grid(row=2, column=0, sticky="ew", padx=18, pady=(10, 4))
        ttk.Button(actions, text="Auswahl entfernen",
                   command=self._remove_stat_rows).pack(side=tk.LEFT)
        ttk.Button(actions, text="Liste leeren",
                   command=self._clear_stat_rows).pack(side=tk.LEFT, padx=6)
        self.stats_count_label = ttk.Label(actions, text="0 / 32",
                                           style="Muted.Panel.TLabel")
        self.stats_count_label.pack(side=tk.LEFT, padx=10)
        ttk.Button(actions, text="Neu laden",
                   command=self._refresh_snapshot).pack(side=tk.RIGHT, padx=(6, 0))
        ttk.Button(actions, text="Zum Rechner synchronisieren",
                   style="Accent.TButton", command=self._sync_statistics).pack(
                       side=tk.RIGHT)

        analysis_actions = ttk.Frame(tab, style="Panel.TFrame")
        analysis_actions.grid(row=3, column=0, sticky="ew", padx=18,
                              pady=(4, 14))
        ttk.Label(analysis_actions, text="Auswertung",
                  style="Panel.TLabel").pack(side=tk.LEFT, padx=(0, 8))
        ttk.Button(analysis_actions, text="Summary X",
                   command=lambda: self._statistics_analysis(
                       "SUMMARY", "X")).pack(side=tk.LEFT, padx=2)
        ttk.Button(analysis_actions, text="Summary Y",
                   command=lambda: self._statistics_analysis(
                       "SUMMARY", "Y")).pack(side=tk.LEFT, padx=2)
        ttk.Button(analysis_actions, text="Regression",
                   command=lambda: self._statistics_analysis(
                       "REGRESSION")).pack(side=tk.LEFT, padx=2)
        ttk.Button(analysis_actions, text="Histogramm",
                   command=lambda: self._statistics_analysis(
                       "HISTOGRAM")).pack(side=tk.LEFT, padx=2)
        self._stats_mode_changed()

    def _build_basic_tab(self) -> None:
        self.basic_tab = self._new_tab("BASIC")
        tab = self.basic_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(1, weight=1)

        toolbar = ttk.Frame(tab, style="Panel.TFrame")
        toolbar.grid(row=0, column=0, sticky="ew", padx=18, pady=(18, 10))
        ttk.Button(toolbar, text="Datei laden",
                   command=self._load_basic_file).pack(side=tk.LEFT)
        ttk.Button(toolbar, text="Datei speichern",
                   command=self._save_basic_file).pack(side=tk.LEFT, padx=6)

        load_button = ttk.Button(toolbar, text="Vom Rechner laden",
                                 command=self._load_basic_device)
        load_button.pack(side=tk.LEFT, padx=(18, 6))
        sync_button = ttk.Button(toolbar, text="Zum Rechner speichern",
                                 command=self._sync_basic_program)
        sync_button.pack(side=tk.LEFT)
        stop_button = ttk.Button(toolbar, text="Stop",
                                 command=self._stop_basic_program)
        stop_button.pack(side=tk.RIGHT, padx=(6, 0))
        run_button = ttk.Button(toolbar, text="Ausführen",
                                style="Accent.TButton",
                                command=self._run_basic_program)
        run_button.pack(side=tk.RIGHT)
        self.basic_connection_buttons.extend(
            (load_button, sync_button, stop_button, run_button))

        content = ttk.Frame(tab, style="Panel.TFrame")
        content.grid(row=1, column=0, sticky="nsew", padx=18)
        content.columnconfigure(0, weight=3)
        content.columnconfigure(1, weight=2)
        content.rowconfigure(0, weight=1)

        editor_frame = ttk.LabelFrame(content, text="Programm")
        editor_frame.grid(row=0, column=0, sticky="nsew", padx=(0, 8))
        editor_frame.columnconfigure(0, weight=1)
        editor_frame.rowconfigure(0, weight=1)
        self.basic_editor = tk.Text(
            editor_frame, width=48, wrap=tk.NONE, undo=True,
            bg="#ffffff", fg=INK,
            insertbackground=INK, selectbackground="#d9ece9",
            font=("Consolas", 11), relief=tk.FLAT, padx=10, pady=10,
        )
        editor_scroll = ttk.Scrollbar(editor_frame, orient=tk.VERTICAL,
                                      command=self.basic_editor.yview)
        self.basic_editor.configure(yscrollcommand=editor_scroll.set)
        self.basic_editor.grid(row=0, column=0, sticky="nsew")
        editor_scroll.grid(row=0, column=1, sticky="ns")

        output_frame = ttk.LabelFrame(content, text="Ausgabe")
        output_frame.grid(row=0, column=1, sticky="nsew", padx=(8, 0))
        output_frame.columnconfigure(0, weight=1)
        output_frame.rowconfigure(0, weight=1)
        self.basic_output = tk.Text(
            output_frame, width=28, wrap=tk.WORD, bg=DISPLAY, fg="#f5f7fa",
            insertbackground="#ffffff", selectbackground=TEAL,
            font=("Consolas", 11), relief=tk.FLAT, padx=10, pady=10,
        )
        self.basic_output.grid(row=0, column=0, sticky="nsew")
        self.basic_output.configure(state=tk.DISABLED)

        input_row = ttk.Frame(tab, style="Panel.TFrame")
        input_row.grid(row=2, column=0, sticky="ew", padx=18, pady=18)
        ttk.Label(input_row, text="INPUT", style="Panel.TLabel").pack(
            side=tk.LEFT)
        basic_input = ttk.Entry(input_row, textvariable=self.basic_input_var,
                                width=24, font=("Consolas", 10))
        basic_input.pack(side=tk.LEFT, padx=(6, 6))
        basic_input.bind("<Return>",
                         lambda _event: self._send_basic_input())
        input_button = ttk.Button(input_row, text="Eingabe senden",
                                  command=self._send_basic_input)
        input_button.pack(side=tk.LEFT)
        self.basic_connection_buttons.append(input_button)
        ttk.Label(input_row, textvariable=self.basic_status_var,
                  style="Muted.Panel.TLabel").pack(side=tk.RIGHT)

    def _build_history_tab(self) -> None:
        self.history_tab = self._new_tab("Verlauf")
        tab = self.history_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(0, weight=1)
        self.history_tree = ttk.Treeview(
            tab, columns=("index", "expression", "result"), show="headings",
            selectmode="browse",
        )
        self.history_tree.heading("index", text="#")
        self.history_tree.heading("expression", text="Ausdruck")
        self.history_tree.heading("result", text="Ergebnis")
        self.history_tree.column("index", width=55, stretch=False, anchor=tk.CENTER)
        self.history_tree.column("expression", width=420, anchor=tk.W)
        self.history_tree.column("result", width=180, anchor=tk.E)
        self.history_tree.grid(row=0, column=0, sticky="nsew", padx=18, pady=(18, 10))
        self.history_tree.bind("<Double-1>", self._use_history_expression)

        actions = ttk.Frame(tab, style="Panel.TFrame")
        actions.grid(row=1, column=0, sticky="ew", padx=18, pady=(0, 18))
        ttk.Button(actions, text="Neu laden",
                   command=self._refresh_snapshot).pack(side=tk.LEFT)
        ttk.Button(actions, text="Im Rechner verwenden", style="Teal.TButton",
                   command=self._use_history_expression).pack(side=tk.RIGHT)

    def _build_console_tab(self) -> None:
        self.console_tab = self._new_tab("Protokoll")
        tab = self.console_tab
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(1, weight=1)
        command_row = ttk.Frame(tab, style="Panel.TFrame")
        command_row.grid(row=0, column=0, sticky="ew", padx=18, pady=(18, 10))
        command_row.columnconfigure(0, weight=1)
        raw_entry = ttk.Entry(command_row, textvariable=self.raw_command_var,
                              font=("Consolas", 11))
        raw_entry.grid(row=0, column=0, sticky="ew", padx=(0, 8))
        raw_entry.bind("<Return>", lambda _event: self._send_raw_command())
        ttk.Button(command_row, text="Senden", style="Accent.TButton",
                   command=self._send_raw_command).grid(row=0, column=1)

        self.console_text = tk.Text(
            tab, wrap=tk.WORD, bg="#181b1f", fg="#e8eaed",
            insertbackground="#ffffff", selectbackground=TEAL,
            font=("Consolas", 10), relief=tk.FLAT, padx=12, pady=10,
        )
        self.console_text.grid(row=1, column=0, sticky="nsew", padx=18, pady=(0, 18))
        self.console_text.configure(state=tk.DISABLED)

    def _refresh_ports(self) -> None:
        try:
            from serial.tools import list_ports
            ports = sorted(list_ports.comports(), key=lambda item: item.device)
        except ImportError:
            messagebox.showerror(APP_NAME, "PySerial ist nicht installiert.")
            return
        labels = [f"{port.device}  |  {port.description}" for port in ports]
        devices = [port.device for port in ports]
        self.port_combo["values"] = labels
        current = self._selected_port()
        if devices and current not in devices:
            self.port_combo.current(0)
        elif not devices:
            self.port_var.set("")
        self.status_var.set(f"{len(devices)} serieller Port gefunden"
                            if len(devices) == 1
                            else f"{len(devices)} serielle Ports gefunden")

    def _selected_port(self) -> str:
        return self.port_var.get().split("  |  ", 1)[0].strip()

    def _connect(self) -> None:
        port = self._selected_port()
        if not port:
            messagebox.showwarning(APP_NAME, "Kein serieller Port ausgewählt.")
            return

        def operation() -> DeviceSnapshot:
            if self.client is not None:
                self.client.close()
            self.client = SerialClient(port, 115200, 2.0).open()
            try:
                return read_device_snapshot(self.client)
            except Exception:
                self.client.close()
                self.client = None
                raise

        def success(snapshot: DeviceSnapshot) -> None:
            self._set_connected(True, port)
            self._apply_snapshot(snapshot)
            self.root.after(0, self._load_units)

        self._submit(f"Verbinde mit {port}", operation, success)

    def _disconnect(self) -> None:
        if not self.connected:
            return

        def operation() -> None:
            if self.client is not None:
                self.client.close()
                self.client = None

        self._submit("Trenne Verbindung", operation,
                     lambda _value: self._set_connected(False))

    def _refresh_snapshot(self) -> None:
        if not self._require_connection():
            return
        self._submit("Lade Rechnerdaten", lambda: read_device_snapshot(self.client),
                     self._apply_snapshot)

    def _set_result(self, value: str) -> None:
        text = str(value)
        length = len(text)
        size = 28 if length <= 16 else (
            20 if length <= 24 else (14 if length <= 40 else (
                11 if length <= 60 else (9 if length <= 90 else 7))))
        self.result_var.set(text)
        self.result_label.configure(font=("Consolas", size, "bold"))

    def _evaluate(self) -> None:
        if not self._require_connection():
            return
        expression = self.expression_var.get()

        def operation() -> tuple[str, DeviceSnapshot]:
            value = evaluate_expression(self.client, expression)
            return value, read_device_snapshot(self.client)

        def success(result: tuple[str, DeviceSnapshot]) -> None:
            value, snapshot = result
            self._apply_snapshot(snapshot)
            self._set_result(value)

        self._submit("Führe Ausdruck aus", operation, success)

    def _send_expression(self) -> None:
        if not self._require_connection():
            return
        expression = self.expression_var.get()

        def operation() -> str:
            return self.client.command(f"SET EXPR {expression}")

        self._submit("Sende Ausdruckseditor", operation,
                     lambda response: self._append_console(f"< {response}"))

    @staticmethod
    def _replace_text(widget: tk.Text, text: str) -> None:
        widget.configure(state=tk.NORMAL)
        widget.delete("1.0", tk.END)
        widget.insert("1.0", text)
        widget.configure(state=tk.DISABLED)

    def _set_angle(self) -> None:
        if not self._require_connection():
            return
        mode = self.angle_var.get()

        def operation() -> DeviceSnapshot:
            self.client.command(f"SET ANGLE {mode}")
            return read_device_snapshot(self.client)

        self._submit("Setze Winkelmodus", operation, self._apply_snapshot)

    def _set_precision(self) -> None:
        if not self._require_connection():
            return
        mode = self.precision_var.get()

        def operation() -> DeviceSnapshot:
            self.client.command(f"SET PRECISION {mode}")
            return read_device_snapshot(self.client)

        self._submit("Setze Präzisionsmodus", operation,
                     self._apply_snapshot)

    def _programmer_action(self, action: str) -> None:
        if not self._require_connection():
            return
        try:
            base = self.programmer_base_var.get()
            bits = int(self.programmer_bits_var.get())
            value = parse_integer(self.programmer_value_var.get(), base)
            operand = None
            if action in ("AND", "OR", "XOR"):
                operand = parse_integer(self.programmer_operand_var.get(), base)
            elif action in ("BSET", "BCLEAR", "BTOGGLE"):
                operand = int(self.programmer_bit_var.get())
            selected_bit = min(int(self.programmer_bit_var.get()), bits - 1)
            fraction = min(int(self.format_fraction_var.get()), bits - 1)
            signed = self.programmer_signed_var.get()
            if action in ("BSET", "BCLEAR", "BTOGGLE"):
                operand = selected_bit
        except (ValueError, ProtocolError) as error:
            messagebox.showerror(APP_NAME, str(error))
            return

        def operation() -> dict[str, str]:
            result = programmer_operation(
                self.client, action, value, bits, operand)
            self.client.command(f"SET FORMAT {bits} {fraction}")
            self.client.command(
                f"SET PROGRAMMER {result['value']} {base} "
                f"{1 if signed else 0} {selected_bit}")
            return result

        def success(result: dict[str, str]) -> None:
            displayed = result[base.lower()] if base in ("BIN", "HEX") \
                else result["value"]
            self.programmer_value_var.set(displayed)
            self.programmer_bit_var.set(selected_bit)
            self.format_fraction_var.set(fraction)
            lines = [
                f"DEC unsigned  {result['value']}",
                f"DEC signed    {result['signed']}",
                f"HEX           {result['hex']}",
                f"BIN           {result['bin']}",
                f"Carry {result['carry']}    Overflow {result['overflow']}",
            ]
            self._replace_text(self.programmer_output, "\n".join(lines))

        self._submit(f"Programmer {action}", operation, success)

    def _inspect_format(self) -> None:
        if not self._require_connection():
            return
        try:
            base = self.programmer_base_var.get()
            value = parse_integer(self.programmer_value_var.get(), base)
            bits = int(self.programmer_bits_var.get())
            fraction = int(self.format_fraction_var.get())
        except (ValueError, ProtocolError) as error:
            messagebox.showerror(APP_NAME, str(error))
            return

        def operation() -> dict[str, str]:
            result = inspect_number_format(self.client, value, bits, fraction)
            self.client.command(f"SET FORMAT {bits} {fraction}")
            return result

        def success(result: dict[str, str]) -> None:
            self._replace_text(self.format_output, "\n".join((
                f"Unsigned       {result['unsigned']}",
                f"2er-Komplement {result['signed']}",
                f"Q{bits - fraction - 1}.{fraction}          {result['fixed']}",
                f"Float32 bits   0x{result['float32']}",
                f"Float64 bits   0x{result['float64']}",
            )))

        self._submit("Analysiere Zahlenformat", operation, success)

    def _inspect_ieee(self) -> None:
        if not self._require_connection():
            return
        try:
            width = int(self.ieee_width_var.get())
            raw_text = self.ieee_raw_var.get().strip().lower()
            raw = int(raw_text[2:] if raw_text.startswith("0x") else raw_text, 16)
        except ValueError:
            messagebox.showerror(APP_NAME, "IEEE-Bitmuster muss hexadezimal sein.")
            return

        def success(result: dict[str, str]) -> None:
            self._replace_text(self.format_output, "\n".join((
                f"IEEE {result.get('width', width)}  {result['class']}",
                f"Wert          {result['value']}",
                f"Vorzeichen    {result['sign']}",
                f"Exponent raw  {result['rawexp']}",
                f"Exponent      {result['exponent']}",
                f"Mantisse      {result['mantissa']}",
            )))

        self._submit("Prüfe IEEE-Bitmuster",
                     lambda: inspect_ieee(self.client, width, raw), success)

    def _plot_graph(self) -> None:
        if not self._require_connection():
            return
        try:
            ranges = {name: float(variable.get())
                      for name, variable in self.graph_range_vars.items()}
            if (not all(math.isfinite(value) for value in ranges.values()) or
                    ranges["xmin"] >= ranges["xmax"] or
                    ranges["ymin"] >= ranges["ymax"]):
                raise ValueError
        except ValueError:
            messagebox.showerror(APP_NAME, "Ungültiger Graphbereich.")
            return
        functions = {name: value.get().strip()
                     for name, value in self.function_vars.items()}
        variables = {name: value.get() for name, value in self.variable_vars.items()}

        def operation() -> dict[str, list[tuple[float, float]]]:
            synchronize_symbols(self.client, variables, functions)
            self.client.command(
                "SET GRAPH " + " ".join(
                    f"{ranges[name]:.17g}"
                    for name in ("xmin", "xmax", "ymin", "ymax")))
            return {
                name: sample_graph(self.client, name, ranges["xmin"],
                                   ranges["xmax"], 81)
                for name, expression in functions.items() if expression
            }

        self._submit("Plotte Funktionen", operation,
                     lambda points: self._draw_graph(points, ranges))

    def _draw_graph(self, series: dict[str, list[tuple[float, float]]],
                    ranges: dict[str, float]) -> None:
        canvas = self.graph_canvas
        canvas.delete("all")
        width = max(canvas.winfo_width(), 200)
        height = max(canvas.winfo_height(), 160)
        x_min, x_max = ranges["xmin"], ranges["xmax"]
        y_min, y_max = ranges["ymin"], ranges["ymax"]

        def screen(x: float, y: float) -> tuple[float, float]:
            return ((x - x_min) / (x_max - x_min) * width,
                    height - (y - y_min) / (y_max - y_min) * height)

        for index in range(11):
            x = index * width / 10
            y = index * height / 10
            canvas.create_line(x, 0, x, height, fill="#e2e6ea")
            canvas.create_line(0, y, width, y, fill="#e2e6ea")
        if x_min <= 0 <= x_max:
            axis_x, _ = screen(0, 0)
            canvas.create_line(axis_x, 0, axis_x, height, fill="#5f6871", width=2)
        if y_min <= 0 <= y_max:
            _, axis_y = screen(0, 0)
            canvas.create_line(0, axis_y, width, axis_y, fill="#5f6871", width=2)
        colors = {"F1": ACCENT, "F2": TEAL, "F3": "#2867b2"}
        for name, points in series.items():
            segment: list[float] = []
            for x, y in points:
                if not math.isfinite(y) or y < y_min or y > y_max:
                    if len(segment) >= 4:
                        canvas.create_line(*segment, fill=colors[name], width=2)
                    segment = []
                    continue
                sx, sy = screen(x, y)
                segment.extend((sx, sy))
            if len(segment) >= 4:
                canvas.create_line(*segment, fill=colors[name], width=2)
        self.graph_analysis_result_var.set(
            "  ".join(f"{name}: {self.function_vars[name].get()}"
                      for name in series))

    def _analyze_graph(self) -> None:
        if not self._require_connection():
            return
        try:
            left = float(self.graph_left_var.get())
            right = float(self.graph_right_var.get())
        except ValueError:
            messagebox.showerror(APP_NAME, "Analysewerte müssen Zahlen sein.")
            return
        action = self.graph_analysis_var.get()
        first = self.graph_function_var.get()
        second = self.graph_second_var.get()
        values = [left] if action == "DERIV" else [left, right]
        functions = {name: value.get().strip()
                     for name, value in self.function_vars.items()}
        variables = {name: value.get() for name, value in self.variable_vars.items()}

        def operation() -> dict[str, str]:
            synchronize_symbols(self.client, variables, functions)
            return analyze_graph(self.client, action, first, values, second)

        def success(result: dict[str, str]) -> None:
            self.graph_analysis_result_var.set(
                "  ".join(f"{name}={value}" for name, value in result.items()))

        self._submit("Analysiere Graph", operation, success)

    def _evaluate_logic(self) -> None:
        if not self._require_connection():
            return
        expression = self.logic_expression_var.get()
        assignment = int(self.logic_assignment_var.get())

        def success(result: dict[str, str]) -> None:
            self._replace_text(
                self.logic_output,
                f"Ausdruck: {expression}\nBelegung: {result['assignment']:>2}\n"
                f"Ausgang:  {result['value']}")

        self._submit("Simuliere Logikgatter",
                     lambda: evaluate_logic(self.client, expression, assignment),
                     success)

    def _truth_table(self) -> None:
        if not self._require_connection():
            return
        expression = self.logic_expression_var.get()

        def success(table: dict[str, Any]) -> None:
            variables = table["variables"]
            lines = ["  ".join(variables + ["OUT"]),
                     "--" * (len(variables) + 1)]
            for row in table["rows"]:
                lines.append("  ".join(
                    [str(value) for value in row["inputs"] + [row["value"]]]))
            self._replace_text(self.logic_output, "\n".join(lines))

        self._submit("Lese Wahrheitstabelle",
                     lambda: read_truth_table(self.client, expression), success)

    def _logic_form(self, kind: str) -> None:
        if not self._require_connection():
            return
        expression = self.logic_expression_var.get()
        simplified = self.logic_simplified_var.get()
        self._submit(
            f"Berechne {kind}",
            lambda: read_logic_form(self.client, expression, kind, simplified),
            lambda result: self._replace_text(
                self.logic_output,
                f"{kind} ({'vereinfacht' if simplified else 'kanonisch'})\n\n"
                f"{result}"))

    def _load_units(self) -> None:
        if not self._require_connection():
            return
        categories = tuple(self.unit_category_combo["values"])
        category = categories.index(self.unit_category_var.get())

        def success(data: dict[str, Any]) -> None:
            self.unit_cache[category] = data
            labels = [f"{unit['name']} ({unit['symbol']})"
                      for unit in data["units"]]
            self.unit_from_combo["values"] = labels
            self.unit_to_combo["values"] = labels
            if labels:
                self.unit_from_combo.current(0)
                self.unit_to_combo.current(1 if len(labels) > 1 else 0)

        self._submit("Lade Einheiten",
                     lambda: read_unit_category(self.client, category), success)

    def _convert_unit(self) -> None:
        if not self._require_connection():
            return
        categories = tuple(self.unit_category_combo["values"])
        category = categories.index(self.unit_category_var.get())
        try:
            value = float(self.unit_input_var.get())
            from_index = self.unit_from_combo.current()
            to_index = self.unit_to_combo.current()
            if from_index < 0 or to_index < 0:
                raise ValueError
        except ValueError:
            messagebox.showerror(APP_NAME, "Wert und Einheiten auswählen.")
            return

        def success(result: dict[str, str]) -> None:
            self.unit_result_var.set(
                f"{self.unit_input_var.get()} {result['from']} = "
                f"{result['value']} {result['to']}")

        self._submit("Rechne Einheit um",
                     lambda: convert_unit(self.client, category, from_index,
                                          to_index, value), success)

    def _load_constants(self) -> None:
        if not self._require_connection():
            return

        def success(constants: list[dict[str, str]]) -> None:
            self.constants_tree.delete(*self.constants_tree.get_children())
            for item in constants:
                self.constants_tree.insert("", tk.END, values=(
                    item["symbol"], item["name"], item["value"],
                    item["unit"], item["source"],
                ))

        self._submit("Lade Konstanten", lambda: read_constants(self.client), success)

    def _evaluate_complex(self) -> None:
        if not self._require_connection():
            return
        expression = self.complex_expression_var.get()
        angle = self.complex_angle_var.get()

        def success(result: dict[str, str]) -> None:
            self.complex_result_var.set(
                f"Kartesisch\n{result['cart']}\n\nPolar\n{result['polar']}\n\n"
                f"Real = {result['real']}\nImag = {result['imag']}")

        self._submit("Berechne komplexen Ausdruck",
                     lambda: evaluate_complex(self.client, expression, angle),
                     success)

    def _statistics_analysis(self, action: str, axis: str = "X") -> None:
        if not self._require_connection():
            return
        mode = self.stats_mode_var.get()
        values = []
        for item in self.stats_tree.get_children():
            row = self.stats_tree.item(item, "values")
            values.append([row[1]] if mode == 1 else [row[1], row[2]])

        def operation() -> dict[str, str]:
            synchronize_statistics(self.client, mode, values)
            return analyze_statistics(self.client, action, axis)

        def success(result: dict[str, str]) -> None:
            self._replace_text(self.stats_analysis_output, "\n".join(
                f"{name:<12} {value}" for name, value in result.items()))

        self._submit(f"Statistik {action}", operation, success)

    def _sync_symbols(self) -> None:
        if not self._require_connection():
            return
        variables = {name: value.get() for name, value in self.variable_vars.items()}
        functions = {name: value.get() for name, value in self.function_vars.items()}

        memory = self.memory_var.get()
        favorites = {name: value.get()
                     for name, value in self.favorite_vars.items()}

        def operation() -> DeviceSnapshot:
            import_state(self.client, {
                "variables": variables,
                "functions": functions,
                "memory": memory,
                "favorites": favorites,
            })
            return read_device_snapshot(self.client)

        self._submit("Synchronisiere Speicher", operation, self._apply_snapshot)

    def _sync_statistics(self) -> None:
        if not self._require_connection():
            return
        mode = self.stats_mode_var.get()
        values = []
        for item in self.stats_tree.get_children():
            row = self.stats_tree.item(item, "values")
            values.append([row[1]] if mode == 1 else [row[1], row[2]])

        def operation() -> DeviceSnapshot:
            synchronize_statistics(self.client, mode, values)
            return read_device_snapshot(self.client)

        self._submit("Synchronisiere Statistik", operation, self._apply_snapshot)

    def _basic_source(self) -> str:
        return self.basic_editor.get("1.0", "end-1c")

    def _set_basic_source(self, program: list[str]) -> None:
        self.basic_editor.delete("1.0", tk.END)
        if program:
            self.basic_editor.insert("1.0", "\n".join(program) + "\n")

    def _apply_basic_result(self, result: BasicRunResult) -> None:
        status = result.status
        self.basic_status_var.set(
            f"{status.get('state', '?')}  {status.get('status', '?')}  "
            f"{status.get('steps', '0')} Schritte")
        self.basic_output.configure(state=tk.NORMAL)
        self.basic_output.delete("1.0", tk.END)
        if result.output:
            self.basic_output.insert("1.0", "\n".join(result.output) + "\n")
        self.basic_output.configure(state=tk.DISABLED)

    def _load_basic_file(self) -> None:
        selected = filedialog.askopenfilename(
            parent=self.root, title="BASIC-Programm laden",
            filetypes=(("BASIC", "*.bas"), ("Text", "*.txt"),
                       ("Alle Dateien", "*.*")),
        )
        if not selected:
            return
        try:
            source = Path(selected).read_text(encoding="ascii")
            program = normalize_basic_program(source)
        except (OSError, UnicodeError, ProtocolError) as error:
            messagebox.showerror(APP_NAME, str(error))
            return
        self._set_basic_source(program)
        self.status_var.set(f"BASIC-Datei geladen: {selected}")

    def _save_basic_file(self) -> None:
        try:
            program = normalize_basic_program(self._basic_source())
        except ProtocolError as error:
            messagebox.showerror(APP_NAME, str(error))
            return
        selected = filedialog.asksaveasfilename(
            parent=self.root, title="BASIC-Programm speichern",
            defaultextension=".bas", filetypes=(("BASIC", "*.bas"),
                                                ("Text", "*.txt")),
            initialfile="program.bas",
        )
        if not selected:
            return
        try:
            Path(selected).write_text(
                ("\n".join(program) + "\n") if program else "",
                encoding="ascii")
        except OSError as error:
            messagebox.showerror(APP_NAME, str(error))
            return
        self.status_var.set(f"BASIC-Datei gespeichert: {selected}")

    def _load_basic_device(self) -> None:
        if not self._require_connection():
            return
        self._submit("Lade BASIC-Programm",
                     lambda: read_basic_program(self.client),
                     self._set_basic_source)

    def _sync_basic_program(self) -> None:
        if not self._require_connection():
            return
        source = self._basic_source()

        def success(program: list[str]) -> None:
            self._set_basic_source(program)
            self.basic_status_var.set("GESPEICHERT")

        self._submit("Speichere BASIC-Programm",
                     lambda: synchronize_basic_program(self.client, source),
                     success)

    def _run_basic_program(self) -> None:
        if not self._require_connection():
            return
        if self.busy:
            self.root.bell()
            self.status_var.set("Ein Vorgang läuft bereits")
            return
        source = self._basic_source()
        self.basic_cancel.clear()
        self.basic_running = True

        def success(result: BasicRunResult) -> None:
            self.basic_running = False
            self._apply_basic_result(result)

        def failure(error: Exception) -> None:
            self.basic_running = False
            messagebox.showerror(APP_NAME, str(error))

        self._submit(
            "Führe BASIC-Programm aus",
            lambda: run_basic_program(
                self.client, source, self.basic_cancel.is_set),
            success, failure)

    def _stop_basic_program(self) -> None:
        if not self._require_connection():
            return
        if self.basic_running:
            self.basic_cancel.set()
            self.basic_status_var.set("STOP ANGEFORDERT")
            return
        self._submit("Stoppe BASIC-Programm",
                     lambda: stop_basic_program(self.client),
                     self._apply_basic_result)

    def _send_basic_input(self) -> None:
        if not self._require_connection():
            return
        if self.busy:
            self.root.bell()
            self.status_var.set("Ein Vorgang läuft bereits")
            return
        value = self.basic_input_var.get()
        self.basic_cancel.clear()
        self.basic_running = True

        def success(result: BasicRunResult) -> None:
            self.basic_running = False
            self.basic_input_var.set("")
            self._apply_basic_result(result)

        def failure(error: Exception) -> None:
            self.basic_running = False
            messagebox.showerror(APP_NAME, str(error))

        self._submit("Sende BASIC-Eingabe",
                     lambda: continue_basic_program(
                         self.client, value, self.basic_cancel.is_set),
                     success, failure)

    def _export_json(self) -> None:
        if not self._require_connection():
            return
        selected = filedialog.asksaveasfilename(
            parent=self.root, title="Rechnerdaten exportieren",
            defaultextension=".json", filetypes=(("JSON", "*.json"),),
            initialfile="pico-calculator-state.json",
        )
        if not selected:
            return
        path = Path(selected)

        def operation() -> Path:
            data = export_state(self.client)
            path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n",
                            encoding="utf-8")
            return path

        self._submit("Exportiere JSON", operation,
                     lambda result: self.status_var.set(f"Exportiert: {result}"))

    def _import_json(self) -> None:
        if not self._require_connection():
            return
        selected = filedialog.askopenfilename(
            parent=self.root, title="Rechnerdaten importieren",
            filetypes=(("JSON", "*.json"), ("Alle Dateien", "*.*")),
        )
        if not selected:
            return
        path = Path(selected)

        def operation() -> DeviceSnapshot:
            data = json.loads(path.read_text(encoding="utf-8"))
            if not isinstance(data, dict):
                raise ProtocolError("JSON-Wurzel muss ein Objekt sein")
            import_state(self.client, data)
            return read_device_snapshot(self.client)

        self._submit("Importiere JSON", operation, self._apply_snapshot)

    def _send_raw_command(self) -> None:
        if not self._require_connection():
            return
        command = self.raw_command_var.get().strip()
        if not command:
            return
        self._append_console(f"> {command}")

        def operation() -> tuple[str, DeviceSnapshot]:
            response = self.client.command(command)
            return response, read_device_snapshot(self.client)

        def success(result: tuple[str, DeviceSnapshot]) -> None:
            response, snapshot = result
            self._append_console(f"< {response}")
            self._apply_snapshot(snapshot)
            self.raw_command_var.set("")

        self._submit("Sende Protokollbefehl", operation, success,
                     lambda error: self._append_console(f"< {error}"))

    def _press_key(self, key: str) -> None:
        if key == "AC":
            self.expression_var.set("")
            self.expression_entry.icursor(0)
        elif key == "DEL":
            cursor = self.expression_entry.index(tk.INSERT)
            if cursor > 0:
                self.expression_entry.delete(cursor - 1)
        else:
            self.expression_entry.insert(tk.INSERT, key)
        self.expression_entry.focus_set()

    def _add_stat_row(self) -> None:
        if len(self.stats_tree.get_children()) >= 32:
            messagebox.showwarning(APP_NAME, "Die Statistikliste ist voll.")
            return
        try:
            x_value = float(self.stats_x_var.get())
            y_value = float(self.stats_y_var.get()) if self.stats_mode_var.get() == 2 else 0.0
            if not math.isfinite(x_value) or not math.isfinite(y_value):
                raise ValueError
        except ValueError:
            messagebox.showerror(APP_NAME, "X und Y müssen endliche Zahlen sein.")
            return
        values = [len(self.stats_tree.get_children()), format_number(x_value)]
        if self.stats_mode_var.get() == 2:
            values.append(format_number(y_value))
        self.stats_tree.insert("", tk.END, values=values)
        self.stats_x_var.set("")
        self.stats_y_var.set("")
        self._update_stats_count()

    def _remove_stat_rows(self) -> None:
        for item in self.stats_tree.selection():
            self.stats_tree.delete(item)
        self._renumber_stats()

    def _clear_stat_rows(self) -> None:
        self.stats_tree.delete(*self.stats_tree.get_children())
        self._update_stats_count()

    def _renumber_stats(self) -> None:
        for index, item in enumerate(self.stats_tree.get_children()):
            values = list(self.stats_tree.item(item, "values"))
            values[0] = index
            self.stats_tree.item(item, values=values)
        self._update_stats_count()

    def _stats_mode_changed(self) -> None:
        mode = self.stats_mode_var.get()
        self.stats_y_entry.configure(state="normal" if mode == 2 else "disabled")
        self.stats_tree.configure(displaycolumns=("index", "x", "y")
                                  if mode == 2 else ("index", "x"))
        for item in self.stats_tree.get_children():
            values = list(self.stats_tree.item(item, "values"))
            if mode == 2 and len(values) < 3:
                values.append("0")
            elif mode == 1 and len(values) > 2:
                values = values[:2]
            self.stats_tree.item(item, values=values)

    def _use_history_expression(self, _event: Any = None) -> None:
        selected = self.history_tree.selection()
        if not selected:
            return
        values = self.history_tree.item(selected[0], "values")
        self.expression_var.set(values[1])
        self.notebook.select(self.calculator_tab)
        self.expression_entry.focus_set()

    def _use_session_expression(self, _event: Any = None) -> None:
        selected = self.session_tree.selection()
        if selected:
            self.expression_var.set(self.session_tree.item(selected[0], "values")[0])
            self.expression_entry.focus_set()

    def _apply_snapshot(self, snapshot: DeviceSnapshot) -> None:
        self.snapshot = snapshot
        info = snapshot.info
        diag = snapshot.diagnostics
        state = snapshot.state
        self.device_vars["Modell"].set(info.get("model", "-"))
        self.device_vars["Firmware"].set(info.get("firmware", "-"))
        self.device_vars["Protokoll"].set(info.get("protocol", "-"))
        self.device_vars["Winkel"].set(diag.get("angle", "-"))
        self.device_vars["Präzision"].set(diag.get("precision", "-"))
        self.device_vars["Seite"].set(diag.get("page", "-"))
        self.device_vars["Verlauf"].set(diag.get("history", "0"))
        self.device_vars["Statistik"].set(
            f"{diag.get('mode', '-')}VAR / {diag.get('stats', '0')}")
        self.device_vars["BASIC"].set(
            f"{diag.get('basic', '0')} / {diag.get('basic_state', '-')}")
        self.angle_var.set(str(state.get("angle", diag.get("angle", "DEG"))))
        self.precision_var.set(str(
            state.get("precision", diag.get("precision", "HIGH"))))
        self.complex_angle_var.set(self.angle_var.get())
        self.expression_var.set(str(state.get("expression", "")))
        self._set_result(format_number(
            state.get("result_text", state.get("result", 0))))
        for name, value in state.get("variables", {}).items():
            if name in self.variable_vars:
                self.variable_vars[name].set(format_number(value))
        for name, value in state.get("functions", {}).items():
            if name in self.function_vars:
                self.function_vars[name].set(str(value))
        self.memory_var.set(format_number(state.get("memory", 0)))
        for name, value in state.get("favorites", {}).items():
            if name in self.favorite_vars:
                self.favorite_vars[name].set(str(value))
        programmer = state.get("programmer", {})
        self.programmer_value_var.set(str(programmer.get("value", 0)))
        self.programmer_base_var.set(str(programmer.get("base", "DEC")))
        self.programmer_signed_var.set(bool(programmer.get("signed", False)))
        self.programmer_bit_var.set(int(programmer.get("selected_bit", 0)))
        number_format = state.get("number_format", {})
        self.programmer_bits_var.set(int(number_format.get("bits", 64)))
        self.format_fraction_var.set(int(number_format.get("fraction", 16)))
        graph = state.get("graph", {})
        for name, variable in self.graph_range_vars.items():
            if name in graph:
                variable.set(format_number(graph[name]))
        self._load_history(state.get("history", []))
        self._load_statistics(state.get("statistics", {}))
        self._set_basic_source(state.get("program", []))

    def _load_history(self, history: list[dict[str, Any]]) -> None:
        self.history_tree.delete(*self.history_tree.get_children())
        self.session_tree.delete(*self.session_tree.get_children())
        for entry in history:
            expression = str(entry.get("expression", ""))
            result = str(entry.get("result", format_number(entry.get("value", 0))))
            self.history_tree.insert("", tk.END, values=(
                entry.get("index", ""), expression, result,
            ))
        for entry in reversed(history[-8:]):
            self.session_tree.insert("", tk.END, values=(
                entry.get("expression", ""), entry.get("result", ""),
            ))

    def _load_statistics(self, statistics: dict[str, Any]) -> None:
        mode = int(statistics.get("mode", 1))
        self.stats_mode_var.set(mode if mode in (1, 2) else 1)
        self.stats_tree.delete(*self.stats_tree.get_children())
        for index, row in enumerate(statistics.get("values", [])):
            values = [index, format_number(row[0])]
            if self.stats_mode_var.get() == 2:
                values.append(format_number(row[1]))
            self.stats_tree.insert("", tk.END, values=values)
        self._stats_mode_changed()
        self._update_stats_count()

    def _update_stats_count(self) -> None:
        self.stats_count_label.configure(
            text=f"{len(self.stats_tree.get_children())} / 32")

    def _set_connected(self, connected: bool, port: str = "") -> None:
        self.connected = connected
        self.connection_var.set(f"Verbunden: {port}" if connected else "Nicht verbunden")
        self.status_canvas.itemconfigure(self.status_dot,
                                         fill=TEAL if connected else MUTED)
        self.connect_button.configure(state="disabled" if connected else "normal")
        self.disconnect_button.configure(state="normal" if connected else "disabled")
        state = "normal" if connected else "disabled"
        for button in (self.sync_button, self.export_button, self.import_button):
            button.configure(state=state)
        for button in self.basic_connection_buttons:
            button.configure(state=state)
        if not connected:
            for variable in self.device_vars.values():
                variable.set("-")

    def _require_connection(self) -> bool:
        if self.connected:
            return True
        messagebox.showwarning(APP_NAME, "Zuerst mit dem Rechner verbinden.")
        return False

    def _submit(self, description: str, operation: Callable[[], Any],
                on_success: Callable[[Any], None] | None = None,
                on_error: Callable[[Exception], None] | None = None) -> None:
        if self.busy:
            self.root.bell()
            self.status_var.set("Ein Vorgang läuft bereits")
            return
        self.busy = True
        self.status_var.set(description)
        self.progress.start(12)
        task = (description, operation, on_success, on_error)
        self.tasks.put(task)

    def _worker_loop(self) -> None:
        while True:
            task = self.tasks.get()
            if task is None:
                if self.client is not None:
                    self.client.close()
                return
            try:
                value = task[1]()
                self.results.put((task, value, None))
            except Exception as error:
                self.results.put((task, None, error))

    def _poll_results(self) -> None:
        try:
            while True:
                task, value, error = self.results.get_nowait()
                description, _operation, on_success, on_error = task
                self.busy = False
                self.progress.stop()
                if error is None:
                    self.status_var.set(f"{description}: OK")
                    self._append_console(f"[{time.strftime('%H:%M:%S')}] {description}: OK")
                    if on_success is not None:
                        on_success(value)
                else:
                    self.status_var.set(f"{description}: Fehler")
                    self._append_console(f"[{time.strftime('%H:%M:%S')}] {error}")
                    if on_error is not None:
                        on_error(error)
                    else:
                        messagebox.showerror(APP_NAME, str(error))
        except queue.Empty:
            pass
        self.root.after(50, self._poll_results)

    def _append_console(self, line: str) -> None:
        self.console_text.configure(state=tk.NORMAL)
        self.console_text.insert(tk.END, line + "\n")
        self.console_text.see(tk.END)
        self.console_text.configure(state=tk.DISABLED)

    def _close(self) -> None:
        self.tasks.put(None)
        self.root.destroy()


def main() -> None:
    enable_native_dpi()
    root = tk.Tk()
    CalculatorLinkApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
