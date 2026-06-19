"""Search for validation constants in wasm binary by looking for i32.const instructions."""
import struct

# The constants as little-endian 32-bit integers
# i32.const in wasm is opcode 0x41 followed by LEB128 encoded value
# For values < 128, it's just 0x41 <byte>

def find_leb128_u32(data, value):
    """Find i32.const instructions loading a specific value."""
    results = []
    i = 0
    while i < len(data) - 1:
        if data[i] == 0x41:  # i32.const
            # Decode LEB128
            i += 1
            result = 0
            shift = 0
            while i < len(data):
                byte = data[i]
                result |= (byte & 0x7f) << shift
                shift += 7
                i += 1
                if (byte & 0x80) == 0:
                    break
            if result == value:
                results.append(i)
        else:
            i += 1
    return results

with open('examples/periodic/periodic.uid', 'rb') as f:
    pass  # just testing path

import sys
wasm_path = 'build/artifacts/periodic/periodic.wasm'
try:
    with open(wasm_path, 'rb') as f:
        wasm = f.read()

    for label, val in [
        ('0x1649AFE2 (old code)', 0x1649AFE2),
        ('0x1649F7E2 (file match)', 0x1649F7E2),
        ('373946338 (old decimal)', 373946338),
        ('373948386 (new decimal)', 373948386),
    ]:
        positions = find_leb128_u32(wasm, val)
        print(f'{label}: {len(positions)} occurrences')
        if positions:
            for p in positions[:5]:
                print(f'  at wasm offset 0x{p:x}')

    # Also search for raw byte patterns
    print()
    for label, pat in [
        ('old code bytes', b'\xe2\xaf\x49\x16'),
        ('file bytes', b'\xe2\xf7\x49\x16'),
    ]:
        count = wasm.count(pat)
        print(f'{label}: {count}')
except FileNotFoundError:
    print(f'Could not open {wasm_path}')
    print('Trying relative path from script dir...')
