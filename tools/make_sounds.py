#!/usr/bin/env python3
"""Makes the teddy bear's warning, ShippingCrate/data/sounds/evacuate.ogg: "Эвакуируйтесь!"
("Evacuate!") in Russian, from the espeak-ng library and its Russian voice (Ubuntu packages
libespeak-ng1 and espeak-ng-data; the espeak-ng command itself isn't needed), pitched low and
slow, with a little echo, as mono 44.1 kHz Vorbis like the game's own sounds.

    python3 tools/make_sounds.py
"""
import ctypes, os, subprocess, tempfile, wave

TEXT = "Эвакуируйтесь!"
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
OUT = os.path.join(ROOT, 'ShippingCrate', 'data', 'sounds', 'evacuate.ogg')

AUDIO_OUTPUT_SYNCHRONOUS = 2
POS_CHARACTER = 1
ESPEAK_CHARS_UTF8 = 1
ESPEAK_RATE, ESPEAK_VOLUME, ESPEAK_PITCH = 1, 2, 3

lib = ctypes.CDLL('libespeak-ng.so.1')
rate = lib.espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 500, None, 0)
if rate <= 0:
    raise SystemExit('espeak-ng failed to start')

samples = bytearray()
CALLBACK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(ctypes.c_short), ctypes.c_int, ctypes.c_void_p)

@CALLBACK
def collect(wav, count, events):
    if wav and count > 0:
        samples.extend(ctypes.string_at(wav, count * 2))
    return 0

lib.espeak_SetSynthCallback(collect)
if lib.espeak_SetVoiceByName(b'ru') != 0:
    raise SystemExit('no Russian voice in espeak-ng-data')
lib.espeak_SetParameter(ESPEAK_RATE, 120, 0)     # words a minute: slow
lib.espeak_SetParameter(ESPEAK_PITCH, 20, 0)     # 0-100: low
lib.espeak_SetParameter(ESPEAK_VOLUME, 120, 0)
text = TEXT.encode('utf-8')
lib.espeak_Synth(text, len(text) + 1, 0, POS_CHARACTER, 0, ESPEAK_CHARS_UTF8, None, None)
lib.espeak_Synchronize()
if not samples:
    raise SystemExit('espeak-ng produced no audio')

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with tempfile.TemporaryDirectory() as tmp:
    raw = os.path.join(tmp, 'raw.wav')
    with wave.open(raw, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(bytes(samples))
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-i', raw,
                    '-af', 'aresample=44100,silenceremove=start_periods=1:start_threshold=-50dB,'
                           'aecho=0.8:0.6:60:0.3,loudnorm=I=-12:TP=-1,aresample=44100,apad=pad_dur=0.1',
                    '-ac', '1', '-c:a', 'libvorbis', '-q:a', '6', OUT], check=True)
print('wrote', os.path.normpath(OUT), os.path.getsize(OUT), 'bytes')
