/*
 * widget-gallery — Motif Widget Gallery
 * Self-contained Motif widget demo using XmTabStack.
 * No MRM/UID dependency; all widgets created in C code.
 * Only includes widgets that Motif genuinely provides — no fakery.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/TabStack.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/List.h>
#include <Xm/Scale.h>
#include <Xm/ScrolledW.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/PanedW.h>
#include <Xm/MessageB.h>
#include <Xm/CascadeB.h>
#include <Xm/FileSB.h>
#include <Xm/DrawingA.h>

static Widget status_label = NULL;
static Widget tab_stack = NULL;
static Widget radio_value_label = NULL;

/* -- Utility helpers -- */

static Widget get_shell(Widget w) {
  while (w && !XtIsShell(w)) w = XtParent(w);
  return w;
}

/* -- Status bar helper -- */

static void status(const char *msg) {
  if (!status_label) return;
  XmString xms = XmStringCreateLocalized((char *)msg);
  XtVaSetValues(status_label, XmNlabelString, xms, NULL);
  XmStringFree(xms);
}

/* -- Callbacks -- */

static void push_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)w; (void)cb;
  status((const char *)cd);
}

static void toggle_cb(Widget w, XtPointer cd, XtPointer cb) {
  XmToggleButtonCallbackStruct *cs = (XmToggleButtonCallbackStruct *)cb;
  char buf[64];
  snprintf(buf, sizeof(buf), "%s: %s", (const char *)cd,
           cs->set ? "ON" : "OFF");
  status(buf);
}

static void radio_cb(Widget w, XtPointer cd, XtPointer cb) {
  XmToggleButtonCallbackStruct *cs = (XmToggleButtonCallbackStruct *)cb;
  if (!cs->set) return;
  char buf[64];
  snprintf(buf, sizeof(buf), "Radio selected: %s", (const char *)cd);
  status(buf);
  if (radio_value_label) {
    XmString xms = XmStringCreateLocalized((char *)cd);
    XtVaSetValues(radio_value_label, XmNlabelString, xms, NULL);
    XmStringFree(xms);
  }
}

static void list_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cb;
  XmListCallbackStruct *cs = (XmListCallbackStruct *)cd;
  char *choice = NULL;
  XmStringGetLtoR(cs->item, XmFONTLIST_DEFAULT_TAG, &choice);
  if (choice) {
    char buf[128];
    snprintf(buf, sizeof(buf), "List selected: %s", choice);
    status(buf);
    XtFree(choice);
  }
}

static void scale_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)w; (void)cb;
  XmScaleCallbackStruct *cs = (XmScaleCallbackStruct *)cd;
  char buf[64];
  snprintf(buf, sizeof(buf), "Scale value: %d", cs->value);
  status(buf);
}

static void text_activate_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cb;
  char *text = XmTextGetString(w);
  char buf[128];
  snprintf(buf, sizeof(buf), "%s: \"%s\"", (const char *)cd, text);
  status(buf);
  XtFree(text);
}

static void exit_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)w; (void)cd; (void)cb;
  exit(0);
}

static void about_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)w; (void)cd; (void)cb;
  Widget shell = get_shell(w);
  Widget dialog = XmCreateInformationDialog(shell, "about", NULL, 0);
  XmString msg = XmStringCreateLocalized(
      "Motif Widget Gallery\n\n"
      "A showcase of genuine Motif widgets running on em-x11 (Emscripten/WASM).\n"
      "Motif 2.4");
  XtVaSetValues(dialog, XmNmessageString, msg, NULL);
  XmStringFree(msg);
  XtUnmanageChild(XtNameToWidget(dialog, "XmPushButton2")); /* Cancel */
  XtUnmanageChild(XtNameToWidget(dialog, "XmPushButton3")); /* Help */
  XtManageChild(dialog);
}

static void view_tab_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)w; (void)cb;
  int page = (int)(intptr_t)cd;
  Widget child = XmTabStackIndexToWidget(tab_stack, page);
  if (child) XmTabStackSelectTab(child, True);
}

/* -- Menu bar helpers -- */

