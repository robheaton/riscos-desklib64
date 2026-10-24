# DeskLib64 - test coverage

Made by `tools/coverage.py`. A function is **called** if a test program calls it, and **reached** if a test calls it or a function that it calls does (so `Wimp_Poll`, which `Event_Poll` calls, counts). Reached means the test code can run it, not that its result was checked: `docs/STATUS.md` says what each test checks.

**479 of 619 library functions are reached (412 are called directly).**

| Module | Called | Reached | Total | Not reached |
|---|---:|---:|---:|---|
| Clear | 5 | 5 | 5 |  |
| Clipboard | 2 | 2 | 2 |  |
| ColourTrans | 6 | 6 | 6 |  |
| Coord | 9 | 9 | 9 |  |
| Debug | 0 | 0 | 17 | ClaimSignal, Initialise, InitialisePipetype, InitialiseReporter, InitialiseSignal, InitialiseUniqueFile, InitialiseUniquePipe, Print, PrintPipetype, PrintReporter, PrintStderr, PrintUniqueFile, PrintUniquePipe, Printf, ReleaseSignal, _Assert, _Dummyf |
| DeskLib | 0 | 0 | 2 | _snprintf, _vsnprintf |
| Dialog2 | 4 | 9 | 11 | CloseDialog, DeleteDialog |
| Dialog | 5 | 5 | 7 | Show, ShowAt |
| DragASprite | 3 | 3 | 3 |  |
| Drag | 1 | 1 | 2 | SetHandlers |
| Environment | 11 | 11 | 17 | LogInitialise, LogMessage, LogMessageWithName, SysBeep, TaskInfo, TaskNameFromHandle |
| Error | 4 | 8 | 8 |  |
| EventMsg | 2 | 5 | 6 | ReleaseMessage |
| Event | 5 | 8 | 9 | InitNested |
| File | 24 | 24 | 24 |  |
| Filing | 14 | 17 | 18 | ScanDirByDir |
| Filter | 0 | 0 | 4 | DeRegisterPostFilter, DeRegisterPreFilter, RegisterPostFilter, RegisterPreFilter |
| (no prefix) | 8 | 14 | 14 |  |
| Font2 | 4 | 4 | 4 |  |
| Font | 27 | 27 | 28 | SetScaleFactor |
| GFX | 1 | 1 | 11 | CLG, GetClip, ReadPoint, Rectangle, SetClip, SetColour, VDU, Wait, Write0, WriteN |
| Handler | 7 | 9 | 10 | Key |
| Hourglass | 5 | 5 | 5 |  |
| Icon | 37 | 46 | 47 | StartDrag |
| Kbd | 2 | 2 | 3 | GET |
| LinkList | 7 | 7 | 7 |  |
| Mem | 4 | 4 | 8 | CheckHeap, Compact, MoveAnchor, Size |
| Menu2 | 2 | 3 | 5 | CreateFromMsgs, DetachMenu |
| Menu | 22 | 25 | 25 |  |
| MessageTrans | 4 | 4 | 4 |  |
| Module | 2 | 2 | 2 |  |
| MsgTrans | 3 | 3 | 4 | ReportPS |
| Msgs | 4 | 5 | 8 | Report, ReportFatal, _Find |
| OS | 2 | 2 | 2 |  |
| PDriver | 0 | 0 | 17 | AbortJob, CancelJob, CancelJobWithError, CheckFeatures, CurrentJob, DrawPage, EndJob, EnumerateJobs, GetRectangle, GiveRectangle, Info, InsertIllustration, PageSize, PrinterName, ScreenDump, SelectIllustration, SelectJob |
| Pane2 | 16 | 16 | 16 |  |
| Pane | 8 | 8 | 8 |  |
| Pointer | 4 | 4 | 6 | RestrictToIcon, RestrictToWindow |
| Print | 0 | 0 | 1 | StartPrint |
| Resource | 6 | 6 | 6 |  |
| SWI | 0 | 0 | 2 | SWINumberFromString, SWINumberToString |
| Save | 1 | 1 | 3 | ReleaseSaveHandlers, SetFiletype |
| Screen | 2 | 2 | 2 |  |
| Serial | 0 | 0 | 15 | Activate, Deactivate, GetByte, GetFormat, GetRxRate, GetStatus, GetTxRate, NonZeroPollWordOnReceive, PutByte, SendBreak, SetFormat, SetRxRate, SetStatus, SetTxRate, _NZPWORRemove |
| Slider | 4 | 4 | 4 |  |
| Sprite | 63 | 64 | 76 | IconMemorySize, PlotGreyP, PlotMaskP, PlotMaskScaled, PlotMaskScaledP, PlotP, PlotScaledP, RedirectMask, RedirectMaskP, RedirectP, ScreenLoad, WimpPlot |
| Str | 5 | 5 | 5 |  |
| TaskWindow | 6 | 6 | 6 |  |
| Template | 7 | 10 | 11 | LinkSpriteArea |
| TextFile | 1 | 2 | 2 |  |
| Time | 6 | 6 | 6 |  |
| Tinct | 0 | 0 | 8 | AvailableFeatures, Compress, ConvertSprite, Decompress, Plot, PlotAlpha, PlotScaled, PlotScaledAlpha |
| Url | 0 | 0 | 4 | AddHandler, Encode, Launch, RemoveHandler |
| WimpMsg | 0 | 0 | 4 | Ack, GetTaskHandle, Reply, Send |
| Wimp | 31 | 50 | 59 | CommandWindow, Poll, PollIdle, ReportErrorR, SetMode, SetPalette, SetPointerShape, SpriteOp, TransferBlock |
| Window | 16 | 19 | 20 | MoveWindow |
|  | 0 | 0 | 1 | _wassert |
