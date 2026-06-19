"""Dump IDB records and find appMain widget validation."""
import struct

with open('examples/periodic/periodic.uid', 'rb') as f:
    data = f.read()

REC_SZ = 4096

# Record 1: file offset 0x1000
print("=== Record 1 (offset 0x1000) ===")
r1 = data[0x1000:0x1100]
validation = struct.unpack_from('<I', r1, 0)[0]
record_type = struct.unpack_from('<H', r1, 4)[0]
record_num = struct.unpack_from('<H', r1, 6)[0]
print(f"  IDBRecordHeader: validation=0x{validation:08x} type={record_type} rec_num={record_num}")

# Show raw bytes 8..199
print("\n  Bytes 8..199:")
for i in range(8, 200, 16):
    hex_str = ' '.join(f'{b:02x}' for b in r1[i:i+16])
    ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in r1[i:i+16])
    print(f"  {i:4d}: {hex_str}  {ascii_str}")

# Search for "appMain" in the file
pos = data.find(b'appMain')
print(f"\n'appMain' string at file offset 0x{pos:x}")

# Show context around appMain
ctx_start = max(0, pos - 32)
ctx_end = min(len(data), pos + 64)
print(f"\n  Context around 'appMain' (offset 0x{ctx_start:x} - 0x{ctx_end:x}):")
ctx = data[ctx_start:ctx_end]
for i in range(0, len(ctx), 16):
    off = ctx_start + i
    hex_str = ' '.join(f'{b:02x}' for b in ctx[i:i+16])
    ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in ctx[i:i+16])
    print(f"  {off:5d}: {hex_str}  {ascii_str}")

# Check all 36 records' types
print("\n=== All record types ===")
for rec_no in range(36):
    off = rec_no * REC_SZ
    validation, rtype, rnum = struct.unpack_from('<I', data, off)[0], struct.unpack_from('<H', data, off + 4)[0], struct.unpack_from('<H', data, off + 6)[0]
    if validation == 0x127C6F72 or rtype > 0:
        print(f"  rec {rec_no:2d} (0x{off:05x}): validation=0x{validation:08x} type={rtype} num={rnum}")

# Also check record 0
print("\n=== Record 0 (offset 0x0000) ===")
r0 = data[0:64]
for i in range(0, 64, 16):
    hex_str = ' '.join(f'{b:02x}' for b in r0[i:i+16])
    ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in r0[i:i+16])
    print(f"  {i:4d}: {hex_str}  {ascii_str}")