static Widget make_menu(Widget menu_bar, const char *label, char mnemonic) {
  XmString xms = XmStringCreateLocalized((char *)label);
  Widget cascade = XtVaCreateManagedWidget(label, xmCascadeButtonWidgetClass,
                                           menu_bar,
                                           XmNlabelString, xms,
                                           XmNmnemonic, (KeySym)mnemonic,
                                           NULL);
  XmStringFree(xms);
  Widget menu = XmCreatePulldownMenu(menu_bar, "pulldown", NULL, 0);
  XtVaSetValues(cascade, XmNsubMenuId, menu, NULL);
  return menu;
}

static Widget add_menu_item(Widget menu, const char *label, char mnemonic,
                            XtCallbackProc cb, XtPointer cd) {
  XmString xms = XmStringCreateLocalized((char *)label);
  Widget btn = XtVaCreateManagedWidget(label, xmPushButtonWidgetClass, menu,
                                       XmNlabelString, xms,
                                       XmNmnemonic, (KeySym)mnemonic,
                                       NULL);
  XmStringFree(xms);
  XtAddCallback(btn, XmNactivateCallback, cb, cd);
  return btn;
}

/* -- Section header: bold label -- */

static Widget section_label(Widget parent, const char *text) {
  XmFontList bold_fl = NULL;
  Widget label;
  XmString xms = XmStringCreateLocalized((char *)text);
  XmFontListEntry entry = XmFontListEntryLoad(XtDisplayOfObject(parent),
      "-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
      XmFONT_IS_FONT, "bold_tag");
  if (entry) {
    bold_fl = XmFontListAppendEntry(NULL, entry);
    XmFontListEntryFree(&entry);
  }
  label = XtVaCreateManagedWidget(text, xmLabelWidgetClass, parent,
                                   XmNlabelString, xms,
                                   XmNfontList, bold_fl,
                                   XmNalignment, XmALIGNMENT_BEGINNING,
                                   NULL);
  if (bold_fl) XmFontListFree(bold_fl);
  XmStringFree(xms);
  return label;
}

/* -- Helper: create a page with tab label constraint -- */

static Widget create_page(Widget ts, const char *name, const char *tab_label) {
  XmString xms = XmStringCreateLocalized((char *)tab_label);
  Widget page = XtVaCreateManagedWidget(name, xmRowColumnWidgetClass, ts,
                                        XmNtabLabelString, xms,
                                        XmNorientation, XmVERTICAL,
                                        XmNpacking, XmPACK_TIGHT,
                                        XmNentryAlignment, XmALIGNMENT_BEGINNING,
                                        XmNspacing, 6,
                                        XmNmarginWidth, 14,
                                        XmNmarginHeight, 10,
                                        NULL);
  XmStringFree(xms);
  return page;
}

/* ====================================================================
 * Tab 1 — Buttons
 * ==================================================================== */

/* -- Menubutton popup callback for Auto-save check -- */
static void mb_autosave_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cb;
  XmToggleButtonCallbackStruct *cs = (XmToggleButtonCallbackStruct *)cd;
  char buf[64];
  snprintf(buf, sizeof(buf), "Auto-save: %s", cs->set ? "ON" : "OFF");
  status(buf);
}

