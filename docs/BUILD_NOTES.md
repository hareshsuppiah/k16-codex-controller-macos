# How the prototype was worked out

This project began with a working but unidentified macro pad: turning the knobs changed macOS volume, while Keyboard Setup Assistant reported that the device could not be identified.

## 1. Separate connection from keyboard identification

Karabiner-EventViewer showed that the pad was connected and emitting events. The Setup Assistant error only meant the device could not answer the conventional left/right Shift identification prompts.

## 2. Record events instead of trusting the printed layout

Physical position was initially misleading. A button assumed to be the intended Fn key actually produced a neighbouring event and triggered unexpected paste/history behaviour. Recording each control in EventViewer established the real map:

- ordinary keypad keys for several buttons;
- `F13`/`F17`/other function events for selected controls;
- consumer volume and mute events for knob rotation and presses.

## 3. Translate consumer events for Codex

Codex's shortcut recorder did not react to direct knob rotation because consumer volume events are not ordinary shortcut chords. Karabiner translates them into deliberately unusual keyboard chords, which are then assigned to Codex actions using the Mac keyboard.

## 4. Work around the missing vendor layout

The vendor configurator knew the USB device but did not contain a `36ae_2475` layout entry. It fell back to a full keyboard and sometimes omitted Lighting. A related `0816_2475` definition described the same 16-key/three-knob geometry. The optional localhost proxy maps the missing identity to that live definition without bundling the vendor application.

## 5. Target only the RGB HID interface

Opening every interface belonging to the keypad produced `kIOReturnExclusiveAccess` because Karabiner legitimately owned its keyboard interface. Matching usage page `0xff00`, usage `2` isolated the vendor-defined lighting interface.

## 6. Reproduce the working lighting sequence

Early RGB packets returned success while the physical pad continued its Spectrum wave. Observing a successful Static change in the vendor configurator revealed a required two-command sequence: select the Static effect, then send the full monochrome HSV configuration.

## 7. Respect macOS privacy identity

The helper requires Input Monitoring. Rebuilding an ad-hoc-signed app changes the identity macOS associates with that approval, so the final install location must be stable and the user may need to remove and re-add the app after rebuilding.

## 8. Add task-state lighting

The background helper follows the newest local Codex session JSONL and reduces it to five display states. This achieved the useful status-at-a-glance behaviour without flashing firmware or modifying Codex.

The result is a working prototype, not universal keypad support. The event map, HID identity, RGB protocol, and Codex session format are explicit compatibility boundaries.
