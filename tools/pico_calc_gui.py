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

from pico_calc_cli import ProtocolError, SerialClient, export_state, import_state
from pico_calc_gui_model import (
    DeviceSnapshot,
    evaluate_expression,
    format_number,
    read_device_snapshot,
    synchronize_statistics,
    synchronize_symbols,
)


APP_NAME = "Pico Calculator Link"
APP_VERSION = "1.0"
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
        self.device_vars = {
            name: tk.StringVar(value="-")
            for name in ("Modell", "Firmware", "Protokoll", "Winkel",
                         "Seite", "Verlauf", "Statistik")
        }
        self.variable_vars = {
            name: tk.StringVar(value="0") for name in "ABCDEF"
        }
        self.function_vars = {
            name: tk.StringVar() for name in ("F1", "F2", "F3")
        }

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
        style.configure("TNotebook.Tab", font=("Segoe UI Semibold", 10),
                        padding=(16, 9), background="#dfe3e8")
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
        self._build_symbols_tab()
        self._build_statistics_tab()
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
        tk.Label(display, textvariable=self.result_var, bg=DISPLAY, fg=AMBER,
                 anchor="e", font=("Consolas", 28, "bold")).grid(
            row=1, column=0, sticky="ew", padx=18, pady=(0, 12))
        self.expression_entry.bind("<Return>", lambda _event: self._evaluate())

        actions = ttk.Frame(tab, style="Panel.TFrame")
        actions.grid(row=1, column=0, sticky="ew", padx=18, pady=(0, 10))
        ttk.Button(actions, text="Editor zum Rechner",
                   command=self._send_expression).pack(side=tk.LEFT)
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

    def _build_symbols_tab(self) -> None:
        self.symbols_tab = self._new_tab("Speicher")
        tab = self.symbols_tab
        tab.columnconfigure(0, weight=1)
        tab.columnconfigure(1, weight=2)
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

        actions = ttk.Frame(tab, style="Panel.TFrame")
        actions.grid(row=1, column=0, columnspan=2, sticky="ew",
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
        table_frame.columnconfigure(0, weight=1)
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

        actions = ttk.Frame(tab, style="Panel.TFrame")
        actions.grid(row=2, column=0, sticky="ew", padx=18, pady=18)
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
        self._stats_mode_changed()

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

    def _evaluate(self) -> None:
        if not self._require_connection():
            return
        expression = self.expression_var.get()

        def operation() -> tuple[float, DeviceSnapshot]:
            value = evaluate_expression(self.client, expression)
            return value, read_device_snapshot(self.client)

        def success(result: tuple[float, DeviceSnapshot]) -> None:
            value, snapshot = result
            self._apply_snapshot(snapshot)
            self.result_var.set(format_number(value))

        self._submit("Führe Ausdruck aus", operation, success)

    def _send_expression(self) -> None:
        if not self._require_connection():
            return
        expression = self.expression_var.get()

        def operation() -> str:
            return self.client.command(f"SET EXPR {expression}")

        self._submit("Sende Ausdruckseditor", operation,
                     lambda response: self._append_console(f"< {response}"))

    def _sync_symbols(self) -> None:
        if not self._require_connection():
            return
        variables = {name: value.get() for name, value in self.variable_vars.items()}
        functions = {name: value.get() for name, value in self.function_vars.items()}

        def operation() -> DeviceSnapshot:
            synchronize_symbols(self.client, variables, functions)
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
        self.device_vars["Seite"].set(diag.get("page", "-"))
        self.device_vars["Verlauf"].set(diag.get("history", "0"))
        self.device_vars["Statistik"].set(
            f"{diag.get('mode', '-')}VAR / {diag.get('stats', '0')}")
        self.expression_var.set(str(state.get("expression", "")))
        self.result_var.set(format_number(state.get("result", 0)))
        for name, value in state.get("variables", {}).items():
            if name in self.variable_vars:
                self.variable_vars[name].set(format_number(value))
        for name, value in state.get("functions", {}).items():
            if name in self.function_vars:
                self.function_vars[name].set(str(value))
        self._load_history(state.get("history", []))
        self._load_statistics(state.get("statistics", {}))

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
