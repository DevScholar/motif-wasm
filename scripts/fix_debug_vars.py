"""Fix the debug fprintf variable names in Mrmwread.c."""
target = 'ignored-area/third-party/motif/src/lib/Mrm/Mrmwread.c'
with open(target, 'r') as f:
    content = f.read()

# Fix line ~207: UrmGetIndexedWidget uses file_id, not file_id_return
old1 = "(*file_id_return)->byte_swapped, widgetrec->size);\n"
old1 += "  if ( UrmWRValid(widgetrec) )\n"
old1 += "    return MrmSUCCESS ;\n"
old1 += "  else\n"
old1 += "    {\n"
old1 += "      if ( file_id->byte_swapped ) swapbytes(widgetrec->validation);"

new1 = "file_id->byte_swapped, widgetrec->size);\n"
new1 += "  if ( UrmWRValid(widgetrec) )\n"
new1 += "    return MrmSUCCESS ;\n"
new1 += "  else\n"
new1 += "    {\n"
new1 += "      if ( file_id->byte_swapped ) swapbytes(widgetrec->validation);"

count = content.count(old1)
if count > 0:
    content = content.replace(old1, new1)
    print(f'Fixed UrmGetIndexedWidget ({count} occurrences)')
else:
    print('Pattern 1 not found')

# Fix line ~281: UrmGetRIDWidget also uses file_id
old2 = "(*file_id_return)->byte_swapped, widgetrec->size);\n"
old2 += "  if ( UrmWRValid(widgetrec) )\n"
old2 += "    return MrmSUCCESS ;\n"
old2 += "  else\n"
old2 += "    {\n"
old2 += "      if ( file_id->byte_swapped ) swapbytes(widgetrec->validation);\n"
old2 += "      if ( UrmWRValid(widgetrec) )\n"
old2 += "        {\n"
old2 += "          Urm__SwapRGMWidgetRecord(widgetrec);\n"
old2 += "          return MrmSUCCESS ;\n"
old2 += "        }\n"
old2 += "    }\n"
old2 += "\n"
old2 += "  return Urm__UT_Error(\"UrmGetRIDWidget\""

new2 = "file_id->byte_swapped, widgetrec->size);\n"
new2 += "  if ( UrmWRValid(widgetrec) )\n"
new2 += "    return MrmSUCCESS ;\n"
new2 += "  else\n"
new2 += "    {\n"
new2 += "      if ( file_id->byte_swapped ) swapbytes(widgetrec->validation);\n"
new2 += "      if ( UrmWRValid(widgetrec) )\n"
new2 += "        {\n"
new2 += "          Urm__SwapRGMWidgetRecord(widgetrec);\n"
new2 += "          return MrmSUCCESS ;\n"
new2 += "        }\n"
new2 += "    }\n"
new2 += "\n"
new2 += "  return Urm__UT_Error(\"UrmGetRIDWidget\""

count = content.count(old2)
if count > 0:
    content = content.replace(old2, new2)
    print(f'Fixed UrmGetRIDWidget ({count} occurrences)')
else:
    print('Pattern 2 not found')

with open(target, 'w') as f:
    f.write(content)

# Show remaining debug lines for verification
for i, line in enumerate(content.split('\n')):
    if 'DEBUG VALIDATE' in line:
        print(f'Line {i+1}: {line.rstrip()}')