static void create_buttons_tab(Widget ts) {
  Widget page = create_page(ts, "buttonsPage", "Buttons");

  /* -- Button -- */
  section_label(page, "Button");

  {
    Widget row = XtVaCreateManagedWidget("pbrow", xmRowColumnWidgetClass, page,
                                         XmNorientation, XmHORIZONTAL,
                                         XmNpacking, XmPACK_TIGHT,
                                         XmNspacing, 3,
                                         NULL);
    Widget w = XtVaCreateManagedWidget("Normal", xmPushButtonWidgetClass, row, NULL);
    XtAddCallback(w, XmNactivateCallback, push_cb, "Button: Normal");

    w = XtVaCreateManagedWidget("Disabled", xmPushButtonWidgetClass, row, NULL);
    XtVaSetValues(w, XmNsensitive, False, NULL);

    w = XtVaCreateManagedWidget("Close", xmPushButtonWidgetClass, row, NULL);
    XtAddCallback(w, XmNactivateCallback, exit_cb, NULL);
  }

  /* -- Checkbutton -- */
  section_label(page, "Checkbutton");

  {
    Widget w = XtVaCreateManagedWidget("Checked by default",
                                       xmToggleButtonWidgetClass, page,
                                       XmNset, True,
                                       XmNalignment, XmALIGNMENT_BEGINNING,
                                       NULL);
    XtAddCallback(w, XmNvalueChangedCallback, toggle_cb, "Checked by default");

    w = XtVaCreateManagedWidget("Unchecked",
                                xmToggleButtonWidgetClass, page,
                                XmNalignment, XmALIGNMENT_BEGINNING,
                                NULL);
    XtAddCallback(w, XmNvalueChangedCallback, toggle_cb, "Unchecked");

    w = XtVaCreateManagedWidget("Disabled",
                                xmToggleButtonWidgetClass, page,
                                XmNalignment, XmALIGNMENT_BEGINNING,
                                XmNsensitive, False,
                                NULL);
  }

  /* -- Radiobutton -- */
  section_label(page, "Radiobutton");

  {
    Widget row = XtVaCreateManagedWidget("rrow", xmRowColumnWidgetClass, page,
                                         XmNorientation, XmHORIZONTAL,
                                         XmNpacking, XmPACK_TIGHT,
                                         XmNspacing, 6,
                                         NULL);
    Widget w = XtVaCreateManagedWidget("Tcl", xmToggleButtonWidgetClass, row,
                                       XmNindicatorType, XmONE_OF_MANY,
                                       XmNset, True,
                                       NULL);
    XtAddCallback(w, XmNvalueChangedCallback, radio_cb, "Tcl");

    w = XtVaCreateManagedWidget("Python", xmToggleButtonWidgetClass, row,
                                XmNindicatorType, XmONE_OF_MANY,
                                NULL);
    XtAddCallback(w, XmNvalueChangedCallback, radio_cb, "Python");

    w = XtVaCreateManagedWidget("Rust", xmToggleButtonWidgetClass, row,
                                XmNindicatorType, XmONE_OF_MANY,
                                NULL);
    XtAddCallback(w, XmNvalueChangedCallback, radio_cb, "Rust");

    /* Value display label, like Tk's sunken label showing ::lang */
    {
      XmString xms = XmStringCreateLocalized("Tcl");
      radio_value_label = XtVaCreateManagedWidget("radioValue",
                                                   xmLabelWidgetClass, page,
                                                   XmNlabelString, xms,
                                                   XmNalignment, XmALIGNMENT_CENTER,
                                                   XmNwidth, 100,
                                                   XmNshadowThickness, 1,
                                                   NULL);
      XmStringFree(xms);
    }
  }

  /* -- Menubutton (CascadeButton + PulldownMenu) -- */
  section_label(page, "Menubutton");

  {
    XmString xms = XmStringCreateLocalized("File");
    Widget mb = XtVaCreateManagedWidget("menubutton",
                                         xmCascadeButtonWidgetClass, page,
                                         XmNlabelString, xms,
                                         NULL);
    XmStringFree(xms);

    Widget menu = XmCreatePulldownMenu(mb, "mbMenu", NULL, 0);
    XtVaSetValues(mb, XmNsubMenuId, menu, NULL);

    /* Build menu items: New, Open, separator, Auto-save check, separator, Quit */
    XmString item_xms;
    Widget item;

    item_xms = XmStringCreateLocalized("New");
    item = XtVaCreateManagedWidget("mbNew", xmPushButtonWidgetClass, menu,
                                    XmNlabelString, item_xms, NULL);
    XmStringFree(item_xms);
    XtAddCallback(item, XmNactivateCallback, push_cb, "File > New");

    item_xms = XmStringCreateLocalized("Open");
    item = XtVaCreateManagedWidget("mbOpen", xmPushButtonWidgetClass, menu,
                                    XmNlabelString, item_xms, NULL);
    XmStringFree(item_xms);
    XtAddCallback(item, XmNactivateCallback, push_cb, "File > Open");

    XtVaCreateManagedWidget("mbSep1", xmSeparatorWidgetClass, menu, NULL);

    item_xms = XmStringCreateLocalized("Auto-save");
    item = XtVaCreateManagedWidget("mbAutoSave", xmToggleButtonWidgetClass, menu,
                                    XmNlabelString, item_xms,
                                    NULL);
    XmStringFree(item_xms);
    XtAddCallback(item, XmNvalueChangedCallback, mb_autosave_cb, NULL);

    XtVaCreateManagedWidget("mbSep2", xmSeparatorWidgetClass, menu, NULL);

    item_xms = XmStringCreateLocalized("Quit");
    item = XtVaCreateManagedWidget("mbQuit", xmPushButtonWidgetClass, menu,
                                    XmNlabelString, item_xms, NULL);
    XmStringFree(item_xms);
    XtAddCallback(item, XmNactivateCallback, exit_cb, NULL);
  }
}

