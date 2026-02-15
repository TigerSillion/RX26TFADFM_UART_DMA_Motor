import threading
import time
import queue
import csv
import os
import tkinter as tk
from tkinter import ttk, messagebox, filedialog

import serial
import serial.tools.list_ports

BAUD_CANDIDATES = [
    115200,
    230400,
    460800,
    921600,
    1000000,
    1500000,
    2000000,
    3000000,
    4000000,
]

PATTERN = b"0123456789"


class UartRateTesterApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("UART Rate Limit Tester")
        self.root.geometry("980x660")

        self.log_q: queue.Queue[str] = queue.Queue()
        self.worker: threading.Thread | None = None
        self.stop_flag = threading.Event()

        self.port_var = tk.StringVar()
        self.payload_var = tk.StringVar(value="512")
        self.packet_count_var = tk.StringVar(value="100")
        self.timeout_var = tk.StringVar(value="0.50")
        self.phase_var = tk.StringVar(value="Idle")
        self.progress_var = tk.DoubleVar(value=0.0)
        self.detail_var = tk.StringVar(value="-")
        self._total_steps = 1
        self._done_steps = 0
        self.results: list[dict] = []

        self.baud_vars: dict[int, tk.BooleanVar] = {}

        self._build_ui()
        self.refresh_ports()
        self.root.after(120, self._drain_logs)

    def _build_ui(self):
        top = ttk.Frame(self.root, padding=10)
        top.pack(fill=tk.X)

        ttk.Label(top, text="Port").grid(row=0, column=0, sticky=tk.W)
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=20, state="readonly")
        self.port_combo.grid(row=0, column=1, padx=6, sticky=tk.W)

        ttk.Button(top, text="Refresh", command=self.refresh_ports).grid(row=0, column=2, padx=4)

        ttk.Label(top, text="Payload(bytes)").grid(row=0, column=3, padx=(16, 0), sticky=tk.W)
        ttk.Entry(top, textvariable=self.payload_var, width=8).grid(row=0, column=4, padx=4)

        ttk.Label(top, text="Packets/baud").grid(row=0, column=5, padx=(12, 0), sticky=tk.W)
        ttk.Entry(top, textvariable=self.packet_count_var, width=8).grid(row=0, column=6, padx=4)

        ttk.Label(top, text="ReadTimeout(s)").grid(row=0, column=7, padx=(12, 0), sticky=tk.W)
        ttk.Entry(top, textvariable=self.timeout_var, width=8).grid(row=0, column=8, padx=4)

        btns = ttk.Frame(self.root, padding=(10, 0, 10, 10))
        btns.pack(fill=tk.X)
        self.start_btn = ttk.Button(btns, text="Start Scan", command=self.start_scan)
        self.start_btn.pack(side=tk.LEFT)
        self.stop_btn = ttk.Button(btns, text="Stop", command=self.stop_scan, state=tk.DISABLED)
        self.stop_btn.pack(side=tk.LEFT, padx=8)
        ttk.Button(btns, text="Clear Results", command=self.clear_results).pack(side=tk.LEFT)
        ttk.Button(btns, text="Export CSV", command=self.export_csv).pack(side=tk.LEFT, padx=(8, 0))

        status = ttk.LabelFrame(self.root, text="Progress", padding=10)
        status.pack(fill=tk.X, padx=10, pady=(0, 8))
        ttk.Label(status, text="Phase").grid(row=0, column=0, sticky=tk.W)
        ttk.Label(status, textvariable=self.phase_var).grid(row=0, column=1, sticky=tk.W, padx=(6, 18))
        ttk.Label(status, text="Detail").grid(row=0, column=2, sticky=tk.W)
        ttk.Label(status, textvariable=self.detail_var).grid(row=0, column=3, sticky=tk.W, padx=6)
        self.progress = ttk.Progressbar(status, mode="determinate", maximum=100.0, variable=self.progress_var)
        self.progress.grid(row=1, column=0, columnspan=4, sticky=tk.EW, pady=(8, 0))
        status.columnconfigure(3, weight=1)

        baud_frame = ttk.LabelFrame(self.root, text="Baud Candidates", padding=10)
        baud_frame.pack(fill=tk.X, padx=10)
        for i, b in enumerate(BAUD_CANDIDATES):
            var = tk.BooleanVar(value=True)
            self.baud_vars[b] = var
            ttk.Checkbutton(baud_frame, text=str(b), variable=var).grid(
                row=i // 6, column=i % 6, sticky=tk.W, padx=10, pady=2
            )

        result_frame = ttk.LabelFrame(self.root, text="Scan Results", padding=10)
        result_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(8, 0))

        cols = ("baud", "ok", "sent", "recv", "mismatch", "timeout", "seconds", "kbps", "note")
        self.tree = ttk.Treeview(result_frame, columns=cols, show="headings", height=12)
        for c, w in [
            ("baud", 90),
            ("ok", 60),
            ("sent", 90),
            ("recv", 90),
            ("mismatch", 90),
            ("timeout", 80),
            ("seconds", 80),
            ("kbps", 80),
            ("note", 300),
        ]:
            self.tree.heading(c, text=c)
            self.tree.column(c, width=w, anchor=tk.CENTER if c != "note" else tk.W)
        self.tree.pack(fill=tk.BOTH, expand=True)

        log_frame = ttk.LabelFrame(self.root, text="Log", padding=10)
        log_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(8, 10))
        self.log_text = tk.Text(log_frame, height=10)
        self.log_text.pack(fill=tk.BOTH, expand=True)

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo["values"] = ports
        if ports and self.port_var.get() not in ports:
            self.port_var.set(ports[0])
        self._log(f"ports: {ports}")

    def clear_results(self):
        self.results.clear()
        for i in self.tree.get_children():
            self.tree.delete(i)

    def export_csv(self):
        if not self.results:
            messagebox.showinfo("Info", "No results to export")
            return
        default_name = f"uart_rate_scan_{time.strftime('%Y%m%d_%H%M%S')}.csv"
        path = filedialog.asksaveasfilename(
            title="Export UART Scan CSV",
            defaultextension=".csv",
            initialfile=default_name,
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
        )
        if not path:
            return
        self._write_csv(path)
        self._log(f"csv exported: {path}")

    def _write_csv(self, path: str):
        folder = os.path.dirname(path)
        if folder:
            os.makedirs(folder, exist_ok=True)
        with open(path, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(["baud", "ok", "sent", "recv", "mismatch", "timeout", "seconds", "kbps", "note"])
            for r in self.results:
                w.writerow(
                    [
                        r["baud"],
                        r["ok"],
                        r["sent"],
                        r["recv"],
                        r["mismatch"],
                        r["timeout"],
                        f"{r['seconds']:.6f}",
                        f"{r['kbps']:.3f}",
                        r["note"],
                    ]
                )

    def start_scan(self):
        if self.worker and self.worker.is_alive():
            return

        port = self.port_var.get().strip()
        if not port:
            messagebox.showerror("Error", "Select COM port first")
            return

        try:
            payload = int(self.payload_var.get())
            packets = int(self.packet_count_var.get())
            timeout_s = float(self.timeout_var.get())
        except ValueError:
            messagebox.showerror("Error", "Payload/Packets/Timeout format invalid")
            return

        selected = [b for b, v in self.baud_vars.items() if v.get()]
        if not selected:
            messagebox.showerror("Error", "Select at least one baud")
            return

        if payload <= 0 or packets <= 0 or timeout_s <= 0:
            messagebox.showerror("Error", "Payload/Packets/Timeout must be positive")
            return

        self.stop_flag.clear()
        self.start_btn.config(state=tk.DISABLED)
        self.stop_btn.config(state=tk.NORMAL)
        self.phase_var.set("Preparing")
        self.detail_var.set("building test plan")
        self.progress_var.set(0.0)
        self._done_steps = 0
        self._total_steps = max(1, 1 + len(selected) * 2)

        args = (port, selected, payload, packets, timeout_s)
        self.worker = threading.Thread(target=self._scan_worker, args=args, daemon=True)
        self.worker.start()

    def stop_scan(self):
        self.stop_flag.set()
        self._log("stop requested")
        self.phase_var.set("Stopping")

    def _scan_worker(self, port: str, bauds: list[int], payload: int, packets: int, timeout_s: float):
        self._log(f"scan start: port={port} payload={payload} packets={packets} timeout={timeout_s}")
        current_baud = 115200

        self.root.after(0, self._set_phase, "Handshake", f"@PING on {current_baud}")
        boot = self._probe_handshake(port, current_baud, timeout_s)
        if boot["ok"] != "Y":
            self.root.after(0, self._insert_result, boot)
            self.root.after(0, self._scan_done)
            return
        self.root.after(0, self._insert_result, boot)
        self.root.after(0, self._step_done)

        for baud in bauds:
            if self.stop_flag.is_set():
                break
            if baud != current_baud:
                self.root.after(0, self._set_phase, "Switch Baud", f"{current_baud} -> {baud}")
                sw = self._switch_baud(port, current_baud, baud, timeout_s)
                self.root.after(0, self._insert_result, sw)
                self.root.after(0, self._step_done)
                if sw["ok"] != "Y":
                    continue
                current_baud = baud
            self.root.after(0, self._set_phase, "Stress Test", f"{baud} bps")
            result = self._run_single_baud(port, baud, payload, packets, timeout_s)
            self.root.after(0, self._insert_result, result)
            self.root.after(0, self._step_done)

        self.root.after(0, self._scan_done)

    def _open_serial(self, port: str, baud: int, timeout_s: float):
        ser = serial.Serial(port=port, baudrate=baud, timeout=timeout_s, write_timeout=timeout_s)
        # Avoid toggling reset-sensitive control lines on USB-UART adapters/boards.
        try:
            ser.dtr = False
            ser.rts = False
        except Exception:
            pass
        return ser

    def _readline_until_token(self, ser: serial.Serial, token: bytes, timeout_s: float):
        deadline = time.perf_counter() + timeout_s
        last = b""
        while time.perf_counter() < deadline:
            line = ser.readline()
            if not line:
                continue
            last = line
            if token in line:
                return line
        return last

    def _probe_handshake(self, port: str, baud: int, timeout_s: float):
        try:
            ser = self._open_serial(port=port, baud=baud, timeout_s=timeout_s)
        except Exception as e:
            return {
                "baud": baud,
                "ok": "N",
                "sent": 0,
                "recv": 0,
                "mismatch": 0,
                "timeout": 1,
                "seconds": 0.0,
                "kbps": 0.0,
                "note": f"open fail: {e}",
            }

        t0 = time.perf_counter()
        with ser:
            try:
                ser.reset_input_buffer()
                # Recover parser state: if MCU is stuck in "@...waiting EOL", force line termination.
                ser.write(b"\n")
                ser.flush()
                _ = ser.readline()
                ser.write(b"@PING\n")
                ser.flush()
                line = self._readline_until_token(ser, b"@PONG", max(1.5, timeout_s * 4.0))
            except Exception as e:
                return {
                    "baud": baud,
                    "ok": "N",
                    "sent": 6,
                    "recv": 0,
                    "mismatch": 0,
                    "timeout": 1,
                    "seconds": time.perf_counter() - t0,
                    "kbps": 0.0,
                    "note": f"ping fail: {e}",
                }

        ok = b"@PONG" in line
        dt = max(time.perf_counter() - t0, 1e-9)
        return {
            "baud": baud,
            "ok": "Y" if ok else "N",
            "sent": 6,
            "recv": len(line),
            "mismatch": 0 if ok else 1,
            "timeout": 0 if ok else 1,
            "seconds": dt,
            "kbps": (len(line) * 8.0) / dt / 1000.0,
            "note": "handshake @PING/@PONG" if ok else f"bad pong: {line!r}",
        }

    def _switch_baud(self, port: str, old_baud: int, new_baud: int, timeout_s: float):
        t0 = time.perf_counter()
        cmd = f"@SETBAUD:{new_baud}\n".encode("ascii")
        ack_ok = False
        ready_ok = False
        ping_ok = False
        note = "ok"
        ack_len = 0
        ready_len = 0
        try:
            ser = self._open_serial(port=port, baud=old_baud, timeout_s=timeout_s)
            with ser:
                ser.reset_input_buffer()
                ser.write(cmd)
                ser.flush()
                ack = self._readline_until_token(
                    ser, f"@ACK:SETBAUD:{new_baud}".encode("ascii"), max(1.5, timeout_s * 4.0)
                )
                ack_len = len(ack)
                ack_ok = (f"@ACK:SETBAUD:{new_baud}".encode("ascii") in ack)
        except Exception as e:
            note = f"switch cmd fail: {e}"
            ack_ok = False

        if ack_ok:
            time.sleep(0.08)
            try:
                ser2 = self._open_serial(port=port, baud=new_baud, timeout_s=timeout_s)
                with ser2:
                    ready = self._readline_until_token(
                        ser2, f"@READY:{new_baud}".encode("ascii"), max(2.0, timeout_s * 5.0)
                    )
                    ready_len = len(ready)
                    ready_ok = (f"@READY:{new_baud}".encode("ascii") in ready)
                    if not ready_ok:
                        # READY frame can be missed during port reopen window; use active ping confirm.
                        ser2.reset_input_buffer()
                        ser2.write(b"\n")
                        ser2.flush()
                        _ = ser2.readline()
                        ser2.write(b"@PING\n")
                        ser2.flush()
                        pong = self._readline_until_token(ser2, b"@PONG", max(1.5, timeout_s * 4.0))
                        ping_ok = (b"@PONG" in pong)
                        if ping_ok:
                            note = "ready miss but ping ok"
                        else:
                            note = f"ready/ping miss: {ready!r}"
                    else:
                        ping_ok = True
            except Exception as e:
                note = f"reopen fail: {e}"
                ready_ok = False
        else:
            note = "ack fail"

        dt = max(time.perf_counter() - t0, 1e-9)
        ok = ack_ok and (ready_ok or ping_ok)
        recv_size = ack_len + ready_len
        return {
            "baud": new_baud,
            "ok": "Y" if ok else "N",
            "sent": len(cmd),
            "recv": recv_size,
            "mismatch": 0 if ok else 1,
            "timeout": 0 if ok else 1,
            "seconds": dt,
            "kbps": (recv_size * 8.0) / dt / 1000.0,
            "note": f"switch {old_baud}->{new_baud}: {note}",
        }

    def _run_single_baud(self, port: str, baud: int, payload: int, packets: int, timeout_s: float):
        sent = 0
        recv = 0
        mismatch = 0
        timeout_count = 0
        note = "ok"
        ok = True
        t0 = time.perf_counter()

        try:
            ser = self._open_serial(port=port, baud=baud, timeout_s=timeout_s)
        except Exception as e:
            return {
                "baud": baud,
                "ok": "N",
                "sent": 0,
                "recv": 0,
                "mismatch": 0,
                "timeout": 0,
                "seconds": 0.0,
                "kbps": 0.0,
                "note": f"open fail: {e}",
            }

        with ser:
            try:
                ser.reset_input_buffer()
                ser.reset_output_buffer()
            except Exception:
                pass

            base = (PATTERN * ((payload // len(PATTERN)) + 1))[:payload]

            for i in range(packets):
                if self.stop_flag.is_set():
                    note = "stopped"
                    ok = False
                    break
                if i == 0 or ((i + 1) % 10 == 0) or (i + 1 == packets):
                    self.root.after(0, self._set_phase, "Stress Test", f"{baud} pkt {i + 1}/{packets}")

                # Keep test bytes away from command channel markers ('@', CR, LF).
                tx = bytes(base[j] for j in range(payload))

                try:
                    ser.write(tx)
                    ser.flush()
                except Exception as e:
                    ok = False
                    note = f"write fail: {e}"
                    break

                sent += len(tx)

                rx = b""
                deadline = time.perf_counter() + timeout_s
                while len(rx) < len(tx) and time.perf_counter() < deadline:
                    chunk = ser.read(len(tx) - len(rx))
                    if not chunk:
                        continue
                    rx += chunk

                recv += len(rx)

                if len(rx) < len(tx):
                    timeout_count += 1
                    ok = False
                    note = f"timeout at pkt {i}"
                    break

                if rx != tx:
                    mismatch += 1
                    ok = False
                    note = f"mismatch at pkt {i}"
                    break

        dt = max(time.perf_counter() - t0, 1e-9)
        kbps = (recv * 8.0) / dt / 1000.0

        self._log(
            f"baud={baud} ok={ok} sent={sent} recv={recv} mismatch={mismatch} timeout={timeout_count} "
            f"sec={dt:.3f} kbps={kbps:.1f} note={note}"
        )

        return {
            "baud": baud,
            "ok": "Y" if ok else "N",
            "sent": sent,
            "recv": recv,
            "mismatch": mismatch,
            "timeout": timeout_count,
            "seconds": dt,
            "kbps": kbps,
            "note": note,
        }

    def _insert_result(self, r: dict):
        self.results.append(r)
        self.tree.insert(
            "",
            tk.END,
            values=(
                r["baud"],
                r["ok"],
                r["sent"],
                r["recv"],
                r["mismatch"],
                r["timeout"],
                f"{r['seconds']:.3f}",
                f"{r['kbps']:.1f}",
                r["note"],
            ),
        )

    def _scan_done(self):
        self.start_btn.config(state=tk.NORMAL)
        self.stop_btn.config(state=tk.DISABLED)
        self.phase_var.set("Done")
        self.detail_var.set("scan completed")
        self.progress_var.set(100.0)
        if self.results:
            out_dir = os.path.join(os.getcwd(), "tools", "UartRateTester", "results")
            out_name = f"uart_rate_scan_{time.strftime('%Y%m%d_%H%M%S')}.csv"
            out_path = os.path.join(out_dir, out_name)
            try:
                self._write_csv(out_path)
                self._log(f"auto csv saved: {out_path}")
            except Exception as e:
                self._log(f"auto csv save fail: {e}")
        self._log("scan done")

    def _set_phase(self, phase: str, detail: str):
        self.phase_var.set(phase)
        self.detail_var.set(detail)

    def _step_done(self):
        self._done_steps += 1
        p = min(100.0, (self._done_steps * 100.0) / float(max(1, self._total_steps)))
        self.progress_var.set(p)

    def _log(self, text: str):
        self.log_q.put(text)

    def _drain_logs(self):
        while True:
            try:
                msg = self.log_q.get_nowait()
            except queue.Empty:
                break
            self.log_text.insert(tk.END, f"[{time.strftime('%H:%M:%S')}] {msg}\n")
            self.log_text.see(tk.END)
        self.root.after(120, self._drain_logs)


def main():
    root = tk.Tk()
    app = UartRateTesterApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()


