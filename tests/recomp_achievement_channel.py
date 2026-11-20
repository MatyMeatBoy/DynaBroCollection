"""Exercise the Dyna achievement observer and SDK with a local test pipe."""

import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import threading
import uuid


def listen(path, ready, messages, errors):
    try:
        if os.name == "nt":
            from multiprocessing.connection import Listener

            with Listener(path, family="AF_PIPE") as server:
                ready.set()
                with server.accept() as conn:
                    pending = b""
                    try:
                        while True:
                            pending += conn.recv_bytes()
                            lines = pending.split(b"\n")
                            pending = lines.pop()
                            messages.extend(json.loads(line) for line in lines)
                    except EOFError:
                        assert not pending
        else:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as server:
                server.bind(path)
                server.listen()
                ready.set()
                with server.accept()[0] as conn, conn.makefile("rb") as reader:
                    messages.extend(json.loads(line) for line in reader)
    except Exception as error:
        errors.append(error)
        ready.set()


def run(executable, mode, connected=True):
    with tempfile.TemporaryDirectory() as directory:
        folder = Path(directory)
        game = 1 if mode == "db1" else 2
        profile = folder / f"dynabrothers-achievements-db{game}.txt"
        seed = (183303,) if game == 1 else range(2000001, 2000008)
        saved = "".join(f"{id_} 1\n" for id_ in seed)
        profile.write_text(f"DYNA_ACHIEVEMENTS 1 {game}\n{saved}", encoding="ascii")
        env = os.environ.copy()
        env.pop("RL_SESSION", None)
        messages = []
        if connected:
            path = (r"\\.\pipe\RecompLauncher-DynaTest-" + uuid.uuid4().hex
                    if os.name == "nt" else str(folder / "test.sock"))
            ready = threading.Event()
            errors = []
            receiver = threading.Thread(target=listen, args=(path, ready, messages, errors), daemon=True)
            receiver.start()
            assert ready.wait(5) and not errors, errors
            env["RL_SESSION"] = path
        subprocess.run([executable, str(folder / "settings.ini"), mode], env=env, check=True, timeout=10)
        if connected:
            receiver.join(5)
            assert not receiver.is_alive() and not errors, errors
        return messages, profile.read_text(encoding="ascii")


def main():
    executable, version, manifest_directory = sys.argv[1:]
    source = Path(__file__).resolve().parents[1]
    manifests = Path(manifest_directory)
    for game, expected in ((1, 76), (2, 99)):
        catalog = json.loads((source / "assets" / "achievements" / f"db{game}.json").read_text(encoding="utf-8"))
        name = f"dynabrocollection--db{game}"
        manifest = json.loads((manifests / f"{name}.json").read_text(encoding="utf-8"))
        assert manifest["format"] == 1 and manifest["source"] == "events" and manifest["game"] == name
        assert manifest["versions"] == "0.5.0-beta.1, 0.5.0"
        assert len(manifest["achievements"]) == expected
        assert {row["id"] for row in manifest["achievements"]} == {row["id"] for row in catalog["achievements"]}
        assert all(row["points"] in (5, 10, 25, 50) and row["event"] == f'dyna_{row["id"]}'
                   for row in manifest["achievements"])
        if game == 1:
            assert all("badge" not in row for row in manifest["achievements"])

    messages, profile = run(executable, "db1")
    assert [m["t"] for m in messages] == ["hello", "save_loaded", "event", "casual"], messages
    assert messages[0]["game"] == "dynabrocollection--db1" and messages[0]["version"] == version
    assert messages[0]["memory"] is False
    assert messages[2]["name"] == "dyna_183303" and messages[2]["frame"] > 0
    assert messages[3]["reason"] == "diagnostic" and "183304 " not in profile

    messages, profile = run(executable, "final")
    assert [m["t"] for m in messages] == ["hello", "event"], messages
    assert messages[0]["game"] == "dynabrocollection--db2" and messages[0]["version"] == version
    assert messages[1]["name"] == "dyna_2000008"
    assert "2000900 " in profile

    messages, _ = run(executable, "db2")
    assert [m["name"] for m in messages[1:]] == [
        "dyna_2000008", *(f"dyna_{id_}" for id_ in range(2000001, 2000008)), "dyna_2000900"
    ], messages
    messages, profile = run(executable, "cheat")
    assert [m["t"] for m in messages] == ["hello", "casual"], messages
    assert messages[1]["reason"] == "cheats" and "2000008 " not in profile

    messages, profile = run(executable, "final", connected=False)
    assert not messages and "2000900 " in profile
    print("Dyna Recomp Achievements channel contract passed")


if __name__ == "__main__":
    main()