/* ====================================================================
 * Tab 2 — Text & Entry
 * ==================================================================== */

static void create_text_entry_tab(Widget ts) {
  Widget page = create_page(ts, "textPage", "Text & Entry");

  /* -- Label -- */
  section_label(page, "Label");

  XtVaCreateManagedWidget("labelDemo", xmLabelWidgetClass, page,
                          XmNlabelString,
                          XmStringCreateLocalized(
                              "XmLabel: a read-only text display widget"),
                          NULL);

  /* -- TextField -- */
  section_label(page, "TextField");

  {
    Widget er = XtVaCreateManagedWidget("er", xmRowColumnWidgetClass, page,
                                        XmNorientation, XmHORIZONTAL,
                                        XmNpacking, XmPACK_TIGHT,
                                        XmNspacing, 3,
                                        NULL);
    XtVaCreateManagedWidget("Input:", xmLabelWidgetClass, er,
                            XmNalignment, XmALIGNMENT_BEGINNING,
                            NULL);
    Widget e = XtVaCreateManagedWidget("textField", xmTextFieldWidgetClass, er,
                                       XmNcolumns, 30,
                                       NULL);
    XmTextFieldSetString(e, "Type your text here...");
    XtAddCallback(e, XmNactivateCallback, text_activate_cb, "TextField");
  }

  /* -- Text (multi-line) -- */
  section_label(page, "Text (multi-line)");

  {
    Widget txt = XtVaCreateManagedWidget("text", xmTextWidgetClass, page,
                                         XmNeditMode, XmMULTI_LINE_EDIT,
                                         XmNrows, 8,
                                         XmNcolumns, 52,
                                         XmNwordWrap, True,
                                         NULL);
    XmTextSetString(txt,
        "Multi-line XmText widget.\n\n"
        "You can select, copy, and type here.\n"
        "- Bullet one\n"
        "- Bullet two\n"
        "- Bullet three\n\n"
        "This widget supports text editing with keyboard and mouse.");
  }
}

/* ====================================================================
 * Tab 3 — Selection
 * ==================================================================== */

static void create_selection_tab(Widget ts) {
  Widget page = create_page(ts, "selectPage", "Selection");

  /* -- List -- */
  section_label(page, "List");

  {
    XmString items[8];
    const char *names[] = {
      "Apple", "Banana", "Cherry", "Date",
      "Elderberry", "Fig", "Grape", "Kiwi"
    };
    int i;
    for (i = 0; i < 8; i++)
      items[i] = XmStringCreateLocalized((char *)names[i]);
    Widget lb = XtVaCreateManagedWidget("list", xmListWidgetClass, page,
                                        XmNitems, items,
                                        XmNitemCount, 8,
                                        XmNvisibleItemCount, 6,
                                        NULL);
    XtAddCallback(lb, XmNsingleSelectionCallback, list_cb, NULL);
    for (i = 0; i < 8; i++) XmStringFree(items[i]);
    XmListSelectPos(lb, 0, False);
  }

  /* -- Scale -- */
  section_label(page, "Scale");

  {
    Widget s = XtVaCreateManagedWidget("scale", xmScaleWidgetClass, page,
                                       XmNorientation, XmHORIZONTAL,
                                       XmNminimum, 0,
                                       XmNmaximum, 100,
                                       XmNvalue, 50,
                                       XmNshowValue, True,
                                       XmNwidth, 260,
                                       NULL);
    XtAddCallback(s, XmNvalueChangedCallback, scale_cb, NULL);
    XtAddCallback(s, XmNdragCallback, scale_cb, NULL);
  }
}

