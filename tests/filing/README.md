# filetest

A command line test of the `File` and `Filing` libraries: `File_Open`/`Close`/`Read*`/`Write*`/`Seek`/`EOF`, `File_Size`, `Exists`, `IsDirectory`, `SetType`/`GetType`, `Date`, `LoadTo`, `AllocLoad0`, `printf`, `File_Copy`, `File_Delete`, `File_CreateDir` and `File_GetLength` (macros over the `SWI ()` function), `Filing_ReadCatalogue`, the single and full directory entries, `Filing_OpenDir`/`ReadDir`/`CloseDir` in all three forms, the OS_GBPB veneers, the path functions and `Filing_ScanDir`.

```
*Run filetest [directory]
```

It makes a scratch directory `dltest` inside the directory given (default `<Wimp$ScrapDir>`), works in it and removes it. It prints a line per check and `SUMMARY 0 failed of 81`.
