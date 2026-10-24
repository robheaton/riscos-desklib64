# colourtest

A desktop task that runs once, appends its results to `<Wimp$ScrapDir>.colourtest` and quits (VDU output into a sprite does not behave as expected in a Terminal window, so it is not a command line program). It tests `ColourTrans_SetGCOL`, `ReturnColourNumberForMode`, `SelectTable`, `InvalidateCache`, `ReturnFontColours` and `SetFontColours`, and drawing into a 32 bits per pixel sprite with `Sprite_Redirect`, `GFX_RectangleFill` and `Sprite_ReadPixel` (a pixel reads as `0xBBGGRR`).

Double-click it (or `*Filer_Run` it), then read the log: it ends with `SUMMARY 0 failed of 21`.