/* ====================================================================
 * Tab 4 — Containers
 * ==================================================================== */

static void create_containers_tab(Widget ts) {
  Widget page = create_page(ts, "containersPage", "Containers");

  /* -- Frame -- */
  section_label(page, "Frame");

  {
    Widget frame = XtVaCreateManagedWidget("Preferences", xmFrameWidgetClass, page,
                                           NULL);
    Widget rc = XtVaCreateManagedWidget("prefRC", xmRowColumnWidgetClass, frame,
                                        XmNorientation, XmVERTICAL,
                                        XmNpacking, XmPACK_TIGHT,
                                        XmNmarginWidth, 10,
                                        XmNmarginHeight, 6,
                                        NULL);
    Widget w;
    w = XtVaCreateManagedWidget("Enable feature X",
                                xmToggleButtonWidgetClass, rc,
                                XmNset, True,
                                NULL);
    XtAddCallback(w, XmNvalueChangedCallback, toggle_cb, "Feature X");
    w = XtVaCreateManagedWidget("Enable feature Y",
                                xmToggleButtonWidgetClass, rc,
                                NULL);
    XtAddCallback(w, XmNvalueChangedCallback, toggle_cb, "Feature Y");
  }

  /* -- PanedWindow -- */
  section_label(page, "PanedWindow");

  {
    Widget pw = XtVaCreateManagedWidget("pw", xmPanedWindowWidgetClass, page,
                                        XmNorientation, XmVERTICAL,
                                        XmNheight, 150,
                                        XmNspacing, 3,
                                        NULL);
    Widget top = XtVaCreateManagedWidget("topPane", xmFrameWidgetClass, pw,
                                         NULL);
    XtVaCreateManagedWidget("topL", xmLabelWidgetClass, top,
                            XmNlabelString,
                            XmStringCreateLocalized(
                                "Top pane — drag the sash below to resize"),
                            XmNmarginHeight, 12,
                            NULL);
    Widget bot = XtVaCreateManagedWidget("botPane", xmFrameWidgetClass, pw,
                                         NULL);
    XtVaCreateManagedWidget("botL", xmLabelWidgetClass, bot,
                            XmNlabelString,
                            XmStringCreateLocalized(
                                "Bottom pane — also resizable"),
                            XmNmarginHeight, 12,
                            NULL);
  }
}

/* ====================================================================
 * Tab 5 — Canvas (DrawingArea)
 * ==================================================================== */

