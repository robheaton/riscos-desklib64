# Status

## Built
All of DeskLib's C (about 270 files) and 259 veneers: 506 objects in `libDeskLib64.a`, no failures, no pointer-size warnings. Not built: `BackTrace`, `PCI`, `USB`.

## Run (the demonstration applications in `tests/`)
* **`hello`**: `Event_Initialise`, `EventMsg_Initialise`, `Wimp_CreateWindow`, `Wimp_CreateIcon` (an indirected text icon), `Event_Claim` handlers for click, close and open, `Window_Show`, `Event_Poll`, `Icon_Printf`, `Event_CloseDown`.
* **`demo2`**: `Resource_Initialise`, `Template_Initialise`, `Template_LoadFile` and `Window_Create` (a window from a template file), an icon made in code, `Menu_New`, `Menu_Show` and `Menu_SetFlags`, the `event_MENU` handler, `Icon_Printf`.

* **`dialogs`**: indirected writable, option and radio icons loaded from a template file; `Dialog2` as a static window, as a transient dialogue (`Dialog2_OpenDialogMenu`) and as a menu leaf (`Dialog2_OpenDialogMenuLeaf` from a `message_MENUWARN`); OK, Cancel, Return and Escape.
* **`save`**: the `Save` library: a save box with a draggable file icon (an indirected sprite), saving by pathname with the OK button, and dragging the file icon to a Filer window (the DataSave protocol and the result handler).

* **`sprite`**: `spritetest` (command line, 38 checks): `Sprite_InitArea`, `Create`, `ReadInfo`, `Select`, `WritePixel`, `ReadPixel`, `Copy`, `Rename`, `Delete`, `FlipX`, `FlipY`, `CreateMask`, `RemoveMask`, mask pixels, `ReadControl`, `Save` and `LoadFile`. `DLSprite` (a window): sprites made in code, `Sprite_Plot`, `PlotScaled`, `PlotMask`, `PlotGrey`, a sprite icon from a user sprite area (`indirectsprite` with `spritearea`), and `Sprite_Redirect`/`UnRedirect` with VDU output into a sprite. `Sprite_PlotGrey` is called, but what it draws depends on the operating system's SpriteExtend version, so it is not checked.

* **`pane`**: `Pane_CreateAndLink` with an offset and size given in code, `Pane_OpenEventHandler`, `Pane_GetSysHandle`, `Pane_Show`; `Pane2_CreateAndAddMain`, `Pane2_CreateAndAddPane` (offset and size from the template) and `Pane2_Show`. A click in a pane, the positions of both windows, a pane that follows its main window when it is dragged and a pane resized with its main window (`pane2_HRESIZE`).

## Known problems
* The older `Dialog` library (`Dialog_WaitForClick`) does not return in testing: the task stops receiving events. It has not been explained yet. `Dialog2` is not affected and is the one to use.

## Not run yet
`DragASpr`, `Font` and the font menu, `Print` and `PDriver`, `Clear`, `Clipboard`, `Filing`/`File`, `Msgs`/`MsgTrans`, `Url`, `Tinct`, `Serial`, `ColourTran`. They compile, the veneers were translated or written from the assembler, and the structures have the OS layout, but nobody has run them. If something fails, please open an issue with the function and what happened.

## Known limits
* Arguments after the fourth to the translated veneers and structures passed by value are assumed to fit a 32-bit word (see docs/PORTING.md).
* The translator was checked by compiling and by the two demonstrations, not exhaustively.
