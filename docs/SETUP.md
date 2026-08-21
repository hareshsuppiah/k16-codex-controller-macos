# Setup

## 1. Install prerequisites

Install Karabiner-Elements and Xcode Command Line Tools:

```sh
xcode-select --install
```

Karabiner-Elements is available from <https://karabiner-elements.pqrs.org/>.

## 2. Confirm the keypad

Use the procedure in [Hardware identification](HARDWARE.md). The supplied configuration is for VID `13998` and PID `9333`.

## 3. Build and install

From the repository root:

```sh
./scripts/install.sh
```

This installs:

- `~/Applications/K16 Codex Lights.app`;
- `~/Library/LaunchAgents/io.github.hareshsuppiah.k16codexlights.plist`;
- `~/.config/karabiner/assets/complex_modifications/k16-codex-controller.json`.

The build is ad-hoc signed for local use. Rebuilding changes the code signature and may require Input Monitoring to be approved again.

## 4. Approve Input Monitoring

1. Open **System Settings → Privacy & Security → Input Monitoring**.
2. Add `~/Applications/K16 Codex Lights.app` if it is not listed.
3. Turn it on.
4. Run `./scripts/restart.sh`.

macOS requires the user to make this security decision. The installer does not bypass it.

## 5. Enable Karabiner rules

1. Open Karabiner-Elements.
2. Go to **Complex Modifications**.
3. Choose **Add predefined rule**.
4. Enable the rules under **K16 Codex Controller**.

Enable only the controls you want. The rules are device-scoped, and the Codex knob rules are additionally limited to the Codex desktop app.

## 6. Configure Codex shortcuts

Open Codex → **Settings → Keyboard shortcuts**. Use the Mac keyboard, not the K16 pad, while the shortcut recorder is active.

Assign:

- increase reasoning effort: Command-Control-Option-=;
- decrease reasoning effort: Command-Control-Option--;
- open model picker: Control-Shift-M.

The K16 knobs emit consumer media events that Codex's recorder may ignore. Karabiner translates those events to the ordinary shortcut chords above.

## 7. Test

```sh
./scripts/doctor.sh
./scripts/test.sh
```

To test one colour directly:

```sh
"$HOME/Applications/K16 Codex Lights.app/Contents/MacOS/K16CodexLights" --test complete
```

Accepted names are `idle`, `thinking`, `complete`, `needs_input`, and `error`.

## 8. Optional vendor configurator workaround

The vendor site did not contain a layout entry for `36ae_2475`, so it initially displayed a full QWERTY keyboard or omitted the Lighting page. The related `0816_2475` definition matches this 16-key/three-knob layout.

With Node.js installed:

```sh
node tools/vendor-layout-proxy.mjs
```

Open <http://localhost:8765> in Chrome or Edge and choose **Initialize device**. The proxy performs read-only GET/HEAD requests and maps the missing layout path to the related live vendor definition.

Close the configurator before starting the lighting helper so only one process owns the vendor HID interface.