static void da_expose_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cd; (void)cb;
  Display *dpy = XtDisplayOfObject(w);
  Window win = XtWindowOfObject(w);
  GC gc = XCreateGC(dpy, win, 0, NULL);

  /* White background */
  XSetForeground(dpy, gc, 0x00FFFFFF);
  XFillRectangle(dpy, win, gc, 0, 0, 420, 260);

  /* 1. Rectangle: fill=#4a90d9, outline=#1a3a5c, width=2 */
  XSetForeground(dpy, gc, 0x004a90d9);
  XFillRectangle(dpy, win, gc, 20, 20, 120, 60);
  XSetForeground(dpy, gc, 0x001a3a5c);
  XSetLineAttributes(dpy, gc, 2, LineSolid, CapButt, JoinMiter);
  XDrawRectangle(dpy, win, gc, 20, 20, 120, 60);
  XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
  XSetForeground(dpy, gc, 0x00FFFFFF);
  XDrawString(dpy, win, gc, 47, 53, "Rectangle", 9);

  /* 2. Oval: fill=#e85d75, outline=#8b1a2b, width=2 */
  XSetForeground(dpy, gc, 0x00e85d75);
  XFillArc(dpy, win, gc, 180, 20, 120, 60, 0, 360 * 64);
  XSetForeground(dpy, gc, 0x008b1a2b);
  XSetLineAttributes(dpy, gc, 2, LineSolid, CapButt, JoinMiter);
  XDrawArc(dpy, win, gc, 180, 20, 120, 60, 0, 360 * 64);
  XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
  XSetForeground(dpy, gc, 0x00FFFFFF);
  XDrawString(dpy, win, gc, 226, 53, "Oval", 4);

  /* 3. Axis-like lines: #333, width=4 */
  XSetForeground(dpy, gc, 0x00333333);
  XSetLineAttributes(dpy, gc, 4, LineSolid, CapButt, JoinMiter);
  XDrawLine(dpy, win, gc, 20, 120, 320, 120);
  XDrawLine(dpy, win, gc, 20, 118, 20, 180);
  XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
  XSetForeground(dpy, gc, 0x00555555);
  XDrawString(dpy, win, gc, 40, 140, "Lines", 5);

  /* 4. Polygon (pentagon): fill=#50c878, outline=#1a5c30, width=2 */
  {
    XPoint poly[5] = {
      {230, 200}, {290, 150}, {350, 180}, {330, 230}, {250, 230}
    };
    XSetForeground(dpy, gc, 0x0050c878);
    XFillPolygon(dpy, win, gc, poly, 5, Complex, CoordModeOrigin);
    XSetForeground(dpy, gc, 0x001a5c30);
    XSetLineAttributes(dpy, gc, 2, LineSolid, CapButt, JoinMiter);
    XDrawLines(dpy, win, gc, poly, 5, CoordModeOrigin);
    XDrawLine(dpy, win, gc, 250, 230, 230, 200);
    XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
    XSetForeground(dpy, gc, 0x00FFFFFF);
    XDrawString(dpy, win, gc, 268, 198, "Polygon", 7);
  }

  /* 5. Arc: #9370db, width=3, start=30, extent=270 */
  XSetForeground(dpy, gc, 0x009370db);
  XSetLineAttributes(dpy, gc, 3, LineSolid, CapButt, JoinMiter);
  XDrawArc(dpy, win, gc, 30, 160, 100, 80, 30 * 64, 270 * 64);
  XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
  XDrawString(dpy, win, gc, 70, 210, "Arc", 3);

  /* 6. Canvas text: #e67e22 */
  XSetForeground(dpy, gc, 0x00e67e22);
  XDrawString(dpy, win, gc, 380, 35, "Canvas", 6);
  XDrawString(dpy, win, gc, 383, 50, "Text", 4);

  XFreeGC(dpy, gc);
}

static void da_resize_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cd; (void)cb;
  XClearWindow(XtDisplayOfObject(w), XtWindowOfObject(w));
  da_expose_cb(w, cd, cb);
}

static void create_canvas_tab(Widget ts) {
  Widget page = create_page(ts, "canvasPage", "Canvas");

  section_label(page, "DrawingArea: Xlib drawing primitives");

  Widget da = XtVaCreateManagedWidget("drawArea", xmDrawingAreaWidgetClass, page,
                                      XmNwidth, 420,
                                      XmNheight, 260,
                                      XmNshadowThickness, 1,
                                      XmNshadowType, XmSHADOW_IN,
                                      NULL);
  XtAddCallback(da, XmNexposeCallback, da_expose_cb, NULL);
  XtAddCallback(da, XmNresizeCallback, da_resize_cb, NULL);
}

/* ====================================================================
 * Tab 6 — Common Dialogs
 * ==================================================================== */

static void info_dialog_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cd; (void)cb;
  Widget shell = get_shell(w);
  Widget d = XmCreateInformationDialog(shell, "infoDialog", NULL, 0);
  XmString msg = XmStringCreateLocalized(
      "This is an info message box.\nPress OK to continue.");
  XtVaSetValues(d, XmNmessageString, msg, NULL);
  XmStringFree(msg);
  XtUnmanageChild(XtNameToWidget(d, "XmPushButton2"));
  XtUnmanageChild(XtNameToWidget(d, "XmPushButton3"));
  XtManageChild(d);
}

static void error_dialog_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cd; (void)cb;
  Widget shell = get_shell(w);
  Widget d = XmCreateErrorDialog(shell, "errorDialog", NULL, 0);
  XmString msg = XmStringCreateLocalized(
      "Something went wrong!\nPlease try again.");
  XtVaSetValues(d, XmNmessageString, msg, NULL);
  XmStringFree(msg);
  XtUnmanageChild(XtNameToWidget(d, "XmPushButton2"));
  XtUnmanageChild(XtNameToWidget(d, "XmPushButton3"));
  XtManageChild(d);
}

