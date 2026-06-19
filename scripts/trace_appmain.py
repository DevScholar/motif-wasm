"""Trace 'appMain' widget through the IDB B-tree index to find its actual validation bytes."""
import struct

with open('examples/periodic/periodic.uid', 'rb') as f:
    data = f.read()

REC_SZ = 4096

def get_record(rec_no):
    off = rec_no * REC_SZ
    return off, data[off:off + REC_SZ]

def parse_record_header(rec_data):
    validation = struct.unpack_from('<I', rec_data, 0)[0]
    record_type = struct.unpack_from('<H', rec_data, 4)[0]
    record_num = struct.unpack_from('<H', rec_data, 6)[0]
    return validation, record_type, record_num

# Parse header record (record number 1)
off, rec1 = get_record(1)
print(f"=== Record 1 (header) at file offset 0x{off:x} ===")

validation, rtype, rnum = parse_record_header(rec1)
print(f"  Header validation=0x{validation:08x} type={rtype} rec_num={rnum}")

hhdr = rec1[8:]
db_version = hhdr[0:10].split(b'\x00')[0].decode('ascii', errors='replace')
creator = hhdr[10:18].split(b'\x00')[0].decode('ascii', errors='replace')
creator_version = hhdr[18:28].split(b'\x00')[0].decode('ascii', errors='replace')
creation_date = hhdr[28:44].split(b'\x00')[0].decode('ascii', errors='replace')
module = hhdr[44:58].split(b'\x00')[0].decode('ascii', errors='replace')
module_version = hhdr[58:68].split(b'\x00')[0].decode('ascii', errors='replace')

print(f"  db_version='{db_version}'")
print(f"  creator='{creator}'")
print(f"  creator_version='{creator_version}'")
print(f"  module='{module}'")
print(f"  module_version='{module_version}'")

index_root = struct.unpack_from('<H', hhdr, 68)[0]
num_indexed = struct.unpack_from('<H', hhdr, 70)[0]
print(f"  index_root={index_root} num_indexed={num_indexed}")

# Determine B-tree entry sizes by probing
# Leaf: IDBIndexLeafHdr(8) + IDBRecordHeader(8) = 16 byte header
# Entry: index_stg(2) + IDBDataPointer(4) = 6 bytes (ILP32) or more (LP64)

def walk_tree(rec_no, target, depth=0):
    """Recursively walk B-tree, find target name, return (data_ptr, name)."""
    off, rec = get_record(rec_no)
    _, rtype, _ = parse_record_header(rec)
    indent = "  " * (depth + 1)

    if rtype == 3:  # Leaf
        parent = struct.unpack_from('<H', rec, 8)[0]
        index_count = struct.unpack_from('<H', rec, 10)[0]
        heap_start = struct.unpack_from('<H', rec, 12)[0]

        # Try different entry sizes
        for entry_size in [6, 8, 10, 12]:
            entries = []
            valid = True
            for i in range(index_count):
                entry_off = 16 + i * entry_size
                if entry_off + 6 > REC_SZ:
                    valid = False
                    break
                index_stg = struct.unpack_from('<H', rec, entry_off)[0]
                data_ptr = struct.unpack_from('<I', rec, entry_off + 2)[0]
                str_off = 16 + heap_start + index_stg
                name = ''
                if str_off < REC_SZ:
                    str_bytes = rec[str_off:str_off+64]
                    null_pos = str_bytes.find(b'\x00')
                    if null_pos >= 0:
                        name = str_bytes[:null_pos].decode('ascii', errors='replace')
                entries.append((name, data_ptr))
                if name == target:
                    print(f"{indent}FOUND '{target}' at rec={rec_no}, entry_size={entry_size}, data_ptr=0x{data_ptr:08x}")
                    return data_ptr, name

            # Validate: first few names should look reasonable
            sample = [e[0] for e in entries[:5]]
            if valid and sample and all(len(s) > 0 and len(s) < 120 for s in sample):
                print(f"{indent}Leaf rec={rec_no} esize={entry_size}: {len(entries)} entries, e.g. {sample}")
                break
        else:
            print(f"{indent}Leaf rec={rec_no}: could not find valid entry size")
        return None, None

    elif rtype == 4:  # Node
        parent = struct.unpack_from('<H', rec, 8)[0]
        index_count = struct.unpack_from('<H', rec, 10)[0]
        heap_start = struct.unpack_from('<H', rec, 12)[0]

        for entry_size in [10, 12, 14, 16]:
            entries = []
            valid = True
            for i in range(index_count):
                entry_off = 16 + i * entry_size
                if entry_off + 10 > REC_SZ:
                    valid = False
                    break
                index_stg = struct.unpack_from('<H', rec, entry_off)[0]
                data_ptr = struct.unpack_from('<I', rec, entry_off + 2)[0]
                lt = struct.unpack_from('<H', rec, entry_off + 6)[0]
                gt = struct.unpack_from('<H', rec, entry_off + 8)[0]
                str_off = 16 + heap_start + index_stg
                name = ''
                if str_off < REC_SZ:
                    str_bytes = rec[str_off:str_off+64]
                    null_pos = str_bytes.find(b'\x00')
                    if null_pos >= 0:
                        name = str_bytes[:null_pos].decode('ascii', errors='replace')
                entries.append((name, data_ptr, lt, gt))
                if name == target:
                    print(f"{indent}FOUND '{target}' at node rec={rec_no}, data_ptr=0x{data_ptr:08x}")
                    return data_ptr, name

            sample = [e[0] for e in entries[:3]]
            if valid and sample and all(len(s) > 0 and len(s) < 120 for s in sample):
                print(f"{indent}Node rec={rec_no} esize={entry_size}: {len(entries)} entries")
                for name, dp, lt, gt in entries:
                    print(f"{indent}  '{name}' dp=0x{dp:08x} lt={lt} gt={gt}")
                # Navigate: entries are sorted, find where target belongs
                for i, (name, dp, lt, gt) in enumerate(entries):
                    if name == target:
                        return dp, name
                    if target < name:
                        if lt > 0:
                            result, found = walk_tree(lt, target, depth + 1)
                            if result:
                                return result, found
                        break
                else:
                    # target > all entries, follow last GT
                    if entries and entries[-1][3] > 0:
                        result, found = walk_tree(entries[-1][3], target, depth + 1)
                        if result:
                            return result, found
                break
        else:
            print(f"{indent}Node rec={rec_no}: could not find valid entry size")
        return None, None
    else:
        print(f"{indent}Unknown record type {rtype} at rec {rec_no}")
        return None, None

