"""Add debug fprintf to Mrmwread.c validation check."""
target = 'ignored-area/third-party/motif/src/lib/Mrm/Mrmwread.c'
with open(target, 'r') as f:
    content = f.read()

# The pattern: widgetrec assignment followed by validation check
# We need to insert fprintf between these two lines
old = "  widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;\n  if ( UrmWRValid(widgetrec) )"
new = ("  widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;\n"
       "  fprintf(stderr, \"DEBUG VALIDATE: index=%%s valid=0x%%08x expect=0x%%08x swapped=%%d size=%%d\\n\","
       " index, widgetrec->validation, URMWidgetRecordValid, (*file_id_return)->byte_swapped,"
       " widgetrec->size);\n"
       "  if ( UrmWRValid(widgetrec) )")

if old in content:
    content = content.replace(old, new)
    with open(target, 'w') as f:
        f.write(content)
    print('PATCHED OK')
else:
    print('NOT FOUND - searching...')
    for i, line in enumerate(content.split('\n')):
        if 'UrmRCBuffer (context_id)' in line or 'UrmWRValid(widgetrec)' in line:
            print(f'  Line {i+1}: {repr(line)}')
