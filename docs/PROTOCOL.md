# RGB protocol notes

These notes describe the tested unit only. They document observed behaviour without redistributing vendor source code.

## HID interface

- vendor ID: `0x36ae`;
- product ID: `0x2475`;
- primary usage page: `0xff00`;
- primary usage: `2`;
- output report ID: `0`;
- output report length: 64 bytes.

## Static colour sequence

The firmware requires two output reports.

### 1. Select Static effect

Relevant bytes:

```text
[0]  = 6
[1]  = 22
[5]  = 1   lighting type
[7]  = 1   Static mode
```

### 2. Configure Static monochrome colour

Relevant bytes:

```text
[0]  = 6
[1]  = 11
[2]  = 11
[5]  = 1   lighting type
[7]  = 1   Static mode
[8]  = 4   maximum brightness
[9]  = 3   vendor default speed; unused by Static
[10] = 1   direction; unused by Static
[11] = 1   monochrome enabled
[13] = H   hue, 0–255
[14] = S   saturation, 0–255
[15] = V   value, 0–255
```

All unspecified bytes are zero. The helper converts RGB status colours to the firmware's 8-bit HSV representation.

## Why the first implementation failed

Sending per-key colour payloads while the device remained in Spectrum mode produced successful host return codes but no persistent visual change. Selecting the related layout's assumed custom mode was also insufficient. Capturing the successful vendor UI sequence showed that a separate command `22` must precede the full command `11` configuration.

## Safety boundary

The project writes lighting reports only. It does not invoke firmware-update commands or overwrite keymap storage.
