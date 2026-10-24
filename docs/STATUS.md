# Status

## Built
All of DeskLib's C (about 270 files) and 259 veneers: 506 objects in `libDeskLib64.a`, no failures, no pointer-size warnings. Not built: `BackTrace`, `PCI`, `USB`.

## Run (the demonstration applications in `tests/`)
* **`hello`**: `Event_Initialise`, `EventMsg_Initialise`, `Wimp_CreateWindow`, `Wimp_CreateIcon` (an indirected text icon), `Event_Claim` handlers for click, close and open, `Window_Show`, `Event_Poll`, `Icon_Printf`, `Event_CloseDown`.
* **`demo2`**: `Resource_Initialise`, `Template_Initialise`, `Template_LoadFile` and `Window_Create` (a window from a template file), an icon made in code, `Menu_New`, `Menu_Show` and `Menu_SetFlags`, the `event_MENU` handler, `Icon_Printf`.

* **`dialogs`**: indirected writable, option and radio icons loaded from a template file; `Dialog2` as a static window, as a transient dialogue (`Dialog2_OpenDialogMenu`) and as a menu leaf (`Dialog2_OpenDialogMenuLeaf` from a `message_MENUWARN`); OK, Cancel, Return and Escape; and the older `Dialog` library (`Dialog_Create`, `ShowStatic`, `WaitForClick`, `Hide`, `Destroy`) with the Old API button.
* **`save`**: the `Save` library: a save box with a draggable file icon (an indirected sprite), saving by pathname with the OK button, and dragging the file icon to a Filer window (the DataSave protocol and the result handler).

* **`sprite`**: `spritetest` (command line, 38 checks): `Sprite_InitArea`, `Create`, `ReadInfo`, `Select`, `WritePixel`, `ReadPixel`, `Copy`, `Rename`, `Delete`, `FlipX`, `FlipY`, `CreateMask`, `RemoveMask`, mask pixels, `ReadControl`, `Save` and `LoadFile`. `DLSprite` (a window): sprites made in code, `Sprite_Plot`, `PlotScaled`, `PlotMask`, `PlotGrey`, a sprite icon from a user sprite area (`indirectsprite` with `spritearea`), and `Sprite_Redirect`/`UnRedirect` with VDU output into a sprite. `Sprite_PlotGrey` is called, but what it draws depends on the operating system's SpriteExtend version, so it is not checked.

* **`pane`**: `Pane_CreateAndLink` with an offset and size given in code, `Pane_OpenEventHandler`, `Pane_GetSysHandle`, `Pane_Show`; `Pane2_CreateAndAddMain`, `Pane2_CreateAndAddPane` (offset and size from the template) and `Pane2_Show`. A click in a pane, the positions of both windows, a pane that follows its main window when it is dragged and a pane resized with its main window (`pane2_HRESIZE`).

* **`msgs`** (command line, 32 checks): `MsgTrans_LoadFile` (found through the resource path), `MsgTrans_Lookup` and `MsgTrans_LookupPS` with up to four `%n` parameters, defaults (`Token:default`), a missing token, a result cut at the buffer size, `MsgTrans_LoseFile`; the SWI veneers `MessageTrans_FileInfo`, `OpenFile` (with a buffer in the heap), `Lookup` (with and without a buffer) and `CloseFile`; the global messages; `Msgs_LoadFile`, `Msgs_Lookup` (with an include and a default), `Msgs_printf` and `Msgs_DropGroup`.

* **`filing`** (command line, 81 checks): all of `File` (open, read, write, seek, extent, size, type, date, load, copy, delete, directories) and `Filing` (catalogue, single entries, `OpenDir`/`ReadDir`/`CloseDir` in all three forms, the OS_GBPB veneers with a match, path functions, `ScanDir`), including the macros over `SWI ()` with pointer arguments. File system dependent values (the internal name `SIN`) are not checked.

* **`colour`** (a desktop task that logs to a file, 21 checks): `ColourTrans_SetGCOL` (the colours read back from a sprite that `GFX_RectangleFill` drew into after `Sprite_Redirect`), `ReturnColourNumberForMode` (current mode and a sprite mode), `SelectTable`, `InvalidateCache`, `ReturnFontColours` and `SetFontColours` for the current font.

