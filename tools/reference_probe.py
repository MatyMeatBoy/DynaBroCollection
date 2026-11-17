"""Offline libretro oracle for local ROM qualification; not the product runtime."""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
from PIL import Image


class GameInfo(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p), ('size', C.c_size_t), ('meta', C.c_char_p)]


class Variable(C.Structure):
    _fields_ = [('key', C.c_char_p), ('value', C.c_char_p)]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('rom', type=Path)
    p.add_argument('--core', type=Path, default=Path('local/oracle/genesis_plus_gx_libretro.dll'))
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--frames', type=int, default=1800)
    p.add_argument('--start', type=int, action='append', default=[])
    p.add_argument('--press-a', type=int, default=-1)
    p.add_argument('--press', action='append', default=[], metavar='BUTTON:FRAME:DURATION')
    p.add_argument('--capture', type=int, action='append', default=[])
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    core = C.CDLL(str(args.core.resolve()))
    env_type = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
    video_type = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
    audio_type = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
    batch_type = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
    poll_type = C.CFUNCTYPE(None)
    input_type = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)
    pixel_format, frame, audio_samples = 0, 0, 0
    variables = {}
    input_calls = {}
    pressed_calls = 0
    directory = str(args.output.resolve()).encode()
    screenshot_frames = {120, 300, 600, 1200, args.frames, *args.capture}
    button_ids = dict(A=1, B=0, C=8, START=3, UP=4, DOWN=5, LEFT=6, RIGHT=7, X=9, Y=10, Z=11, MODE=2)
    presses = []
    for item in args.press:
        button, start, duration = item.upper().split(':')
        presses.append((button_ids[button], int(start), int(duration)))

    @env_type
    def environment(command, data):
        nonlocal pixel_format
        if command in (9, 31, 30):  # system/save/content directory
            C.cast(data, C.POINTER(C.c_char_p))[0] = directory
            return True
        if command == 10:
            pixel_format = C.cast(data, C.POINTER(C.c_int))[0]
            return pixel_format in (0, 1, 2)
        if command == 16:  # legacy core options, first choice is default
            v = C.cast(data, C.POINTER(Variable))
            i = 0
            while v[i].key:
                value = v[i].value.split(b'; ', 1)[-1].split(b'|')[0]
                variables[v[i].key] = value
                i += 1
            return True
        if command == 15:
            v = C.cast(data, C.POINTER(Variable)).contents
            if v.key not in variables:
                return False
            v.value = variables[v.key]
            return True
        if command == 17:
            C.cast(data, C.POINTER(C.c_bool))[0] = False
            return True
        if command == 3:
            C.cast(data, C.POINTER(C.c_bool))[0] = False
            return True
        if command == 52:  # refuse newer options API: core supplies legacy fallback
            C.cast(data, C.POINTER(C.c_uint))[0] = 0
            return True
        if command in (11, 18):  # input descriptors / support-no-game
            return True
        return False

    @video_type
    def video(data, width, height, pitch):
        if not data or frame not in screenshot_frames:
            return
        raw = C.string_at(data, pitch * height)
        if pixel_format == 1:
            image = Image.frombytes('RGB', (width, height), raw, 'raw', 'BGRX', pitch)
        else:
            rgb = bytearray(width * height * 3)
            for y in range(height):
                for x in range(width):
                    offset = y * pitch + x * 2
                    word = raw[offset] | raw[offset + 1] << 8
                    if pixel_format == 2:
                        color = ((word >> 11 & 31) * 255 // 31, (word >> 5 & 63) * 255 // 63, (word & 31) * 255 // 31)
                    else:
                        color = ((word >> 10 & 31) * 255 // 31, (word >> 5 & 31) * 255 // 31, (word & 31) * 255 // 31)
                    index = (y * width + x) * 3
                    rgb[index:index + 3] = bytes(color)
            image = Image.frombytes('RGB', (width, height), bytes(rgb))
        image.save(args.output / f'frame-{frame:06d}.png')

    @audio_type
    def audio(left, right):
        nonlocal audio_samples
        audio_samples += 1

    @batch_type
    def batch(data, frames):
        nonlocal audio_samples
        audio_samples += frames
        return frames

    @poll_type
    def poll():
        pass

    @input_type
    def input_state(port, device, index, button):
        nonlocal pressed_calls
        key = f'{port}:{device}:{index}:{button}'
        input_calls[key] = input_calls.get(key, 0) + 1
        mask = 0
        if port == 0:
            if any(start <= frame < start + 12 for start in args.start):
                mask |= 1 << 3
            if args.press_a >= 0 and args.press_a <= frame < args.press_a + 12:
                mask |= 1 << 1
            for button_id, start, duration in presses:
                if start <= frame < start + duration:
                    mask |= 1 << button_id
        value = mask if button == 256 else int(bool(mask & (1 << button))) if button < 16 else 0
        if value:
            pressed_calls += 1
        return value

    for name, callback in [('environment', environment), ('video_refresh', video), ('audio_sample', audio),
                           ('audio_sample_batch', batch), ('input_poll', poll), ('input_state', input_state)]:
        setter = getattr(core, 'retro_set_' + name)
        setter.argtypes = [type(callback)]
        setter(callback)
    core.retro_init()
    core.retro_load_game.argtypes = [C.POINTER(GameInfo)]
    core.retro_load_game.restype = C.c_bool
    rom = args.rom.read_bytes()
    storage = C.create_string_buffer(rom)
    info = GameInfo(str(args.rom.resolve()).encode(), C.cast(storage, C.c_void_p), len(rom), None)
    if not core.retro_load_game(C.byref(info)):
        raise RuntimeError('Oracle rejected ROM')
    core.retro_set_controller_port_device.argtypes = [C.c_uint, C.c_uint]
    core.retro_set_controller_port_device(0, 1)
    for frame in range(1, args.frames + 1):
        core.retro_run()
    core.retro_get_memory_data.argtypes = [C.c_uint]
    core.retro_get_memory_data.restype = C.c_void_p
    core.retro_get_memory_size.argtypes = [C.c_uint]
    core.retro_get_memory_size.restype = C.c_size_t
    size = core.retro_get_memory_size(2)
    pointer = core.retro_get_memory_data(2)
    if pointer and size:
        (args.output / 'final.ram').write_bytes(C.string_at(pointer, size))
    core.retro_unload_game()
    core.retro_deinit()
    report = dict(frames=args.frames, audio_samples=audio_samples, pixel_format=pixel_format,
                  rom_sha256=hashlib.sha256(rom).hexdigest(), core_sha256=hashlib.sha256(args.core.read_bytes()).hexdigest(),
                  start=args.start, press_a=args.press_a, presses=args.press, ram_size=size,
                  input_calls=input_calls, pressed_calls=pressed_calls)
    (args.output / 'probe.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
