from pathlib import Path
import ctypes
import json
import queue
import re
import socket
import struct
import subprocess
import threading
import time
import tkinter as tk

from PIL import Image, ImageTk
from padctl import BUTTONS, atomic_json

KEYS = {"Up": 0, "Down": 1, "Left": 2, "Right": 3, "z": 4, "x": 5, "c": 6, "Return": 7,
        "BackSpace": 9, "s": 10, "d": 11, "a": 12}
FRAME_BYTES = 320 * 240 * 4
GUEST_RATE = re.compile(rb"\[R36S-LIVE\] presentations=(\d+) elapsed_us=(\d+)")
GUEST_TARGET = re.compile(rb"\[pacer\] target frame rate: ([\d.]+) fps")


class PreviewRates:
    def __init__(self, log):
        self.log = log
        self.offset = 0
        self.guest_fps = None
        self.target_fps = None
        self.views = 0
        self.since = time.monotonic()

    def measure(self, now):
        with self.log.open("rb") as stream:
            stream.seek(self.offset)
            new_data = stream.read()
        end = new_data.rfind(b"\n") + 1
        self.offset += end
        for line in new_data[:end].splitlines():
            if target := GUEST_TARGET.search(line):
                self.target_fps = float(target[1])
            if rate := GUEST_RATE.search(line):
                frames, microseconds = map(int, rate.groups())
                if microseconds:
                    self.guest_fps = frames * 1_000_000 / microseconds
        viewer_fps = self.views / (now - self.since)
        self.since, self.views = now, 0
        return viewer_fps

    def title(self, now):
        viewer_fps = self.measure(now)
        if viewer_fps == 0:
            return "Visor sin fotogramas nuevos"
        guest = "VM — FPS"
        if self.guest_fps is not None:
            guest = f"VM {self.guest_fps:.1f} FPS"
            if self.target_fps:
                guest += f" / {self.target_fps:.1f} ({self.guest_fps / self.target_fps:.0%})"
        return f"{guest} · Visor {viewer_fps:.1f} FPS"


def xinput_buttons():
    # XINPUT_STATE is a packet number followed by the gamepad's button mask.
    try:
        get_state = ctypes.WinDLL("xinput1_4.dll").XInputGetState
    except OSError:
        return lambda: 0
    raw = ctypes.create_string_buffer(16)
    mapping = {0x0001: 0, 0x0002: 1, 0x0004: 2, 0x0008: 3, 0x2000: 4, 0x1000: 5,
               0x4000: 6, 0x0010: 7, 0x0020: 9, 0x0100: 10, 0x0200: 11, 0x8000: 12}

    def read():
        if get_state(0, ctypes.byref(raw)):
            return 0
        buttons = struct.unpack_from("<H", raw, 4)[0]
        return sum(1 << bit for physical, bit in mapping.items() if buttons & physical)

    return read


def start_guest(cmd, log):
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    cmd[cmd.index("-serial") + 1] = f"file:{log}"
    cmd += ["-chardev", f"socket,id=preview,host=127.0.0.1,port={port},server=on,wait=on,nodelay=on",
            "-device", "virtio-serial-pci" if "x86_64" in Path(cmd[0]).stem else "virtio-serial-device",
            "-device", "virtserialport,chardev=preview,name=org.dynabro.preview"]
    vm = subprocess.Popen(cmd, creationflags=subprocess.CREATE_NO_WINDOW)
    return vm, port


def connect_guest(vm, port):
    for _ in range(160):
        if vm.poll() is not None:
            break
        try:
            return socket.create_connection(("127.0.0.1", port), timeout=1)
        except (ConnectionRefusedError, TimeoutError):
            time.sleep(0.1)
    raise RuntimeError("The preview connection did not open")