* **`font`** (a desktop task that logs to a file, 36 checks and a window): `Font_CacheAddr`, `ListFonts` (all fonts), `GetWimpFont`, `FindFont`, `ReadDefn`, `ReadInfo`, `SetFont`, `CurrentFont`, `ReadScaleFactor`, `StringWidth` (with the `dl_ptr` in `font_string`), `StringBBox`, `ConvertToOS` and `ConvertToPoints`, `LoseFont`; `Font2` claim, reclaim and release; `Font_Paint` of text in four fonts in a redraw handler; the font menu (`Menu_FontMenu` with a tick, shown with `Menu_Show`; `Menu_FontMenuDecode` on a selection; the chosen font is painted).

* **`clear`** (command line, 20 checks): `Mem_Initialise`, `Mem_Alloc`, `Mem_MidExtend` and `Mem_Free` (the `Mem` replacement on `malloc`), and the `Clear` library: `Clear_Init`, `Clear_Create`, `Clear_Save`, `Clear_Load` and `Clear_Free` for 8 and 24 bits per pixel pictures, with the file written and read back and compared.

* **`clip`** (two tasks from one application): `Clipboard_Claim` and `Clipboard_Request`, received by the other task (the message sizes, the flags, the window and internal handles and the file types), and the same messages sent directly with `Wimp_SendMessage`. The data transfer that follows a request (`Clipboard`'s paste handler) is not implemented by the library and is not tested.
* **`drag`** (a window with a sprite icon): `DragASprite_DragIcon` (reads the window and icon state, starts the drag with the DragASprite module), `DragASprite_Start` with explicit arguments and `DragASprite_Stop`, and the `user drag ended` event.

* **`font3`** (a desktop task that logs to a file, 12 checks and a window): `Font_ScanString` with `font_scan_BBOX` (plain, doubled, mirrored and turned by a quarter, each checked against the plain box), the end pointer returned through `char **`, `Font_StringWidth`, and `Font_Paint3` with transformation matrices (the same string plain, at twice the size, rotated 30 degrees, mirrored and turned a quarter), all through the `SWI ()` function.

* **`env`** (command line, 51 checks): `Str` (`strlencr`, `strcmpcr`, `strcatcr`, `strcpycr`, `strncpycr`, `Str_LeafName`, `stricmp`, `strnicmp`, `stricmpcr`, `strnicmpcr`, `strdup`, `MakeCR`, `MakeASCIIZ`, `MakeHex`, `DecodeHex`); `Time` (`ConvertTimeToUTCOrdinals`, `ConvertDateAndTime`, `ConvertStandardDateAndTime`, names and ordinal suffixes, for a fixed time); `Environment` (`SysVarRead`, `SysVarSet` for strings and numbers, `SysVarDelete`, `ExpandString` and its end pointer, `Command`, `OS_CLI`, `ModuleIsActive`, `OSVersion`, `TaskList`, `TaskListFree`, `TaskIsActive`).

* **`misc`** (command line, 61 checks): `Coord` (point and rectangle tests, unions, window origin and conversions both ways), `LinkList` (all functions and macros), `TextFile` (`ReadToDelimiter`, `SkipBlanks`), `Screen` (`CacheModeInfo`, `ReadModeVariable`, the size, eigen factor and depth variables), `OS_Byte` and `OS_Word` (reads), `Module_Claim` and `Module_Free`, `Pointer` (get, set, restrict to a rectangle, unrestrict), `Kbd` (modifiers and `KeyDown` with nothing held), `Hourglass` (runs without a fault; the display is not checked). `Filter` and `Kbd_GET` (which waits for a key) are not run.

* **`menu2`** (a window that logs to a file): `Menu2_Create`, `Menu2_AttachMenu` (opened with the Menu button), ticked, shaded and dotted items, a static submenu and a dynamic one made by a function (a title and an item longer than twelve characters, so indirected), the flags function (called again by an Adjust click), the select functions with Select and Adjust, and the sub-menu function. The `Error` library: `Error_Report` with a formatted message, `Error_Check`, `Error_CheckSilent`; the error box is shown and each call returns after OK.

## Not run yet
`Print` and `PDriver`, `Url`, `Tinct`, `Serial`. They compile, the veneers were translated or written from the assembler, and the structures have the OS layout, but nobody has run them. If something fails, please open an issue with the function and what happened.

## Known limits
* Arguments after the fourth to the translated veneers and structures passed by value are assumed to fit a 32-bit word (see docs/PORTING.md).
* The translator was checked by compiling and by the two demonstrations, not exhaustively.
