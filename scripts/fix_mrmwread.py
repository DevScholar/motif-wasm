"""Fix the broken fprintf line in Mrmwread.c."""
target = 'ignored-area/third-party/motif/src/lib/Mrm/Mrmwread.c'
with open(target, 'r', encoding='utf-8') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if 'fprintf(stderr, "DEBUG VALIDATE:' in line:
        del lines[i+1]  # continuation line with close quote
        lines[i] = ('  fprintf(stderr, "DEBUG VALIDATE: index=%s validation=0x%08x expected=0x%08x swapped=%d\\n",'
                     ' index, widgetrec->validation, URMWidgetRecordValid, (*file_id_return)->byte_swapped);\n')
        print(f'Fixed line {i+1}')
        break

with open(target, 'w', encoding='utf-8') as f:
    f.writelines(lines)
print('Done')
