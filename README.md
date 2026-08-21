# K16 Codex Controller for macOS

Turn an inexpensive 16-key, three-knob USB macro pad into an unofficial Codex control surface for macOS.

The project maps useful editing actions, uses two rotary knobs for Codex model and reasoning-effort controls, and changes the keypad lighting to reflect the current Codex task state.

> [!IMPORTANT]
> This is an experimental community project. It is not affiliated with or endorsed by OpenAI, Work Louder, Temu, G-LRA, or the keypad manufacturer. It does not install modified firmware.

## What it does

| Colour | Meaning |
|---|---|
| White | Codex is idle |
| Blue | Codex is working |
| Green | The task completed successfully |
| Yellow | Codex needs input or approval |
| Pink | An error occurred or the task was aborted |

The tested layout also provides:

- top knob: increase or decrease Codex reasoning effort;
- middle knob: open and navigate the Codex model picker;
- dedicated Fn/Globe, Return, Escape, Select All, Copy, Paste, and user-defined screenshot buttons.

## Tested hardware

- Listing: [Temu 16-key macro programming keypad with three knobs](https://www.temu.com/goods.html?goods_id=601101322436966)
- USB/device name: `k16_n3` / `G-LRA`
- USB vendor ID: `0x36ae` (`13998`)
- USB product ID: `0x2475` (`9333`)
- RGB interface: usage page `0xff00`, usage `2`

Temu listings and internal hardware can change without notice. A visually identical keypad may use a different USB identity or lighting protocol. Check [Hardware identification](docs/HARDWARE.md) before installing.

## Requirements

- macOS 13 or later;
- the Codex desktop app;
- [Karabiner-Elements](https://karabiner-elements.pqrs.org/);
- Xcode Command Line Tools (`xcode-select --install` if `clang` is unavailable).

Node.js is only required for the optional vendor configurator workaround.

## Quick start

```sh
git clone https://github.com/hareshsuppiah/k16-codex-controller-macos.git
cd k16-codex-controller-macos
./scripts/install.sh
```

The installer builds an ad-hoc-signed local app, installs it in `~/Applications`, installs the Karabiner rule file, and registers a per-user login service.

Then complete the two macOS/app steps that cannot be safely automated:

1. Open **System Settings → Privacy & Security → Input Monitoring** and enable **K16 Codex Lights**.
2. Open Karabiner-Elements → **Complex Modifications → Add predefined rule**, then enable the K16 rules you want.

In Codex → **Settings → Keyboard shortcuts**, assign these shortcuts using the Mac keyboard:

| Codex action | Shortcut expected by the supplied Karabiner rules |
|---|---|
| Increase reasoning effort | Command-Control-Option-= |
| Decrease reasoning effort | Command-Control-Option-- |
| Open model picker | Control-Shift-M |

Restart the service after granting Input Monitoring:

```sh
./scripts/restart.sh
```

See [Setup](docs/SETUP.md) for the complete walkthrough.

## Default controls

The raw codes below are what the tested firmware emits. Physical placement can differ across batches.

| Raw keypad event | Default result |
|---|---|
| `F17` | Fn/Globe |
| Keypad `0` | Return |
| Keypad `.` | Escape |
| Keypad Enter | Command-Shift-2 |
| Keypad `7` | Command-C |
| Keypad `8` | Command-V |
| Keypad `-` | Command-A |
| Top knob clockwise/counter-clockwise | Increase/decrease reasoning effort |
| Middle knob rotation and press | Open, navigate, and confirm the model picker |

Use Karabiner-EventViewer to confirm your unit's events before changing the supplied rules.

## How the lighting works

`K16CodexLights` watches the most recently active local Codex sessions under `~/.codex/sessions`, derives a small state for each task, and sends a monochrome HSV colour to the keypad's vendor HID interface. It does not upload session content or call a network service.

Status priority is **needs input → error → thinking → complete → idle**. This means a task waiting for your response turns the pad yellow even while another task is still working. The monitor recognises both current `function_call` and older `custom_tool_call` representations of Codex's `request_user_input` event.

Codex's session-file format is not a public API. A future Codex update may require the parser to be updated.

## Repository contents

- `src/`: native macOS HID/status monitor;
- `config/`: importable Karabiner rules;
- `scripts/`: build, install, restart, test, and uninstall tools;
- `tools/`: optional vendor web-configurator layout workaround;
- `docs/`: setup, hardware, protocol, and troubleshooting notes.

For the problem-solving journey and the reasons behind the implementation, see [How the prototype was worked out](docs/BUILD_NOTES.md).

## Why this exists

The goal is a low-cost, hackable approximation of the useful control-surface ideas demonstrated by products such as the Work Louder Codex Micro. It is not a feature-for-feature clone: this project uses an off-the-shelf macro pad, Karabiner mappings, and a host-side lighting helper.

## Privacy and safety

- Lighting and shortcut processing are local.
- The helper requests Input Monitoring because macOS requires that permission for direct HID access.
- The helper targets only the tested vendor/product ID and vendor-defined RGB interface.
- The optional vendor proxy contacts `www.sdcx-tech.com`; it is not required during normal use.
- No firmware is flashed.

See [SECURITY.md](SECURITY.md) for reporting and safe-use guidance.

## License

MIT. See [LICENSE](LICENSE).
