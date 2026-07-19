import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from pico_calc_cli import ProtocolError, export_state, import_state


class FakeClient:
    def __init__(self):
        self.commands = []

    def command(self, command):
        self.commands.append(command)
        responses = {
            "GET RESULT": "OK RESULT\t42",
            "GET EXPR": "OK EXPR\t6*7",
            "GET HISTORY": "OK HISTORY\t1",
            "GET HISTORY 0": "OK HISTORY\t0\t42\t6*7\t42",
            "GET STATS": "OK STATS\t2\t2",
            "GET STATS 0": "OK STATS\t0\t1\t3",
            "GET STATS 1": "OK STATS\t1\t2\t5",
        }
        if command.startswith("GET VAR "):
            name = command[-1]
            return f"OK VAR\t{name}\t0"
        if command.startswith("GET FUNC "):
            name = command[-2:]
            return f"OK FUNC\t{name}\tx+1"
        if command.startswith("GET"):
            return responses[command]
        return "OK"


class DependentFunctionClient(FakeClient):
    def __init__(self):
        super().__init__()
        self.f2_defined = False

    def command(self, command):
        if command == "SET FUNC F1 f2(x)+1" and not self.f2_defined:
            self.commands.append(command)
            raise ProtocolError("ERR INVALID_FUNC")
        if command == "SET FUNC F2 x*2":
            self.f2_defined = True
        return super().command(command)


class CliStateTests(unittest.TestCase):
    def test_export_state(self):
        exported = export_state(FakeClient())
        self.assertEqual(exported["result"], 42.0)
        self.assertEqual(exported["expression"], "6*7")
        self.assertEqual(exported["functions"]["F1"], "x+1")
        self.assertEqual(exported["history"][0]["value"], 42.0)
        self.assertEqual(exported["statistics"]["values"], [[1.0, 3.0], [2.0, 5.0]])

    def test_import_state(self):
        client = FakeClient()
        import_state(client, {
            "format": "pico-sbc-calculator-state",
            "expression": "A+1",
            "variables": {"A": 3.5},
            "functions": {"F1": "x+A"},
            "statistics": {"mode": 2, "values": [[1, 3], [2, 5]]},
        })
        self.assertIn("SET EXPR A+1", client.commands)
        self.assertIn("SET VAR A 3.5", client.commands)
        self.assertIn("SET FUNC F1 x+A", client.commands)
        self.assertEqual(client.commands[-2:], ["STAT ADD 1 3", "STAT ADD 2 5"])

    def test_import_rejects_bad_rows(self):
        with self.assertRaises(ProtocolError):
            import_state(FakeClient(), {
                "statistics": {"mode": 2, "values": [[1]]},
            })

    def test_import_retries_dependent_functions(self):
        client = DependentFunctionClient()
        import_state(client, {
            "functions": {"F1": "f2(x)+1", "F2": "x*2"},
        })
        self.assertEqual(client.commands.count("SET FUNC F1 f2(x)+1"), 2)
        self.assertTrue(client.f2_defined)


if __name__ == "__main__":
    unittest.main()