print(f"\n=== Walking B-tree from index_root={index_root} ===")
data_ptr, name = walk_tree(index_root, 'appMain')

print(f"\n=== Result ===")
if data_ptr:
    rec_no = data_ptr >> 16
    item_offs = data_ptr & 0xFFFF
    print(f"  data_ptr=0x{data_ptr:08x} -> rec_no={rec_no}, item_offs={item_offs} (0x{item_offs:x})")

    off, rec = get_record(rec_no)
    entry_start = off + item_offs
    print(f"  Entry at file offset 0x{entry_start:x}")

    # Parse IDBDataEntryHdr
    eh = data[entry_start:entry_start+18]
    entry_validation = struct.unpack_from('<I', eh, 0)[0]
    entry_type = struct.unpack_from('<H', eh, 4)[0]
    resource_group = struct.unpack_from('<H', eh, 6)[0]
    resource_type = struct.unpack_from('<H', eh, 8)[0]
    access = struct.unpack_from('<H', eh, 10)[0]
    lock = struct.unpack_from('<H', eh, 12)[0]
    entry_size = struct.unpack_from('<H', eh, 14)[0]
    prev_entry = struct.unpack_from('<H', eh, 16)[0]

    print(f"  IDBDataEntryHdr:")
    print(f"    validation=0x{entry_validation:08x} (expect 0x0D4888AE)")
    print(f"    entry_type={entry_type} resource_group={resource_group} resource_type={resource_type}")
    print(f"    access={access} lock={lock} entry_size={entry_size} prev_entry={prev_entry}")

    # Widget record data at entry_start + 18
    widget_start = entry_start + 18
    widget_validation = struct.unpack_from('<I', data, widget_start)[0]

    print(f"\n  Widget record at 0x{widget_start:x}:")
    print(f"    validation=0x{widget_validation:08x}")
    print(f"    original code expects: 0x1649AFE2 {'<- MATCH' if widget_validation == 0x1649AFE2 else ''}")
    print(f"    file has:              0x1649F7E2 {'<- MATCH' if widget_validation == 0x1649F7E2 else ''}")

    # Dump raw
    raw = data[widget_start:widget_start+64]
    print(f"\n  Raw bytes [0..63]:")
    for i in range(0, 64, 16):
        hex_str = ' '.join(f'{b:02x}' for b in raw[i:i+16])
        ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in raw[i:i+16])
        print(f"    {i:4d}: {hex_str}  {ascii_str}")
else:
    print("  appMain NOT FOUND in B-tree!")
