import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from pico_calc_cli import (
    ProtocolError,
    SerialClient,
    export_state,
    import_state,
    normalize_circuit,
    normalize_basic_program,
    synchronize_circuit,
    synchronize_basic_program,
)


class FakeClient:
    def __init__(self):
        self.commands = []
        self.circuit_add_index = 0

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
            "GET BASIC": "OK BASIC\t2",
            "GET BASIC 0": "OK BASIC\t0\t10\tPRINT \"HELLO\"",
            "GET BASIC 1": "OK BASIC\t1\t20\tEND",
            "GET ANGLE": "OK ANGLE\tDEG",
            "GET PRECISION": "OK PRECISION\tHIGH\t80",
            "GET MEMORY": "OK MEMORY\t7.5",
            "GET PROGRAMMER": (
                "OK PROGRAMMER_STATE\tvalue=255\tbase=HEX\tsigned=1\tbit=7"
            ),
            "GET FORMAT": "OK FORMAT_STATE\tbits=8\tfraction=4",
            "GET GRAPH": (
                "OK GRAPH\txmin=-4\txmax=4\tymin=-3\tymax=3"
                "\ttable=0\tstep=1"
            ),
        }
        if command.startswith("GET VAR "):
            name = command[-1]
            return f"OK VAR\t{name}\t0"
        if command.startswith("GET FUNC "):
            name = command[-2:]
            return f"OK FUNC\t{name}\tx+1"
        if command.startswith("GET FAVORITE "):
            index = command[-1]
            return f"OK FAVORITE\t{index}\tsin("
        if command == "MODULE CIRCUIT INFO":
            return (
                "OK CIRCUIT_INFO\tnodes=2\twires=1\tnode_capacity=24"
                "\twire_capacity=48\tworld_width=1600\tworld_height=1200"
                "\tviewport_x=20\tviewport_y=30\tzoom=150\tcycle=0"
                "\tnext_input=1\tnext_output=1\tnext_gate=0"
            )
        if command.startswith("MODULE CIRCUIT NODE "):
            index = int(command.rsplit(" ", 1)[1])
            if index == 0:
                return (
                    "OK CIRCUIT_NODE\tindex=0\tused=1\ttype=INPUT"
                    "\tx=20\ty=80\tinput=1\toutput=1\tlabel=A"
                )
            if index == 1:
                return (
                    "OK CIRCUIT_NODE\tindex=1\tused=1\ttype=OUTPUT"
                    "\tx=220\ty=80\tinput=0\toutput=1\tlabel=Y"
                )
            return f"OK CIRCUIT_NODE\tindex={index}\tused=0"
        if command.startswith("MODULE CIRCUIT WIRE "):
            index = int(command.rsplit(" ", 1)[1])
            if index == 0:
                return (
                    "OK CIRCUIT_WIRE\tindex=0\tused=1\tsource=0"
                    "\tdestination=1\tinput=0"
                )
            return f"OK CIRCUIT_WIRE\tindex={index}\tused=0"
        if command.startswith("MODULE CIRCUIT ADD "):
            parts = command.split()
            index = self.circuit_add_index
            self.circuit_add_index += 1
            return (
                f"OK CIRCUIT_NODE\tindex={index}\tused=1\ttype={parts[3]}"
                f"\tx={parts[4]}\ty={parts[5]}\tinput={parts[6]}"
                f"\toutput=0\tlabel={parts[7]}"
            )
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
    def test_protocol_line_capacity_matches_firmware(self):
        client = SerialClient("unused", 115200, 1.0)
        with self.assertRaisesRegex(ProtocolError, "255 Zeichen"):
            client.command("X" * 256)

    def test_export_state(self):
        exported = export_state(FakeClient())
        self.assertEqual(exported["result"], 42.0)
        self.assertEqual(exported["result_text"], "42")
        self.assertEqual(exported["expression"], "6*7")
        self.assertEqual(exported["functions"]["F1"], "x+1")
        self.assertEqual(exported["history"][0]["value"], 42.0)
        self.assertEqual(exported["statistics"]["values"], [[1.0, 3.0], [2.0, 5.0]])
        self.assertEqual(exported["program"], ['10 PRINT "HELLO"', "20 END"])
        self.assertEqual(exported["angle"], "DEG")
        self.assertEqual(exported["precision"], "HIGH")
        self.assertEqual(exported["memory"], "7.5")
        self.assertEqual(exported["programmer"]["value"], 255)
        self.assertEqual(exported["number_format"], {"bits": 8, "fraction": 4})
        self.assertEqual(exported["favorites"]["FAV1"], "sin(")
        self.assertEqual(exported["graph"]["xmin"], -4.0)
        self.assertEqual(exported["version"], 6)
        self.assertEqual(exported["circuit"]["zoom"], 150)
        self.assertTrue(exported["circuit"]["nodes"][1]["output"])

    def test_import_state(self):
        client = FakeClient()
        import_state(client, {
            "format": "pico-sbc-calculator-state",
            "expression": "A+1",
            "variables": {"A": 3.5},
            "functions": {"F1": "x+A"},
            "statistics": {"mode": 2, "values": [[1, 3], [2, 5]]},
            "program": ['10 PRINT "HELLO"', "20 END"],
            "angle": "RAD",
            "precision": "ULTRA",
            "memory": 2.5,
            "favorites": {"FAV1": "atan("},
            "number_format": {"bits": 16, "fraction": 8},
            "programmer": {
                "value": 255, "base": "HEX", "signed": True,
                "selected_bit": 7,
            },
            "graph": {"xmin": -2, "xmax": 2, "ymin": -1, "ymax": 1},
        })
        self.assertIn("SET EXPR A+1", client.commands)
        self.assertIn("SET VAR A 3.5", client.commands)
        self.assertIn("SET FUNC F1 x+A", client.commands)
        self.assertIn("SET ANGLE RAD", client.commands)
        self.assertIn("SET PRECISION ULTRA", client.commands)
        self.assertIn("SET MEMORY 2.5", client.commands)
        self.assertIn("SET FAVORITE 1 atan(", client.commands)
        self.assertIn("SET FORMAT 16 8", client.commands)
        self.assertIn("SET PROGRAMMER 255 HEX 1 7", client.commands)
        self.assertIn("SET GRAPH -2 2 -1 1", client.commands)
        self.assertIn("STAT ADD 1 3", client.commands)
        self.assertEqual(client.commands[-3:], [
            "BASIC CLEAR", 'BASIC LINE 10 PRINT "HELLO"',
            "BASIC LINE 20 END",
        ])

    def test_import_rejects_bad_rows(self):
        client = FakeClient()
        with self.assertRaises(ProtocolError):
            import_state(client, {
                "statistics": {"mode": 2, "values": [[1]]},
            })
        self.assertNotIn("STAT CLEAR", client.commands)

    def test_import_rejects_non_finite_values_before_writing(self):
        client = FakeClient()
        with self.assertRaises(ProtocolError):
            import_state(client, {
                "variables": {"A": 1, "B": float("nan")},
            })
        self.assertFalse(any(command.startswith("SET VAR")
                             for command in client.commands))

    def test_import_retries_dependent_functions(self):
        client = DependentFunctionClient()
        import_state(client, {
            "functions": {"F1": "f2(x)+1", "F2": "x*2"},
        })
        self.assertEqual(client.commands.count("SET FUNC F1 f2(x)+1"), 2)
        self.assertTrue(client.f2_defined)

    def test_partial_import_does_not_clear_expression(self):
        client = FakeClient()
        import_state(client, {"variables": {"A": 2}})
        self.assertNotIn("SET EXPR ", client.commands)

    def test_basic_program_is_sorted_and_validated_before_clear(self):
        client = FakeClient()
        program = synchronize_basic_program(client, "20 END\n10 PRINT 1\n")
        self.assertEqual(program, ["10 PRINT 1", "20 END"])
        self.assertEqual(client.commands[-3:], [
            "BASIC CLEAR", "BASIC LINE 10 PRINT 1", "BASIC LINE 20 END",
        ])

        invalid_client = FakeClient()
        with self.assertRaises(ProtocolError):
            synchronize_basic_program(invalid_client, "10 PRINT 1\n10 END")
        self.assertNotIn("BASIC CLEAR", invalid_client.commands)

        with self.assertRaises(ProtocolError):
            normalize_basic_program("PRINT 1")

    def test_circuit_validation_and_synchronization(self):
        circuit = normalize_circuit({
            "viewport_x": 10,
            "viewport_y": 20,
            "zoom": 200,
            "next_input": 1,
            "next_output": 1,
            "next_gate": 1,
            "nodes": [
                {"id": 4, "type": "INPUT", "x": 20, "y": 80,
                 "input": 1, "label": "A"},
                {"id": 7, "type": "NOT", "x": 160, "y": 80,
                 "input": 0, "label": "G1"},
                {"id": 9, "type": "OUTPUT", "x": 300, "y": 80,
                 "input": 0, "label": "Y"},
            ],
            "wires": [
                {"id": 2, "source": 4, "destination": 7, "input": 0},
                {"id": 5, "source": 7, "destination": 9, "input": 0},
            ],
        })
        self.assertFalse(circuit["nodes"][2]["output"])
        client = FakeClient()
        synchronize_circuit(client, circuit)
        self.assertIn("MODULE CIRCUIT CLEAR", client.commands)
        self.assertIn("MODULE CIRCUIT CONNECT 0 1 0", client.commands)
        self.assertIn("MODULE CIRCUIT CONNECT 1 2 0", client.commands)
        self.assertEqual(client.commands[-1], "MODULE CIRCUIT VIEW 10 20 200")

        invalid = dict(circuit)
        invalid["wires"] = list(circuit["wires"]) + [
            {"id": 6, "source": 9, "destination": 4, "input": 0}
        ]
        untouched = FakeClient()
        with self.assertRaises(ProtocolError):
            synchronize_circuit(untouched, invalid)
        self.assertNotIn("MODULE CIRCUIT CLEAR", untouched.commands)

        cyclic = {
            "next_gate": 2,
            "nodes": [
                {"id": 0, "type": "NOT", "x": 20, "y": 40,
                 "input": 0, "label": "G1"},
                {"id": 1, "type": "NOT", "x": 180, "y": 40,
                 "input": 0, "label": "G2"},
            ],
            "wires": [
                {"id": 0, "source": 0, "destination": 1, "input": 0},
                {"id": 1, "source": 1, "destination": 0, "input": 0},
            ],
        }
        with self.assertRaisesRegex(ProtocolError, "Zyklus"):
            normalize_circuit(cyclic)

        implication = normalize_circuit({
            "next_input": 2,
            "next_output": 1,
            "next_gate": 1,
            "nodes": [
                {"id": 0, "type": "INPUT", "x": 20, "y": 40,
                 "input": 1, "label": "A"},
                {"id": 1, "type": "INPUT", "x": 20, "y": 140,
                 "input": 0, "label": "B"},
                {"id": 2, "type": "IMPLIES", "x": 180, "y": 90,
                 "input": 0, "label": "G1"},
                {"id": 3, "type": "OUTPUT", "x": 340, "y": 90,
                 "input": 0, "label": "Y"},
            ],
            "wires": [
                {"id": 0, "source": 0, "destination": 2, "input": 0},
                {"id": 1, "source": 1, "destination": 2, "input": 1},
                {"id": 2, "source": 2, "destination": 3, "input": 0},
            ],
        })
        self.assertFalse(implication["nodes"][3]["output"])


if __name__ == "__main__":
    unittest.main()
