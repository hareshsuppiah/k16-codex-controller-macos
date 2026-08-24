# Contributing

Compatibility reports and small, testable improvements are welcome.

For another keypad variant, include:

- manufacturer/listing description without order identifiers;
- vendor and product IDs;
- primary usage page and usage for the lighting interface;
- Karabiner-EventViewer output names for each control;
- whether each RGB status test succeeds.

Do not upload private Codex session files, vendor firmware, or copied vendor application bundles. Add redacted synthetic fixtures when changing the status parser.

Before opening a pull request:

```sh
./scripts/test.sh
```
