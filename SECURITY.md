# Security and privacy

## Local data

The lighting helper reads the newest file under `~/.codex/sessions` to derive a five-state indicator. It does not transmit session data. Diagnostic logs contain timestamps, state colours, and HID errors, not prompt contents.

## Permissions

macOS Input Monitoring is required for direct HID access. Grant it only to the installed `K16 Codex Lights.app`. The project does not attempt to bypass macOS privacy controls.

## Network access

Normal shortcut and lighting operation uses no network connection. The optional vendor layout proxy connects only to `www.sdcx-tech.com` to serve the vendor's live configurator through localhost.

## Reporting a vulnerability

Open a GitHub security advisory if the repository supports private reporting. Otherwise, open an issue that describes the impact without including credentials, private Codex session files, or other personal data.