static void question_dialog_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cd; (void)cb;
  Widget shell = get_shell(w);
  Widget d = XmCreateQuestionDialog(shell, "questionDialog", NULL, 0);
  XmString msg = XmStringCreateLocalized(
      "Do you want to save changes before closing?");
  XtVaSetValues(d, XmNmessageString, msg, NULL);
  XmStringFree(msg);
  XtUnmanageChild(XtNameToWidget(d, "XmPushButton3"));
  XtManageChild(d);
}

static void warning_dialog_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cd; (void)cb;
  Widget shell = get_shell(w);
  Widget d = XmCreateWarningDialog(shell, "warnDialog", NULL, 0);
  XmString msg = XmStringCreateLocalized(
      "This action cannot be undone.\nContinue?");
  XtVaSetValues(d, XmNmessageString, msg, NULL);
  XmStringFree(msg);
  XtUnmanageChild(XtNameToWidget(d, "XmPushButton3"));
  XtManageChild(d);
}

/* -- File dialog callbacks -- */

static void file_ok_cb(Widget w, XtPointer cd, XtPointer cb) {
  XmFileSelectionBoxCallbackStruct *cbs = (XmFileSelectionBoxCallbackStruct *)cb;
  Widget fs = (Widget)cd;
  char *filename = NULL;
  if (cbs && cbs->value) {
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename);
  }
  if (filename) {
    status(filename);
    XtFree(filename);
  }
  XtUnmanageChild(fs);
}

static void file_cancel_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)w; (void)cb;
  Widget fs = (Widget)cd;
  XtUnmanageChild(fs);
}

static void open_file_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cd; (void)cb;
  Widget shell = get_shell(w);
  Widget d = XmCreateFileSelectionDialog(shell, "openFile", NULL, 0);
  XtAddCallback(d, XmNokCallback, file_ok_cb, (XtPointer)d);
  XtAddCallback(d, XmNcancelCallback, file_cancel_cb, (XtPointer)d);
  XtManageChild(d);
  status("Open file dialog...");
}

static void save_file_cb(Widget w, XtPointer cd, XtPointer cb) {
  (void)cd; (void)cb;
  Widget shell = get_shell(w);
  Widget d = XmCreateFileSelectionDialog(shell, "saveFile", NULL, 0);
  XtAddCallback(d, XmNokCallback, file_ok_cb, (XtPointer)d);
  XtAddCallback(d, XmNcancelCallback, file_cancel_cb, (XtPointer)d);
  XtManageChild(d);
  status("Save file dialog...");
}

static void create_dialogs_tab(Widget ts) {
  Widget page = create_page(ts, "dialogsPage", "Common Dialogs");

  /* -- Message Box -- */
  section_label(page, "MessageBox");

  {
    Widget row = XtVaCreateManagedWidget("mbrow", xmRowColumnWidgetClass, page,
                                         XmNorientation, XmHORIZONTAL,
                                         XmNpacking, XmPACK_TIGHT,
                                         XmNspacing, 3,
                                         NULL);
    Widget w;
    w = XtVaCreateManagedWidget("Info", xmPushButtonWidgetClass, row, NULL);
    XtAddCallback(w, XmNactivateCallback, info_dialog_cb, NULL);
    w = XtVaCreateManagedWidget("Error", xmPushButtonWidgetClass, row, NULL);
    XtAddCallback(w, XmNactivateCallback, error_dialog_cb, NULL);
    w = XtVaCreateManagedWidget("Question", xmPushButtonWidgetClass, row, NULL);
    XtAddCallback(w, XmNactivateCallback, question_dialog_cb, NULL);
    w = XtVaCreateManagedWidget("Warning", xmPushButtonWidgetClass, row, NULL);
    XtAddCallback(w, XmNactivateCallback, warning_dialog_cb, NULL);
  }

  /* -- File Selection Dialog -- */
  section_label(page, "FileSelectionDialog");

  {
    Widget row = XtVaCreateManagedWidget("fdrow", xmRowColumnWidgetClass, page,
                                         XmNorientation, XmHORIZONTAL,
                                         XmNpacking, XmPACK_TIGHT,
                                         XmNspacing, 3,
                                         NULL);
    Widget w;
    w = XtVaCreateManagedWidget("Open File...", xmPushButtonWidgetClass, row,
                                NULL);
    XtAddCallback(w, XmNactivateCallback, open_file_cb, NULL);
    w = XtVaCreateManagedWidget("Save File...", xmPushButtonWidgetClass, row,
                                NULL);
    XtAddCallback(w, XmNactivateCallback, save_file_cb, NULL);
  }
}

