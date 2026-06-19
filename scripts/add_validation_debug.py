"""Add debug printf to Mrmwread.c before the validation check."""
with open('ignored-area/third-party/motif/src/lib/Mrm/Mrmwread.c', 'r') as f:
    content = f.read()

# Find the unique pattern
old = """      if ( UrmWRValid(widgetrec) )
	return MrmSUCCESS ;
      else
	{
	  if ( (*file_id_return)->byte_swapped ) swapbytes(widgetrec->validation);
	  if ( UrmWRValid(widgetrec) )
	    {
	      Urm__SwapRGMWidgetRecord(widgetrec);
	      return MrmSUCCESS ;
	    }
	}

      return Urm__UT_Error("UrmHGetIndexedWidget", _MrmMMsg_0026,
		       NULL, context_id, MrmBAD_WIDGET_REC) ;"""

new = """      fprintf(stderr, "DEBUG VALIDATE: index=%s validation=0x%08x expected=0x%08x byte_swapped=%d\\n",
             index, widgetrec->validation, URMWidgetRecordValid, (*file_id_return)->byte_swapped);
      if ( UrmWRValid(widgetrec) )
	return MrmSUCCESS ;
      else
	{
	  if ( (*file_id_return)->byte_swapped ) swapbytes(widgetrec->validation);
	  fprintf(stderr, "DEBUG VALIDATE after swap: validation=0x%08x\\n", widgetrec->validation);
	  if ( UrmWRValid(widgetrec) )
	    {
	      Urm__SwapRGMWidgetRecord(widgetrec);
	      return MrmSUCCESS ;
	    }
	}

      return Urm__UT_Error("UrmHGetIndexedWidget", _MrmMMsg_0026,
		       NULL, context_id, MrmBAD_WIDGET_REC) ;"""

if old in content:
    content = content.replace(old, new)
    with open('ignored-area/third-party/motif/src/lib/Mrm/Mrmwread.c', 'w') as f:
        f.write(content)
    print("PATCHED: Mrmwread.c UrmHGetWidget validation debug added")
else:
    print("NOT FOUND - looking for pattern fragments...")
    for line in content.split('\n'):
        if 'UrmWRValid(widgetrec)' in line:
            print(repr(line))
