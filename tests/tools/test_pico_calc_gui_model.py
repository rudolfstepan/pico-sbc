import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from pico_calc_cli import ProtocolError
from pico_calc_gui_model import (
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
    parse_properties,
    parse_integer,
    programmer_operation,
    read_constants,
    read_device_snapshot,
    read_logic_form,
    read_truth_table,
    read_unit_category,
    run_basic_program,
    stop_basic_program,
    synchronize_statistics,
    synchronize_symbols,
)


class FakeClient:
    def __init__(self):
        self.commands = []

    def command(self, command):
        self.commands.append(command)
        responses = {
            "INFO": "OK INFO\tprotocol=4\tfirmware=1.6.0\tmodel=scientific-calculator",
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
        return responses.get(command, "OK")


class GuiModelTests(unittest.TestCase):
    def test_parse_properties(self):
        parsed = parse_properties(
            "OK INFO\tprotocol=4\tfirmware=1.6.0", "INFO"
        )
        self.assertEqual(parsed, {"protocol": "4", "firmware": "1.6.0"})
        with self.assertRaises(ProtocolError):
            parse_properties("OK INFO\tbroken", "INFO")

    def test_read_device_snapshot(self):
        snapshot = read_device_snapshot(FakeClient())
        self.assertEqual(snapshot.info["firmware"], "1.6.0")
        self.assertEqual(snapshot.diagnostics["angle"], "DEG")
        self.assertEqual(snapshot.state["precision"], "HIGH")
        self.assertEqual(snapshot.state["history"][0]["expression"], "6*7")
        self.assertEqual(snapshot.state["program"], ["10 END"])

        old_client = FakeClient()
        old_client.command = lambda command: (
            "OK INFO\tprotocol=3\tfirmware=1.5.0"
            if command == "INFO" else "OK"
        )
        with self.assertRaisesRegex(ProtocolError, "Protokoll 4"):
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

    def test_graph_logic_units_complex_and_statistics(self):
        client = FakeClient()
        graph = analyze_graph(client, "ROOT", "F1", [-2, 2])
        self.assertEqual(graph["x"], "0")
        self.assertEqual(evaluate_logic(client, "A&B", 3)["value"], "1")
        table = read_truth_table(client, "A&B")
        self.assertEqual(table["variables"], ["A", "B"])
        self.assertEqual(table["rows"][-1]["value"], 1)
        self.assertEqual(read_logic_form(client, "A&B", "DNF"), "(A&B)")
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