def benchmark_preview(cmd, directory, translated=False, frames=240, input_script=None, virtual_pad=False,
                      report_prefix=None):
    custom = frames != 240 or input_script is not None or report_prefix is not None
    prefix = report_prefix or ("darkos-pad-check" if virtual_pad else "darkos-userspace-workload" if custom else
              "darkos-userspace-benchmark" if translated else "darkos-preview-benchmark")
    log = directory / f"{prefix}.log"
    vm, port = start_guest(cmd, log)
    received = 0
    start_down = False
    start_up = False
    try:
        with connect_guest(vm, port) as remote:
            remote.settimeout(120)
            try:
                while data := remote.recv(256 * 1024):
                    received += len(data)
                    if virtual_pad and not start_down and received >= FRAME_BYTES * 20:
                        remote.sendall(struct.pack("<H", 1 << 7))
                        start_down = True
                    if start_down and not start_up and received >= FRAME_BYTES * 28:
                        remote.sendall(bytes(2))
                        start_up = True
            except ConnectionResetError:
                pass
        if vm.wait(timeout=8):
            raise RuntimeError("QEMU returned an error")
    finally:
        if vm.poll() is None:
            vm.terminate()
            vm.wait(timeout=8)
    contents = log.read_text(errors="replace")
    if "PREVIEW_BENCH_OK" not in contents or received < FRAME_BYTES * 4:
        raise RuntimeError(f"Preview benchmark did not complete; see {log}")
    rows = [json.loads(line) for line in re.findall(r"GENESISRECOMP_BENCHMARK (\{.*\})", contents)]
    if virtual_pad:
        assert len(rows) == 1 and rows[0]["complete"]
        for marker in ("R36S_PREVIEW_VIRTUAL_PAD_READY", "R36S_PREVIEW_PAD_INPUT_OK", "R36S_PREVIEW_SDL_START_OK"):
            assert marker in contents, f"{marker} missing in {log}"
        print("The ARM64 game's SDL controller received Start from the Windows test")
        return
    if custom:
        if len(rows) != 1 or not rows[0]["complete"] or rows[0]["frames"] != frames:
            raise RuntimeError("The custom workload did not complete")
        report = {"runtime": rows[0], "received_bytes": received,
                  "input_script": input_script.name if input_script else None}
        (directory / f"{prefix}.json").write_text(json.dumps(report, indent=2) + "\n")
        print(f"Workload {frames} frames: {rows[0]['fps']:.2f} fps")
        return
    if translated:
        assert len(rows) == 1
        original = json.loads((directory / "darkos-preview-benchmark.json").read_text())
        rows.insert(0, original["optimized"])
    assert len(rows) == 2 and all(row["complete"] and row["frames"] == 240 for row in rows)
    for key in ("state_fnv1a64", "audio_state_fnv1a64"):
        assert rows[0][key] == rows[1][key], key
    report = {"baseline": rows[0], "optimized": rows[1], "received_bytes": received}
    (directory / f"{prefix}.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Baseline {rows[0]['fps']:.2f} fps; optimized {rows[1]['fps']:.2f} fps; hashes match")


