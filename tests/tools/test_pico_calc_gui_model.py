import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from pico_calc_cli import ProtocolError
from pico_calc_gui_model import (
    add_circuit_node,
    analyze_graph,
    analyze_statistics,
    circuit_from_logic,
    circuit_to_logic,
    connect_circuit_nodes,
    continue_basic_program,
    convert_unit,
    create_circuit,
    disconnect_circuit_input,
    evaluate_complex,
    evaluate_expression,
    evaluate_logic,
    format_logic_expression,
    format_number,
    inspect_ieee,
    inspect_number_format,
    number_theory_operation,
    normalize_logic_expression,
    parse_properties,
    parse_integer,
    programmer_operation,
    read_constants,
    read_device_snapshot,
    read_logic_form,
    read_truth_table,
    read_unit_category,
    remove_circuit_node,
    run_basic_program,
    set_circuit_input,
    set_circuit_node_type,
    set_circuit_view,
    stop_basic_program,
    synchronize_statistics,
    synchronize_symbols,
)


class FakeClient:
    def __init__(self):
        self.commands = []
        self.circuit_add_index = 0

    def command(self, command):
        self.commands.append(command)
        responses = {
            "INFO": "OK INFO\tprotocol=6\tfirmware=2.3.0\tmodel=scientific-calculator",
            "DIAG": (
                "OK DIAG\tpage=0\tangle=DEG\tprecision=HIGH"
                "\thistory=1\tstats=1\tmode=1"
                "\tbasic=1\tbasic_state=STOPPED"
            ),
            "GET RESULT": "OK RESULT\t42",
            "GET EXPR": "OK EXPR\t6*7",
            "GET HISTORY": "OK HISTORY\t1",
            "GET HISTORY 0": "OK HISTORY\t0\t42\t6*7\t42",
            "GET STATS": "OK STATS\t1\t1",
            "GET STATS 0": "OK STATS\t0\t3",
            "GET BASIC": "OK BASIC\t1",
            "GET BASIC 0": "OK BASIC\t0\t10\tEND",
            "EVAL sqrt(4)": "OK RESULT\t2",
            "EVAL 1.000000000000000000000000000000000000000000001*2": (
                "OK RESULT\t"
                "2.000000000000000000000000000000000000000000002"
            ),
            "GET BASIC STATUS": (
                "OK BASIC_STATUS\tstate=FINISHED\tstatus=OK\tsteps=2\toutput=1"
            ),
            "GET BASIC OUTPUT": "OK BASIC_OUTPUT\t1",
            "GET BASIC OUTPUT 0": "OK BASIC_OUTPUT\t0\tDONE",
            "GET ANGLE": "OK ANGLE\tDEG",
            "GET PRECISION": "OK PRECISION\tHIGH\t80",
            "GET MEMORY": "OK MEMORY\t0",
            "GET PROGRAMMER": (
                "OK PROGRAMMER_STATE\tvalue=0\tbase=DEC\tsigned=0\tbit=0"
            ),
            "GET FORMAT": "OK FORMAT_STATE\tbits=64\tfraction=16",
            "GET GRAPH": (
                "OK GRAPH\txmin=-10\txmax=10\tymin=-10\tymax=10"
                "\ttable=0\tstep=1"
            ),
            "MODULE PROGRAMMER NOT 0 8": (
                "OK PROGRAMMER\tvalue=255\tsigned=-1\thex=FF\tbin=11111111"
                "\tcarry=0\toverflow=0"
            ),
            "MODULE FORMAT 255 8 4": (
                "OK FORMAT\tunsigned=255\tsigned=-1\tfixed=-0.0625"
                "\tfloat32=BD800000\tfloat64=BFB0000000000000"
            ),
            "MODULE IEEE 32 1065353216": (
                "OK IEEE\twidth=32\tsign=0\trawexp=127\texponent=0"
                "\tmantissa=0\tclass=NORMAL\tvalue=1"
            ),
            "MODULE GRAPH ROOT F1 -2 2": (
                "OK GRAPH_ANALYSIS\taction=ROOT\tx=0\tvalue=0\titerations=1"
            ),
            "MODULE LOGIC EVAL 3 A&B": (
                "OK LOGIC_VALUE\tassignment=3\tvalue=1\tmask=3"
            ),
            "MODULE LOGIC INFO A&B": "OK LOGIC_INFO\tmask=3\tvars=2\trows=4",
            "MODULE LOGIC ROW 0 A&B": (
                "OK LOGIC_VALUE\tassignment=0\tvalue=0\tmask=3"
            ),
            "MODULE LOGIC ROW 1 A&B": (
                "OK LOGIC_VALUE\tassignment=1\tvalue=0\tmask=3"
            ),
            "MODULE LOGIC ROW 2 A&B": (
                "OK LOGIC_VALUE\tassignment=2\tvalue=0\tmask=3"
            ),
            "MODULE LOGIC ROW 3 A&B": (
                "OK LOGIC_VALUE\tassignment=3\tvalue=1\tmask=3"
            ),
            "MODULE LOGIC FORM DNF SIMPLE 0 A&B": (
                "OK LOGIC_FORM\ttotal=5\toffset=0\tdata=(A&B)"
            ),
            "MODULE CIRCUIT FROM A IMPLIES B": (
                "OK CIRCUIT_FROM\tnodes=2\twires=1"
            ),
            "MODULE CIRCUIT EXPR 1": (
                "OK CIRCUIT_EXPR\tassignment=1"
                "\texpression=(A IMPLIES B)"
            ),
            "MODULE UNIT CATEGORY 0": (
                "OK UNIT_CATEGORY\tindex=0\tname=LENGTH\tcount=2"
            ),
            "MODULE UNIT ITEM 0 0": (
                "OK UNIT\tindex=0\tname=millimetre\tsymbol=mm"
            ),
            "MODULE UNIT ITEM 0 1": (
                "OK UNIT\tindex=1\tname=metre\tsymbol=m"
            ),
            "MODULE UNIT CONVERT 0 0 1 1000": (
                "OK UNIT_RESULT\tvalue=1\tfrom=mm\tto=m"
            ),
            "MODULE CONSTANT COUNT": "OK CONSTANTS\t1",
            "MODULE CONSTANT 0": (
                "OK CONSTANT\tindex=0\tname=speed of light\tsymbol=c"
                "\tvalue=299792458\tunit=m/s\tsource=SI"
            ),
            "MODULE COMPLEX DEG 1+2i": (
                "OK COMPLEX\treal=1\timag=2\tcart=1+2i\tpolar=2.236 < 63 deg"
            ),
            "MODULE NUMBER GCD 84 30": (
                "OK NUMBER\taction=GCD\ta=84\tb=30\tvalue=6"
            ),
            "MODULE NUMBER PRIME 97": (
                "OK NUMBER\taction=PRIME\tinput=97\tvalue=1"
            ),
            "MODULE NUMBER FACTOR 360": (
                "OK NUMBER\taction=FACTOR\tinput=360\tcount=6"
                "\tcomplete=1\tvalue=2*2*2*3*3*5"
            ),
            "MODULE NUMBER POW 7 128 13": (
                "OK NUMBER\taction=POW\ta=7\tb=128\tmodulus=13\tvalue=3"
            ),
            "STAT SUMMARY X": (
                "OK STATS_SUMMARY\taxis=X\tcount=2\tmean=1.5\tmedian=1.5"
                "\tmin=1\tmax=2\tpopstd=0.5\tsamplestd=0.707"
            ),
            "STAT REGRESSION": (
                "OK STATS_REGRESSION\tslope=2\tintercept=1\tcorrelation=1"
            ),
            "STAT HISTOGRAM": (
                "OK STATS_HISTOGRAM\tmin=1\tmax=2\tbins=1,0,0,0,0,0,0,1"
            ),
        }
        if command.startswith("GET VAR "):
            return f"OK VAR\t{command[-1]}\t0"
        if command.startswith("GET FUNC "):
            return f"OK FUNC\t{command[-2:]}\tx+1"
        if command.startswith("GET FAVORITE "):
            return f"OK FAVORITE\t{command[-1]}\tsin("
        if command == "MODULE CIRCUIT INFO":
            return (
                "OK CIRCUIT_INFO\tnodes=2\twires=1\tnode_capacity=24"
                "\twire_capacity=48\tworld_width=1600\tworld_height=1200"
                "\tviewport_x=0\tviewport_y=0\tzoom=150\tcycle=0"
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
        return responses.get(command, "OK")


class GuiModelTests(unittest.TestCase):
    def test_parse_properties(self):
        parsed = parse_properties(
            "OK INFO\tprotocol=6\tfirmware=2.3.0", "INFO"
        )
        self.assertEqual(parsed, {"protocol": "6", "firmware": "2.3.0"})
        with self.assertRaises(ProtocolError):
            parse_properties("OK INFO\tbroken", "INFO")

    def test_read_device_snapshot(self):
        snapshot = read_device_snapshot(FakeClient())
        self.assertEqual(snapshot.info["firmware"], "2.3.0")
        self.assertEqual(snapshot.diagnostics["angle"], "DEG")
        self.assertEqual(snapshot.state["precision"], "HIGH")
        self.assertEqual(snapshot.state["history"][0]["expression"], "6*7")
        self.assertEqual(snapshot.state["program"], ["10 END"])
        self.assertTrue(snapshot.state["circuit"]["nodes"][1]["output"])

        old_client = FakeClient()
        old_client.command = lambda command: (
            "OK INFO\tprotocol=3\tfirmware=1.5.0"
            if command == "INFO" else "OK"
        )
        with self.assertRaisesRegex(ProtocolError, "Protokoll 6"):
            read_device_snapshot(old_client)

    def test_evaluate_expression(self):
        self.assertEqual(evaluate_expression(FakeClient(), "sqrt(4)"), "2")
        self.assertEqual(
            evaluate_expression(
                FakeClient(),
                "1.000000000000000000000000000000000000000000001*2",
            ),
            "2.000000000000000000000000000000000000000000002",
        )
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

    def test_basic_run_input_and_stop(self):
        client = FakeClient()
        result = run_basic_program(client, "10 PRINT 1\n20 END")
        self.assertEqual(result.status["state"], "FINISHED")
        self.assertEqual(result.output, ["DONE"])
        self.assertIn("BASIC RUN", client.commands)
        self.assertEqual(continue_basic_program(client, "21").output, ["DONE"])
        self.assertEqual(stop_basic_program(client).status["state"], "FINISHED")

        cancelled = FakeClient()
        run_basic_program(cancelled, "10 GOTO 10", lambda: True)
        self.assertIn("BASIC STOP", cancelled.commands)

        empty = FakeClient()
        with self.assertRaisesRegex(ProtocolError, "nicht leer"):
            run_basic_program(empty, "")
        self.assertNotIn("BASIC CLEAR", empty.commands)

    def test_programmer_and_number_formats(self):
        client = FakeClient()
        self.assertEqual(parse_integer("FF", "HEX"), 255)
        result = programmer_operation(client, "NOT", 0, 8)
        self.assertEqual(result["signed"], "-1")
        formatted = inspect_number_format(client, 255, 8, 4)
        self.assertEqual(formatted["fixed"], "-0.0625")
        ieee = inspect_ieee(client, 32, 0x3F800000)
        self.assertEqual(ieee["value"], "1")
        self.assertEqual(number_theory_operation(
            client, "GCD", 84, 30)["value"], "6")
        self.assertEqual(number_theory_operation(
            client, "PRIME", 97)["value"], "1")
        self.assertEqual(number_theory_operation(
            client, "FACTOR", 360)["count"], "6")
        self.assertEqual(number_theory_operation(
            client, "POW", 7, 128, 13)["value"], "3")
        with self.assertRaises(ProtocolError):
            number_theory_operation(client, "POW", 7, 128, 0)

    def test_circuit_editor_model(self):
        circuit = create_circuit()
        circuit, input_id = add_circuit_node(circuit, "INPUT", 20, 80)
        circuit, gate_id = add_circuit_node(circuit, "AND", 180, 80)
        circuit, output_id = add_circuit_node(circuit, "OUTPUT", 340, 80)
        circuit = connect_circuit_nodes(circuit, input_id, gate_id, 0)
        circuit = connect_circuit_nodes(circuit, input_id, gate_id, 1)
        circuit = connect_circuit_nodes(circuit, gate_id, output_id, 0)
        circuit = set_circuit_input(circuit, input_id, True)
        self.assertTrue(circuit["nodes"][2]["output"])
        circuit = set_circuit_node_type(circuit, gate_id, "NOT")
        self.assertFalse(circuit["nodes"][2]["output"])
        self.assertEqual(len(circuit["wires"]), 2)
        circuit = disconnect_circuit_input(circuit, output_id, 0)
        self.assertFalse(circuit["nodes"][2]["output"])
        circuit = set_circuit_view(circuit, 100, 120, 200)
        self.assertEqual(circuit["zoom"], 200)
        circuit = remove_circuit_node(circuit, gate_id)
        self.assertEqual(len(circuit["nodes"]), 2)

        implication = create_circuit()
        implication, left = add_circuit_node(implication, "INPUT", 20, 50)
        implication, right = add_circuit_node(implication, "INPUT", 20, 150)
        implication, gate = add_circuit_node(
            implication, "IMPLIES", 180, 100)
        implication = connect_circuit_nodes(implication, left, gate, 0)
        implication = connect_circuit_nodes(implication, right, gate, 1)
        implication = set_circuit_input(implication, left, True)
        self.assertFalse(implication["nodes"][gate]["output"])

    def test_logic_symbols_and_circuit_bridge(self):
        self.assertEqual(
            normalize_logic_expression("¬A∧B∨C⊕D↑E↓F→A↔B"),
            "!A&B|C^D NAND E NOR F IMPLIES A XNOR B",
        )
        self.assertEqual(
            format_logic_expression(
                "!A AND B OR C XOR D NAND E NOR F IMPLIES A IFF B"),
            "¬A ∧ B ∨ C ⊕ D ↑ E ↓ F → A ↔ B",
        )
        client = FakeClient()
        circuit = circuit_from_logic(client, "A→B")
        self.assertEqual(len(circuit["nodes"]), 2)
        converted = circuit_to_logic(client, circuit, 1)
        self.assertEqual(converted["expression"], "(A → B)")
        self.assertEqual(converted["assignment"], 1)

    def test_graph_logic_units_complex_and_statistics(self):
        client = FakeClient()
        graph = analyze_graph(client, "ROOT", "F1", [-2, 2])
        self.assertEqual(graph["x"], "0")
        self.assertEqual(evaluate_logic(client, "A&B", 3)["value"], "1")
        table = read_truth_table(client, "A&B")
        self.assertEqual(table["variables"], ["A", "B"])
        self.assertEqual(table["rows"][-1]["value"], 1)
        self.assertEqual(read_logic_form(client, "A&B", "DNF"), "(A∧B)")
        units = read_unit_category(client, 0)
        self.assertEqual(len(units["units"]), 2)
        self.assertEqual(convert_unit(client, 0, 0, 1, 1000)["value"], "1")
        self.assertEqual(read_constants(client)[0]["symbol"], "c")
        self.assertEqual(evaluate_complex(client, "1+2i", "DEG")["imag"], "2")
        self.assertEqual(analyze_statistics(client, "SUMMARY")["mean"], "1.5")
        self.assertEqual(analyze_statistics(client, "REGRESSION")["slope"], "2")
        self.assertIn("bins", analyze_statistics(client, "HISTOGRAM"))


if __name__ == "__main__":
    unittest.main()