/* ====================================================================
 * Main
 * ==================================================================== */

int main(int argc, char *argv[]) {
  Widget shell, main_win, menu_bar;
  Widget file_menu, view_menu, help_menu;

  setenv("LANG", "C.UTF-8", 1);
  XtSetLanguageProc(NULL, (XtLanguageProc)NULL, NULL);

  /* --- Shell --- */
  shell = XtVaAppInitialize(
      NULL, "WidgetGallery", NULL, 0, &argc, argv, NULL,
      XmNwidth, 740, XmNheight, 560, NULL);

  /* --- MainWindow --- */
  main_win = XtVaCreateManagedWidget("mainWin", xmMainWindowWidgetClass, shell,
                                     NULL);

  /* --- Menu bar --- */
  menu_bar = XmCreateMenuBar(main_win, "menuBar", NULL, 0);
  XtManageChild(menu_bar);

  file_menu = make_menu(menu_bar, "File", 'F');
  add_menu_item(file_menu, "New", 'N', push_cb, "File > New");
  add_menu_item(file_menu, "Open...", 'O', push_cb, "File > Open...");
  XtVaCreateManagedWidget("fileSep", xmSeparatorWidgetClass, file_menu, NULL);
  add_menu_item(file_menu, "Quit", 'Q', exit_cb, NULL);

  view_menu = make_menu(menu_bar, "View", 'V');
  {
    const char *tabs[] = {"Buttons", "Text & Entry", "Selection",
                          "Containers", "Canvas", "Common Dialogs"};
    int i;
    for (i = 0; i < 6; i++)
      add_menu_item(view_menu, tabs[i], tabs[i][0],
                    view_tab_cb, (XtPointer)(intptr_t)i);
  }

  help_menu = make_menu(menu_bar, "Help", 'H');
  add_menu_item(help_menu, "About...", 'A', about_cb, NULL);

  XtVaSetValues(main_win, XmNmenuBar, menu_bar, NULL);

  /* --- TabStack --- */
  tab_stack = XmCreateTabStack(main_win, "tabStack", NULL, 0);

  create_buttons_tab(tab_stack);
  create_text_entry_tab(tab_stack);
  create_selection_tab(tab_stack);
  create_containers_tab(tab_stack);
  create_canvas_tab(tab_stack);
  create_dialogs_tab(tab_stack);

  XtManageChild(tab_stack);
  XtVaSetValues(main_win, XmNworkWindow, tab_stack, NULL);

  /* --- Status bar --- */
  {
    Widget bar = XtVaCreateManagedWidget("statusBar",
                                         xmRowColumnWidgetClass, main_win,
                                         XmNorientation, XmHORIZONTAL,
                                         XmNpacking, XmPACK_TIGHT,
                                         XmNshadowThickness, 1,
                                         NULL);
    XmString xms = XmStringCreateLocalized(
        " Motif 2.4  |  em-x11 wasm");
    status_label = XtVaCreateManagedWidget("status", xmLabelWidgetClass, bar,
                                           XmNlabelString, xms,
                                           XmNalignment, XmALIGNMENT_BEGINNING,
                                           XmNmarginHeight, 2,
                                           NULL);
    XmStringFree(xms);
    XtVaSetValues(main_win, XmNmessageWindow, bar, NULL);
  }

  /* --- Realize and loop --- */
  XtRealizeWidget(shell);
  XtAppMainLoop(XtWidgetToApplicationContext(shell));
  return 0;
}