def play_window(cmd, directory, translated=False, virtual_pad=False, control_dir=None, preview_fps=30):
    log = (control_dir / "guest.log" if control_dir else
           directory / ("darkos-userspace-play.log" if translated else "darkos-play-fast.log"))
    vm, port = start_guest(cmd, log)
    root = tk.Tk()
    title = "DB1 · R36S ARM64" + (" · Mando virtual" if virtual_pad else "")
    root.title(title)
    rates = PreviewRates(log)
    root.resizable(False, False)
    root.configure(bg="#111111")
    canvas = tk.Canvas(root, width=640, height=480, bg="#000", highlightthickness=0)
    canvas.pack()
    status = canvas.create_text(320, 240, fill="white", text="Arrancando dArkOS ARM64…")
    output = canvas.create_image(0, 0, anchor="nw")
    frames = queue.Queue(maxsize=1)
    connection = None
    keys = set()
    read_pad = xinput_buttons() if virtual_pad else lambda: 0
    physical_keys = 0
    agent_mask = 0
    agent_busy = False
    pressed_at = {}
    release_jobs = {}
    stopping = False
    last_image = None
    last_frame = None
    snapshot_saved = False
    redraw = None

    def receive():
        nonlocal connection
        try:
            connection = connect_guest(vm, port)
        except RuntimeError:
            return
        connection.settimeout(None)
        try:
            with connection.makefile("rb") as stream:
                while True:
                    frame = stream.read(FRAME_BYTES)
                    if len(frame) != FRAME_BYTES:
                        return
                    try:
                        frames.get_nowait()
                    except queue.Empty:
                        pass
                    frames.put_nowait(frame)
        except OSError:
            return

    def send(value):
        if connection:
            try:
                connection.sendall(struct.pack("<H", value))
            except OSError:
                pass

    def send_keys():
        if not stopping:
            send(agent_mask | physical_keys | sum(1 << k for k in keys))

    def key(event, pressed):
        if event.keysym == "Escape" and pressed:
            close()
            return "break"
        key_name = event.keysym.lower() if len(event.keysym) == 1 else event.keysym
        if key_name in KEYS and not stopping:
            button = KEYS[key_name]
            if button in release_jobs:
                root.after_cancel(release_jobs.pop(button))
            if pressed:
                if button not in keys:
                    pressed_at[button] = time.monotonic()
                    keys.add(button)
                    send_keys()
            else:
                delay = max(0, round(1000 * (0.09 - (time.monotonic() - pressed_at.get(button, 0)))))
                release_jobs[button] = root.after(delay, lambda: release(button))
            return "break"

    def release(button):
        release_jobs.pop(button, None)
        pressed_at.pop(button, None)
        keys.discard(button)
        if not stopping:
            send_keys()

    def release_keys(_event):
        for job in release_jobs.values():
            root.after_cancel(job)
        release_jobs.clear()
        pressed_at.clear()
        keys.clear()
        if not stopping:
            send_keys()

    def close():
        nonlocal stopping
        if stopping:
            return
        stopping = True
        send(256)
        root.after(4000, root.destroy)

    def poll_commands():
        nonlocal agent_busy, agent_mask
        if not control_dir or agent_busy or stopping or last_frame is None:
            return
        request = next(iter(sorted(control_dir.glob("*.request.json"))), None)
        if request is None:
            return
        stem = request.name.removesuffix(".request.json")
        result = control_dir / f"{stem}.result.json"
        try:
            command = json.loads(request.read_text(encoding="utf-8"))
            names = command["buttons"]
            hold, settle = int(command["hold"]), int(command["settle"])
            if len(names) != len(set(names)) or not (0 <= hold <= 4000 and 0 <= settle <= 5000):
                raise ValueError("Invalid button request")
            agent_mask = sum(1 << BUTTONS[name] for name in names)
        except (KeyError, ValueError, TypeError) as error:
            atomic_json(result, {"ok": False, "error": str(error)})
            request.unlink()
            return
        request.unlink()
        agent_busy = True
        send_keys()

        def snapshot():
            nonlocal agent_busy
            picture = control_dir / f"{stem}.png"
            last_frame.save(picture)
            atomic_json(result, {"ok": True, "buttons": names, "snapshot": str(picture.resolve()),
                                 "log": str(log.resolve())})
            agent_busy = False

        def stop_keys():
            nonlocal agent_mask
            agent_mask = 0
            send_keys()
            root.after(settle, snapshot)

        root.after(hold, stop_keys)

    def update_title():
        if vm.poll() is not None:
            root.title(f"{title} · VM cerrada")
            return
        if last_image is not None:
            root.title(f"{title} · {rates.title(time.monotonic())}")
        else:
            rates.since = time.monotonic()
        root.after(1000, update_title)

    def draw():
        nonlocal last_image, last_frame, snapshot_saved, redraw, physical_keys
        observed_pad = read_pad() if root.focus_displayof() else 0
        if observed_pad != physical_keys:
            physical_keys = observed_pad
            if not stopping:
                send_keys()
        try:
            data = frames.get_nowait()
        except queue.Empty:
            data = None
        if data:
            frame = Image.frombytes("RGB", (320, 240), data, "raw", "BGRX")
            last_frame = frame
            if not snapshot_saved and frame.getbbox():
                frame.save(directory / "db1-r36s-live.png")
                snapshot_saved = True
                if control_dir:
                    atomic_json(control_dir / "ready.json", {"log": str(log.resolve()), "window": root.title()})
            last_image = ImageTk.PhotoImage(frame.resize((640, 480), Image.Resampling.NEAREST))
            canvas.itemconfigure(output, image=last_image)
            rates.views += 1
        poll_commands()
        vm_status = vm.poll()
        if vm_status is not None:
            if stopping or vm_status == 0:
                root.destroy()
                return
            if not last_image:
                canvas.itemconfigure(status, text=f"La VM se cerró. Consulta {log.name}.")
        redraw = root.after(8 if preview_fps == 60 else 16, draw)

    root.bind("<KeyPress>", lambda event: key(event, True))
    root.bind("<KeyRelease>", lambda event: key(event, False))
    root.bind("<FocusOut>", release_keys)
    root.bind("<F8>", lambda _event: last_frame and last_frame.save(directory / "db1-r36s-live.png"))
    root.protocol("WM_DELETE_WINDOW", close)
    threading.Thread(target=receive, daemon=True).start()
    if not control_dir:
        root.after(150, root.focus_force)
    root.after(1000, update_title)
    draw()
    try:
        root.mainloop()
    finally:
        if redraw:
            try:
                root.after_cancel(redraw)
            except tk.TclError:
                pass
        if connection:
            connection.close()
        if vm.poll() is None:
            vm.terminate()
            vm.wait(timeout=8)
