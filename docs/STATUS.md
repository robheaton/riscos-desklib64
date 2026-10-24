# Status

## Built
All of DeskLib's C (about 270 files) and 259 veneers: 506 objects in `libDeskLib64.a`, no failures, no pointer-size warnings. Not built: `BackTrace`, `PCI`, `USB`.

## Run (the demonstration applications in `tests/`)
* **`hello`**: `Event_Initialise`, `EventMsg_Initialise`, `Wimp_CreateWindow`, `Wimp_CreateIcon` (an indirected text icon), `Event_Claim` handlers for click, close and open, `Window_Show`, `Event_Poll`, `Icon_Printf`, `Event_CloseDown`.
* **`demo2`**: `Resource_Initialise`, `Template_Initialise`, `Template_LoadFile` and `Window_Create` (a window from a template file), an icon made in code, `Menu_New`, `Menu_Show` and `Menu_SetFlags`, the `event_MENU` handler, `Icon_Printf`.

## Not run yet
Dialogue boxes (`Dialog`, `Dialog2`), `Save` and `Drag`/`DragASpr`, `Sprite` (OS_SpriteOp veneers), `Pane`, `Font` and the font menu, `Print` and `PDriver`, `Clear`, `Clipboard`, `Filing`/`File`, `Msgs`/`MsgTrans`, `Url`, `Tinct`, `Serial`, `ColourTran`. They compile, the veneers were translated or written from the assembler, and the structures have the OS layout, but nobody has run them. If something fails, please open an issue with the function and what happened.

## Known limits
* Arguments after the fourth to the translated veneers and structures passed by value are assumed to fit a 32-bit word (see docs/PORTING.md).
* The translator was checked by compiling and by the two demonstrations, not exhaustively.
