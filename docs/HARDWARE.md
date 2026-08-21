# Hardware identification

## Tested unit

The working prototype used a Temu listing described as a 16-key macro programming keypad with three rotary knobs:

- [clean product link](https://www.temu.com/goods.html?goods_id=601101322436966)
- device name: `k16_n3` / `G-LRA`
- vendor ID: `13998` (`0x36ae`)
- product ID: `9333` (`0x2475`)

The price observed during development was approximately AU$40, but marketplace pricing and listings change.

## Confirm your USB identity

1. Connect the keypad directly to the Mac.
2. Open Karabiner-EventViewer.
3. Select **Devices** and find the external keypad.
4. Confirm the vendor and product IDs.
5. Rotate and press each knob on the **Main** screen and record the events.

The tested unit emitted a mixture of ordinary keypad keys, function keys, and consumer media events such as `volume_increment`, `volume_decrement`, and `mute`.

## The macOS Keyboard Setup Assistant warning

macOS may ask you to press the key beside Shift and then report that the keyboard cannot be identified. This pad is not a conventional typing keyboard, so it may not emit the expected identification key.

If the buttons or knobs already produce events, quit or skip the assistant. Select **ANSI** if macOS or Karabiner asks for a virtual keyboard type. The warning does not mean the keypad is disconnected.

## Compatibility boundary

Do not assume compatibility from appearance alone. The native helper deliberately matches all four of these properties:

- vendor ID `0x36ae`;
- product ID `0x2475`;
- usage page `0xff00`;
- usage `2`.

This prevents it from taking exclusive control of the normal keyboard interfaces used by Karabiner. Supporting another unit requires confirming both its event map and RGB protocol.
