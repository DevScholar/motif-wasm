"""Parse periodic.uid and dump widget record creation callbacks."""
import struct

with open('examples/periodic/periodic.uid', 'rb') as f:
    data = f.read()

# IDB record size is 4096
REC_SZ = 4096

def find_all(pattern):
    """Find all offsets of pattern in data."""
    offs = []
    pos = 0
    while True:
        pos = data.find(pattern, pos)
        if pos == -1:
            break
        offs.append(pos)
        pos += 1
    return offs

# URMWidgetRecordValid (from file): 0x1649F7E2 = bytes e2 f7 49 16
# Actually let's look for the cbdesc pattern: URMCallbackDescriptorValid = 0x0666c168
CALLBACK_DESC_VALID = bytes([0x68, 0xc1, 0x66, 0x06])
IDB_ENTRY_VALID = bytes([0xae, 0x88, 0x48, 0x0d])
WIDGET_REC_VALID_FILE = bytes([0xe2, 0xf7, 0x49, 0x16])

print("=== Searching for IDBDataEntryValid markers ===")
entry_positions = find_all(IDB_ENTRY_VALID)
print(f"Found {len(entry_positions)} IDB data entries")

print("\n=== Searching for URMWidgetRecordValid (file version 0x1649F7E2) ===")
widget_positions = find_all(WIDGET_REC_VALID_FILE)
print(f"Found {len(widget_positions)} widget records")

# For each widget record, parse its header and dump creation callbacks
print("\n=== Widget Records with creation_offs > 0 ===")
for wpos in widget_positions:
    # RGMWidgetRecord header: 40 bytes, but data may start at different offset
    # Try: widget record data starts at wpos (which is the validation field)
    # struct: validation(4) size(2) access(2) lock(2) type(2)
    #         name_offs(2) class_offs(2) arglist_offs(2) children_offs(2)
    #         comment_offs(2) creation_offs(2) variety(8) annex(8)
    off = wpos + 4  # skip validation
    size = struct.unpack_from('<H', data, off)[0]; off += 2
    access = struct.unpack_from('<H', data, off)[0]; off += 2
    lock = struct.unpack_from('<H', data, off)[0]; off += 2
    wtype = struct.unpack_from('<H', data, off)[0]; off += 2
    name_offs = struct.unpack_from('<H', data, off)[0]; off += 2
    class_offs = struct.unpack_from('<H', data, off)[0]; off += 2
    arglist_offs = struct.unpack_from('<H', data, off)[0]; off += 2
    children_offs = struct.unpack_from('<H', data, off)[0]; off += 2
    comment_offs = struct.unpack_from('<H', data, off)[0]; off += 2
    creation_offs = struct.unpack_from('<H', data, off)[0]; off += 2

    if creation_offs == 0:
        continue

    # Extract widget name
    name = ''
    if name_offs > 0 and name_offs < size:
        name_bytes = data[wpos + name_offs:wpos + size]
        null_pos = name_bytes.find(b'\x00')
        if null_pos >= 0:
            name = name_bytes[:null_pos].decode('ascii', errors='replace')

    print(f"\nWidget at file offset 0x{wpos:x}: size={size} name='{name}' creation_offs={creation_offs}")

    # Parse callback descriptor
    cbdesc_off = wpos + creation_offs
    if cbdesc_off + 16 > len(data):
        print(f"  cbdesc at 0x{cbdesc_off:x} out of bounds")
        continue

    cb_validation = struct.unpack_from('<I', data, cbdesc_off)[0]
    cb_count = struct.unpack_from('<H', data, cbdesc_off + 4)[0]
    cb_annex = struct.unpack_from('<H', data, cbdesc_off + 6)[0]
    cb_unres = struct.unpack_from('<H', data, cbdesc_off + 8)[0]
    cb_pad1 = struct.unpack_from('<H', data, cbdesc_off + 10)[0]
    cb_pad2 = struct.unpack_from('<I', data, cbdesc_off + 12)[0]

    print(f"  cbdesc at +{creation_offs}: validation=0x{cb_validation:08x} count={cb_count} _pad1={cb_pad1}")

    if cb_validation != 0x0666c168:
        print(f"  WARNING: cbdesc validation mismatch!")
        continue

    for i in range(cb_count):
        item_off = cbdesc_off + 16 + i * 40  # RGMCallbackItem = 40 bytes
        if item_off + 8 > len(data):
            break
        routine = struct.unpack_from('<H', data, item_off)[0]
        rep_type = struct.unpack_from('<H', data, item_off + 2)[0]
        datum_ival = struct.unpack_from('<i', data, item_off + 4)[0]

        # String at wpos + routine
        str_off = wpos + routine
        str_bytes = data[str_off:str_off + 64]
        null_pos = str_bytes.find(b'\x00')
        if null_pos >= 0:
            str_bytes = str_bytes[:null_pos]
        try:
            str_text = str_bytes.decode('ascii', errors='replace')
        except:
            str_text = repr(str_bytes)

        # Also check at wpos + routine - 4
        alt_str_off = wpos + routine - 4
        alt_bytes = data[alt_str_off:alt_str_off + 64]
        alt_null = alt_bytes.find(b'\x00')
        if alt_null >= 0:
            alt_bytes = alt_bytes[:alt_null]
        try:
            alt_text = alt_bytes.decode('ascii', errors='replace')
        except:
            alt_text = repr(alt_bytes)

        print(f"  item[{i}]: routine=0x{routine:04x}({routine}) rep_type={rep_type} datum={datum_ival}")
        print(f"    string at +{routine}: '{str_text}'")
        print(f"    string at +{routine-4}: '{alt_text}'")
        print(f"    raw bytes at +{routine}: {' '.join(f'{b:02x}' for b in data[str_off:str_off+24])}")

print("\n=== Known callback strings in file ===")
callback_names = [
    b'InitPopupCb', b'PopdownCb', b'UnmanageCb', b'ManageCb',
    b'DaExposeCb', b'DaResizeCb', b'DbExposeCb', b'DbResizeCb',
    b'ScaleCb', b'SetScaleCb', b'ViewCb', b'LayoutCb',
    b'ToggleLightsCb', b'ShowCb', b'ExitCb', b'ScrollVisibleCb',
    b'ToggleValueChangedCb', b'ToggleControlCb', b'InitPopupCb',
]
for name in callback_names:
    pos = data.find(name)
    if pos >= 0:
        print(f"  '{name.decode()}' at file offset 0x{pos:x}")
