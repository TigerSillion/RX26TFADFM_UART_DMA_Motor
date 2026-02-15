import argparse
import csv
import os
import time

import serial


TX_PAYLOAD = (b"0123456789" * 13)[:128]


def open_serial(port: str, baud: int, timeout_s: float):
    s = serial.Serial(port=port, baudrate=baud, timeout=timeout_s, write_timeout=timeout_s)
    try:
        s.dtr = False
        s.rts = False
    except Exception:
        pass
    return s


def read_token(s: serial.Serial, token: bytes, timeout_s: float):
    end = time.time() + timeout_s
    last = b""
    while time.time() < end:
        line = s.readline()
        if not line:
            continue
        last = line
        if token in line:
            return True, line
    return False, last


def ping(port: str, baud: int, timeout_s: float):
    try:
        with open_serial(port, baud, timeout_s) as s:
            s.reset_input_buffer()
            s.write(b"\n")
            s.flush()
            _ = s.readline()
            s.write(b"@PING\n")
            s.flush()
            return read_token(s, b"@PONG", 1.5)
    except Exception as e:
        return False, repr(e).encode("ascii", "ignore")


def find_current_baud(port: str, timeout_s: float):
    probe_list = [
        115200, 230400, 460800, 921600, 1000000, 1500000, 1800000, 1900000,
        2000000, 2100000, 2200000, 2300000, 2400000, 2500000, 3000000, 3500000, 4000000
    ]
    for b in probe_list:
        ok, _ = ping(port, b, timeout_s)
        if ok:
            return b
    return None


def set_baud(port: str, cur: int, target: int, timeout_s: float):
    try:
        with open_serial(port, cur, timeout_s) as s:
            s.reset_input_buffer()
            s.write(f"@SETBAUD:{target}\n".encode("ascii"))
            s.flush()
            ok_ack, ack = read_token(s, f"@ACK:SETBAUD:{target}".encode("ascii"), 2.0)
            if not ok_ack:
                return False, f"ack_fail:{ack!r}"
        time.sleep(0.12)
        ok_ping, pong = ping(port, target, timeout_s)
        if not ok_ping:
            return False, f"ping_new_fail:{pong!r}"
        return True, "ok"
    except Exception as e:
        return False, f"exception:{e}"


def read_exact(s: serial.Serial, size: int, timeout_s: float):
    end = time.time() + timeout_s
    data = b""
    while len(data) < size and time.time() < end:
        chunk = s.read(size - len(data))
        if chunk:
            data += chunk
    return data


def stress_once(port: str, baud: int, packets: int, timeout_s: float):
    ok_pkts = 0
    fail_pkts = 0
    timeout_pkts = 0
    mismatch_pkts = 0
    first_fail = "-"
    try:
        with open_serial(port, baud, timeout_s) as s:
            s.reset_input_buffer()
            s.reset_output_buffer()
            for i in range(packets):
                s.write(TX_PAYLOAD)
                s.flush()
                rb = read_exact(s, len(TX_PAYLOAD), timeout_s)
                if len(rb) < len(TX_PAYLOAD):
                    fail_pkts += 1
                    timeout_pkts += 1
                    if first_fail == "-":
                        first_fail = f"timeout@{i}:len={len(rb)}"
                    continue
                if rb != TX_PAYLOAD:
                    fail_pkts += 1
                    mismatch_pkts += 1
                    if first_fail == "-":
                        first_fail = f"mismatch@{i}"
                    continue
                ok_pkts += 1
    except Exception as e:
        return {
            "ok_pkts": 0,
            "fail_pkts": packets,
            "timeout_pkts": packets,
            "mismatch_pkts": 0,
            "pkt_loss_pct": 100.0,
            "first_fail": f"io:{e}",
        }
    return {
        "ok_pkts": ok_pkts,
        "fail_pkts": fail_pkts,
        "timeout_pkts": timeout_pkts,
        "mismatch_pkts": mismatch_pkts,
        "pkt_loss_pct": (fail_pkts * 100.0 / packets) if packets else 0.0,
        "first_fail": first_fail,
    }


def main():
    ap = argparse.ArgumentParser(description="Find stable baud points with deterministic host-driven switching")
    ap.add_argument("--port", default="COM3")
    ap.add_argument("--start", type=int, default=2000000)
    ap.add_argument("--end", type=int, default=4000000)
    ap.add_argument("--step", type=int, default=100000)
    ap.add_argument("--base", type=int, default=2000000, help="recovery base baud between targets")
    ap.add_argument("--packets", type=int, default=400)
    ap.add_argument("--rounds", type=int, default=3)
    ap.add_argument("--timeout", type=float, default=0.10)
    args = ap.parse_args()

    cur = find_current_baud(args.port, args.timeout)
    print(f"current_baud={cur}")
    if cur is None:
        raise SystemExit("no responsive baud")

    if cur != args.base:
        ok, note = set_baud(args.port, cur, args.base, args.timeout)
        print(f"recover_to_base={ok},{note}")
        if not ok:
            raise SystemExit("cannot recover to base")
        cur = args.base

    rows = []
    print("baud,round,switch_ok,pkt_loss_pct,timeout_pkts,mismatch_pkts,first_fail")
    for target in range(args.start, args.end + args.step, args.step):
        for r in range(1, args.rounds + 1):
            if cur != args.base:
                ok_rb, _ = set_baud(args.port, cur, args.base, args.timeout)
                if not ok_rb:
                    cur_found = find_current_baud(args.port, args.timeout)
                    if cur_found is None:
                        rows.append([target, r, 0, 100.0, args.packets, 0, "recover_base_fail"])
                        print(f"{target},{r},0,100.000000,{args.packets},0,recover_base_fail")
                        continue
                    if cur_found != args.base:
                        ok_rb2, note_rb2 = set_baud(args.port, cur_found, args.base, args.timeout)
                        if not ok_rb2:
                            rows.append([target, r, 0, 100.0, args.packets, 0, f"recover_base_fail2:{note_rb2}"])
                            print(f"{target},{r},0,100.000000,{args.packets},0,recover_base_fail2:{note_rb2}")
                            continue
                    cur = args.base

            if target != args.base:
                ok_sw, note_sw = set_baud(args.port, args.base, target, args.timeout)
                if not ok_sw:
                    rows.append([target, r, 0, 100.0, args.packets, 0, f"switch_fail:{note_sw}"])
                    print(f"{target},{r},0,100.000000,{args.packets},0,switch_fail:{note_sw}")
                    continue
                cur = target
            else:
                cur = args.base

            res = stress_once(args.port, target, args.packets, args.timeout)
            rows.append([target, r, 1, res["pkt_loss_pct"], res["timeout_pkts"], res["mismatch_pkts"], res["first_fail"]])
            print(
                f"{target},{r},1,{res['pkt_loss_pct']:.6f},{res['timeout_pkts']},{res['mismatch_pkts']},{res['first_fail']}"
            )

    out_dir = os.path.join("tools", "UartRateTester", "results")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"stable_point_scan_{time.strftime('%Y%m%d_%H%M%S')}.csv")
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["baud", "round", "switch_ok", "pkt_loss_pct", "timeout_pkts", "mismatch_pkts", "first_fail"])
        w.writerows(rows)
    print("saved", out_path)


if __name__ == "__main__":
    main()

