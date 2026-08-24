# Troubleshooting

## Keyboard Setup Assistant cannot identify the keypad

Quit or skip the assistant if Karabiner-EventViewer already sees button or knob events. Choose ANSI when a virtual keyboard type is required.

## The vendor page shows a full keyboard

The tested `36ae_2475` identity is missing from the vendor site's layout bundle. Use the optional proxy described in [Setup](SETUP.md#8-optional-vendor-configurator-workaround).

## The Lighting menu is missing

Wait a few seconds after connecting; device state loads in stages. If it remains missing, verify that the proxy maps both the JavaScript layout reference and the requested JSON path.

## Codex does not detect knob rotation while recording a shortcut

The knobs may emit consumer events such as volume up/down. Codex's recorder may ignore those. Record an ordinary shortcut on the Mac keyboard, then let Karabiner translate the knob event to that shortcut.

## Fn behaves like Paste, Home, or another key

Confirm the actual event in Karabiner-EventViewer. On the tested pad the intended bottom-left button emitted `F17`; mapping a neighbouring physical position by assumption caused the earlier error. Enable only one `F17 → Fn/Globe` rule for this device.

## Lighting stays on a rainbow wave

The keypad accepted RGB writes but ignored them while an animated effect remained active. The working sequence is:

1. select firmware Static mode with command `22`;
2. send the complete Static/monochrome HSV configuration with command `11`.

The helper implements that sequence. See [Protocol](PROTOCOL.md).

## HID access reports `0xe00002c5`

That value is `kIOReturnExclusiveAccess`. Matching every interface on the physical keypad conflicts with Karabiner's keyboard interface. The helper matches only usage page `0xff00`, usage `2`.

Close the vendor web configurator before starting the helper because WebHID and the helper cannot both own that interface.

## Input Monitoring is enabled but the helper still fails

Rebuilding or replacing an ad-hoc-signed app changes its identity for macOS privacy controls.

1. Remove the existing **K16 Codex Lights** entry from Input Monitoring.
2. Add `~/Applications/K16 Codex Lights.app` again.
3. Enable it.
4. Run `./scripts/restart.sh`.

## Inspect logs

```sh
tail -f /tmp/k16-codex-lights.log
tail -f /tmp/k16-codex-lights.stderr.log
```

## The colour does not match the current Codex state

The helper currently parses Codex's internal session JSONL. That format is not a supported public API and may change. Run the parser fixture test with `./scripts/test.sh`, then open an issue with the Codex app version and redacted event shapes. Do not attach private session files.

The monitor follows up to 64 recently modified sessions from the preceding seven days. A pending `request_user_input` event has priority over tasks that are still thinking. Check the derived aggregate without changing the lights:

```sh
"$HOME/Applications/K16 Codex Lights.app/Contents/MacOS/K16CodexLights" --status-once
```

If the sidebar says **Needs input** but this command does not print `needs_input`, the Codex event shape may have changed. Capture only the event type, tool name, and Codex version when reporting it; never publish session content.

If colours appear to overwrite one another, run `./scripts/doctor.sh`. Exactly one `K16CodexLights` process should be running. The installer stops manually launched copies and disables the obsolete `au.com.hareshsuppiah.k16codexlights` login service used by early development builds.
