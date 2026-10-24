# msgtest

A command line test of MessageTrans (`MsgTrans_*`, `MessageTrans_*`) and the `Msgs` library.

On RISC OS 64, put `msgtest`, `Messages` and `Msgs` in one directory (with the file types suffix `,ff8` and `,fff` if the file system needs it), set `DLMsgs$Dir` to that directory and run it:

```
*Set DLMsgs$Dir <the directory>
*Run <DLMsgs$Dir>.msgtest
```

It prints a line for each check and `SUMMARY 0 failed of 32` at the end. `tests/build.sh` builds it into `dist/apps/`.
