import argparse
import csv
import os
import time

import serial


SAFE_PAYLOAD = (b"0123456789" * 13)[:128]


def build_bauds_desc(start_baud: int, end_baud: int, step_baud: int):
    bauds = []
    b = start_baud
    while b >= end_baud:
        bauds.append(b)
        b -= step_baud
    if bauds[-1] != end_baud:
        bauds.append(end_baud)
    return bauds


def open_serial(port: str, baud: int, timeout_s: float):
    s = serial.Serial(port=port, baudrate=baud, timeout=timeout_s, write_timeout=timeout_s)
    try:
        s.dtr = False
        s.rts = False
    except Exception:
        pass
    return s


def read_exact(s: serial.Serial, size: int, timeout_s: float):
    end = time.time() + timeout_s
    data = b""
    while len(data) < size and time.time() < end:
        chunk = s.read(size - len(data))
        if chunk:
            data += chunk
    return data


def dynamic_timeout(payload_len: int, baud: int, base_timeout_s: float):
    # TX+RX bits ~= payload_len * 20 (8N1 both ways), then add margin.
    wire_time = (payload_len * 20.0) / float(baud)
    return max(base_timeout_s, wire_time * 8.0 + 0.01)


def probe_echo_once(port: str, baud: int, payload: bytes, timeout_s: float):
    try:
        with open_serial(port, baud, timeout_s) as s:
            s.reset_input_buffer()
            s.reset_output_buffer()
            s.write(payload)
            s.flush()
            rb = read_exact(s, len(payload), timeout_s)
            return rb == payload, rb
    except Exception:
        return False, b""


def run_baud_test(port: str, baud: int, payload: bytes, packets: int, base_timeout_s: float):
    sent_bytes = 0
    recv_bytes = 0
    ok_pkts = 0
    timeout_pkts = 0
    mismatch_pkts = 0
    first_fail = "-"
    timeout_s = dynamic_timeout(len(payload), baud, base_timeout_s)
    t0 = time.time()

    with open_serial(port, baud, timeout_s) as s:
        s.reset_input_buffer()
        s.reset_output_buffer()
        for i in range(packets):
            s.write(payload)
            s.flush()
            sent_bytes += len(payload)
            rb = read_exact(s, len(payload), timeout_s)
            recv_bytes += len(rb)

            if len(rb) < len(payload):
                timeout_pkts += 1
                if first_fail == "-":
                    first_fail = f"timeout@{i}:len={len(rb)}"
                continue
            if rb != payload:
                mismatch_pkts += 1
                if first_fail == "-":
                    first_fail = f"mismatch@{i}"
                continue
            ok_pkts += 1

    dt = max(time.time() - t0, 1e-9)
    fail_pkts = packets - ok_pkts
    pkt_loss_rate = (fail_pkts * 100.0) / packets if packets else 0.0
    byte_loss = sent_bytes - recv_bytes
    byte_loss_rate = (byte_loss * 100.0) / sent_bytes if sent_bytes else 0.0
    kbps = (recv_bytes * 8.0 / 1000.0) / dt

    return {
        "baud": baud,
        "packets": packets,
        "ok_pkts": ok_pkts,
        "fail_pkts": fail_pkts,
        "timeout_pkts": timeout_pkts,
        "mismatch_pkts": mismatch_pkts,
        "pkt_loss_rate_pct": pkt_loss_rate,
        "sent_bytes": sent_bytes,
        "recv_bytes": recv_bytes,
        "byte_loss": byte_loss,
        "byte_loss_rate_pct": byte_loss_rate,
        "duration_sec": dt,
        "throughput_kbps": kbps,
        "first_fail": first_fail,
    }


def main():
    p = argparse.ArgumentParser(description="Descending UART loss/stability scan (high->low)")
    p.add_argument("--port", default="COM3")
    p.add_argument("--start", type=int, default=2500000, help="start baud")
    p.add_argument("--end", type=int, default=100000, help="end baud")
    p.add_argument("--step", type=int, default=100000, help="step baud")
    p.add_argument("--packets", type=int, default=200, help="packets per baud")
    p.add_argument("--payload", type=int, default=128, help="payload length")
    p.add_argument("--timeout", type=float, default=0.02, help="base timeout seconds")
    args = p.parse_args()

    payload = (SAFE_PAYLOAD * ((args.payload // len(SAFE_PAYLOAD)) + 1))[: args.payload]
    bauds = build_bauds_desc(args.start, args.end, args.step)

    print("baud,ok_pkts,fail_pkts,timeout_pkts,mismatch_pkts,pkt_loss_pct,byte_loss_pct,kbps,first_fail")

    rows = []
    for b in bauds:
        try:
            # quick probe before full test
            ok_probe, _ = probe_echo_once(args.port, b, payload, dynamic_timeout(len(payload), b, args.timeout))
            if not ok_probe:
                # try one-time recovery toggle only if echo appears disabled
                try:
                    with open_serial(args.port, b, dynamic_timeout(len(payload), b, args.timeout)) as s:
                        s.reset_input_buffer()
                        s.write(b"e")
                        s.flush()
                        _ = s.read(64)
                except Exception:
                    pass

            r = run_baud_test(args.port, b, payload, args.packets, args.timeout)
            print(
                f"{b},{r['ok_pkts']},{r['fail_pkts']},{r['timeout_pkts']},{r['mismatch_pkts']},"
                f"{r['pkt_loss_rate_pct']:.4f},{r['byte_loss_rate_pct']:.4f},{r['throughput_kbps']:.1f},{r['first_fail']}"
            )
            rows.append(r)
        except Exception as e:
            print(f"{b},0,{args.packets},{args.packets},0,100.0000,100.0000,0.0,open_or_io_fail:{e}")
            rows.append(
                {
                    "baud": b,
                    "packets": args.packets,
                    "ok_pkts": 0,
                    "fail_pkts": args.packets,
                    "timeout_pkts": args.packets,
                    "mismatch_pkts": 0,
                    "pkt_loss_rate_pct": 100.0,
                    "sent_bytes": 0,
                    "recv_bytes": 0,
                    "byte_loss": 0,
                    "byte_loss_rate_pct": 100.0,
                    "duration_sec": 0.0,
                    "throughput_kbps": 0.0,
                    "first_fail": f"open_or_io_fail:{e}",
                }
            )

    out_dir = os.path.join("tools", "UartRateTester", "results")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"desc_100k_scan_{time.strftime('%Y%m%d_%H%M%S')}.csv")
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(
            [
                "baud",
                "packets",
                "ok_pkts",
                "fail_pkts",
                "timeout_pkts",
                "mismatch_pkts",
                "pkt_loss_rate_pct",
                "sent_bytes",
                "recv_bytes",
                "byte_loss",
                "byte_loss_rate_pct",
                "duration_sec",
                "throughput_kbps",
                "first_fail",
            ]
        )
        for r in rows:
            w.writerow(
                [
                    r["baud"],
                    r["packets"],
                    r["ok_pkts"],
                    r["fail_pkts"],
                    r["timeout_pkts"],
                    r["mismatch_pkts"],
                    f"{r['pkt_loss_rate_pct']:.6f}",
                    r["sent_bytes"],
                    r["recv_bytes"],
                    r["byte_loss"],
                    f"{r['byte_loss_rate_pct']:.6f}",
                    f"{r['duration_sec']:.6f}",
                    f"{r['throughput_kbps']:.3f}",
                    r["first_fail"],
                ]
            )
    print("saved", out_path)


if __name__ == "__main__":
    main()

