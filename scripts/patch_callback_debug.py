import re

with open('ignored-area/third-party/motif/src/lib/Mrm/Mrmwcrw.c', 'r') as f:
    content = f.read()

old = '''  /*
   * Loop through all the items in the callback list
   */
  for ( ndx=0 ; ndx<cbdesc->count ; ndx++ )
    {
      itmptr = &cbdesc->item[ndx] ;

      /*
       * Set the routine pointer to the actual routine address. This
       * routine name must be a registered URM callback.
       */
      rtn_name = (String) bufptr + itmptr->cb_item.routine ;'''

new = '''  /*
   * Loop through all the items in the callback list
   */
  {
    unsigned char *raw = (unsigned char*)cbdesc;
    fprintf(stderr, "FIXUPCALLBACK: count=%d sizeof(RGMCallbackItem)=%zu cbdesc-bufptr=%td\\n",
           cbdesc->count, sizeof(RGMCallbackItem), (char*)cbdesc - (char*)bufptr);
    fprintf(stderr, "  raw cbdesc[0..31]:");
    for (int _k=0; _k<32; _k++) fprintf(stderr, " %02x", raw[_k]);
    fprintf(stderr, "\\n");
    if (cbdesc->count > 0) {
      unsigned char *item0 = (unsigned char*)&cbdesc->item[0];
      fprintf(stderr, "  raw item0[0..39]:");
      for (int _k=0; _k<40; _k++) fprintf(stderr, " %02x", item0[_k]);
      fprintf(stderr, "\\n  item[0].cb_item.routine=%u (0x%x)\\n",
             cbdesc->item[0].cb_item.routine, cbdesc->item[0].cb_item.routine);
    }
  }
  for ( ndx=0 ; ndx<cbdesc->count ; ndx++ )
    {
      itmptr = &cbdesc->item[ndx] ;

      /*
       * Set the routine pointer to the actual routine address. This
       * routine name must be a registered URM callback.
       */
      rtn_name = (String) bufptr + itmptr->cb_item.routine ;'''

if old in content:
    content = content.replace(old, new)
    with open('ignored-area/third-party/motif/src/lib/Mrm/Mrmwcrw.c', 'w') as f:
        f.write(content)
    print("REPLACED OK")
else:
    print("NOT FOUND - searching with regex...")
    # Find lines with key identifiers
    for i, line in enumerate(content.split('\n')):
        if 'Loop through all the items in the callback list' in line:
            print(f"Line {i+1}: {repr(line)}")
        if 'rtn_name = (String) bufptr + itmptr->cb_item.routine' in line:
            print(f"Line {i+1}: {repr(line)}")
