import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from pico_calc_cli import ProtocolError
from pico_calc_gui_model import (
    evaluate_expression,
    format_number,
    parse_properties,
    read_device_snapshot,
    synchronize_statistics,
    synchronize_symbols,
)


class FakeClient:
    def __init__(self):
        self.commands = []

    def command(self, command):
        self.commands.append(command)
        responses = {
            "INFO": "OK INFO\tprotocol=1\tfirmware=1.1.0\tmodel=scientific-calculator",
            "DIAG": "OK DIAG\tpage=0\tangle=DEG\thistory=1\tstats=1\tmode=1",
            "GET RESULT": "OK RESULT\t42",
            "GET EXPR": "OK EXPR\t6*7",
            "GET HISTORY": "OK HISTORY\t1",
            "GET HISTORY 0": "OK HISTORY\t0\t42\t6*7\t42",
            "GET STATS": "OK STATS\t1\t1",
            "GET STATS 0": "OK STATS\t0\t3",
            "EVAL sqrt(4)": "OK RESULT\t2",
        }
        if command.startswith("GET VAR "):
            return f"OK VAR\t{command[-1]}\t0"
        if command.startswith("GET FUNC "):
            return f"OK FUNC\t{command[-2:]}\tx+1"
        return responses.get(command, "OK")


class GuiModelTests(unittest.TestCase):
    def test_parse_properties(self):
        parsed = parse_properties(
            "OK INFO\tprotocol=1\tfirmware=1.1.0", "INFO"
        )
        self.assertEqual(parsed, {"protocol": "1", "firmware": "1.1.0"})
        with self.assertRaises(ProtocolError):
            parse_properties("OK INFO\tbroken", "INFO")

    def test_read_device_snapshot(self):
        snapshot = read_device_snapshot(FakeClient())
        self.assertEqual(snapshot.info["firmware"], "1.1.0")
        self.assertEqual(snapshot.diagnostics["angle"], "DEG")
        self.assertEqual(snapshot.state["history"][0]["expression"], "6*7")

    def test_evaluate_expression(self):
        self.assertEqual(evaluate_expression(FakeClient(), "sqrt(4)"), 2.0)
        with self.assertRaises(ProtocolError):
            evaluate_expression(FakeClient(), "   ")

    def test_synchronize_symbols_and_statistics(self):
        client = FakeClient()
        synchronize_symbols(client, {"A": 3.5}, {"F1": "x+A"})
        synchronize_statistics(client, 2, [[1, 3], [2, 5]])
        self.assertIn("SET VAR A 3.5", client.commands)
        self.assertIn("SET FUNC F1 x+A", client.commands)
        self.assertEqual(client.commands[-2:], ["STAT ADD 1 3", "STAT ADD 2 5"])

    def test_format_number(self):
        self.assertEqual(format_number(1.0 / 3.0), "0.333333333333")


if __name__ == "__main__":
    unittest.main()
