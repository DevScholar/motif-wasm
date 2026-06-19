"""Add debug fprintf to Mrmwread.c before validation check."""
import sys
sys.stdout.reconfigure(encoding='utf-8')

target = 'ignored-area/third-party/motif/src/lib/Mrm/Mrmwread.c'
with open(target, 'rb') as f:
    content = f.read()

# Unique marker: the validation check in UrmHGetWidget
old = b'''  widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
  if ( UrmWRValid(widgetrec) )
    return MrmSUCCESS ;
  else
    {
      if ( (*file_id_return)->byte_swapped ) swapbytes(widgetrec->validation);
      if ( UrmWRValid(widgetrec) )
\t{
\t  Urm__SwapRGMWidgetRecord(widgetrec);
\t  return MrmSUCCESS ;
\t}
    }

  return Urm__UT_Error("UrmHGetIndexedWidget", _MrmMMsg_0026,
\t\t       NULL, context_id, MrmBAD_WIDGET_REC) ;'''

new = b'''  widgetrec = (RGMWidgetRecordPtr) UrmRCBuffer (context_id) ;
  fprintf(stderr, "DEBUG VALIDATE: index=%%s validation=0x%%08x expected=0x%%08x swapped=%%d\\n",
         index, widgetrec->validation, URMWidgetRecordValid, (*file_id_return)->byte_swapped);
  if ( UrmWRValid(widgetrec) )
    return MrmSUCCESS ;
  else
    {
      if ( (*file_id_return)->byte_swapped ) swapbytes(widgetrec->validation);
      fprintf(stderr, "DEBUG VALIDATE after swap: validation=0x%%08x\\n", widgetrec->validation);
      if ( UrmWRValid(widgetrec) )
\t{
\t  Urm__SwapRGMWidgetRecord(widgetrec);
\t  return MrmSUCCESS ;
\t}
    }

  return Urm__UT_Error("UrmHGetIndexedWidget", _MrmMMsg_0026,
\t\t       NULL, context_id, MrmBAD_WIDGET_REC) ;'''

if old in content:
    content = content.replace(old, new)
    with open(target, 'wb') as f:
        f.write(content)
    print("PATCHED OK")
else:
    # Try to find what's different
    print("NOT FOUND as binary. Checking line by line...")
    lines = content.decode('utf-8', errors='replace').split('\n')
    for i, line in enumerate(lines):
        if 'UrmWRValid(widgetrec)' in line and i < 140:
            print(f"Line {i+1}: {repr(line)}")
