# GlideZoom fuzzing

`GlideZoom.FuzzingTest.vcxproj` builds `GlideZoom.FuzzTests.exe`. The harness covers:

- The PowerToys JSON codec and original INI settings codec, including atomic
  rejection and valid-settings serialization round trips. Scope invariants cover
  selected-monitor, linked-desktop, and independent-monitor choices, with seeds for all scopes and legacy
  settings that default to selected-monitor zoom.
- Strict Runner process ID parsing and numeric settings fields, including the
  complete pointer-speed text field without accepting truncated numeric prefixes.
- Hostile HID batch dimensions and descriptor coordinate normalization.
- Touchpad packet assembly and gesture state transitions, including malformed
  contacts, extreme timestamps, partial frames, and cancellation.
- The production `HidReader` against bounded Windows API contract shims, with
  corrupted raw headers/payloads, descriptor capabilities, and API failures.
- Independent-monitor routing and physical-point fallback policies, preserving
  owned button releases and refusing stale mapping generations or epochs.
- Bounded wheel activation/acceleration and shortcut ownership, including AltGr,
  cancellation, held-key repeats, and disabled command admission.
- Pointer gain, fractional feedback, and native-adapter models, including bounded
  delayed/historical readback eligibility, exact-target acknowledgments, and
  cancellation. Simulated coordinates do not establish native API ordering.
- Canonical deployment-path policy and read-only shutdown observation: malformed
  paths, root lookalikes, uncertain scans, and bounded quiet/timeout behavior.

The HID shims are shared with `Tests/HidTestApi.h`. They model the Windows HID
API contract; the Windows binary HID descriptor parser and hardware are outside
this harness. The JSON codec is compiled in a separate translation unit with
the real Windows SDK so mock SDK declarations cannot enter it.

Build from this folder with the repository script:

```powershell
& ..\..\..\..\tools\build\build.ps1 -Platform x64 -Configuration Release
```

After exit code 0, run (from the repository root):

```powershell
.\x64\Release\tests\GlideZoom.FuzzTests\GlideZoom.FuzzTests.exe .\x64\Release\tests\GlideZoom.FuzzTests\corpus -runs=10000 -max_len=16385
```

Release|x64 enables libFuzzer, coverage, and AddressSanitizer. ARM64 is not
supported. Debug|x64 provides a deterministic smoke executable over all three valid INI
zoom scopes combined with all three indicator styles and all 256 repeated-byte inputs; it does not perform fuzzing or
provide sanitizer instrumentation. Keep this project disabled for ordinary
solution builds except Release|x64, following the repository fuzzing convention.
The project copies valid JSON/INI and touchpad seed inputs into its output corpus.

The PowerToys JSON contract remains version `1.0`: `ZoomScope` values are `0` for
one selected-monitor view, `1` for linked desktop zoom, and `2` for independent
monitor views. Fresh settings default to `2`; legacy JSON without the field
retains `0`. INI versions `9` through `11` persist all three values. Version `8` retains its
explicit `0`/`1` choice, and older INI versions migrate to `0`. Validated parsing
never changes the caller's configuration when a scope is malformed or unsupported.

INI version `10` also persists the indicator and wheel acceleration options.
Version `11` additionally requires pointer speed and separate overview opacity.
Earlier INI versions and JSON missing these fields use pointer speed 100 and
inherit overview opacity from the earlier screen-outline opacity setting.
Older INI and JSON settings receive these defaults when the new fields are absent;
present JSON fields must have the correct type and satisfy the same bounds.

| Property | Range / meaning | Default |
| --- | --- | --- |
| `ScopeIndicatorMode` | 0 off, 1 while moving, 2 shortcut only, 3 always | 0 |
| `ScopeIndicatorStyle` | 0 monitor overview, 1 full-screen scope outline, 2 both | 0 |
| `ScopeIndicatorSize` | 10–35 percent of monitor width | 20 |
| `ScopeIndicatorOpacity` | 10–90 percent, screen outline | 55 |
| `OverviewOpacity` | 0–100 percent, overview preview | 55 |
| `PointerSpeed` | 25–300 percent on magnified views | 100 |
| `ScopeIndicatorBorder` | 1–8 pixels | 2 |
| `ScopeIndicatorDuration` | 500–5000 milliseconds | 1500 |
| `ScopeIndicatorDetails` | Show source coordinates, dimensions, and zoom | true |
| `ScopeIndicatorX`, `ScopeIndicatorY` | 0–100 percent of available overview space | 100 |
| `ScopeIndicatorFade` | 0–400 milliseconds; 0 changes immediately | 120 |
| `WheelAcceleration` | Enable extra zoom from fast wheel scrolling | false |
| `WheelAccelerationLimit` | 1–4 maximum boost multiplier | 3 |

Overview, outline, and combined-style corpus seeds exercise all new fields.
Version 11 seeds cover both pointer-speed bounds, automatic scaling on and off,
and fully transparent or opaque overview previews. Earlier version 10 seeds
exercise preview-opacity inheritance.
Style 2 shares movement visibility and fade timing across both visual elements;
existing styles 0 and 1 retain their meaning. Existing seeds retain
the earlier eleven-field JSON and INI schemas for migration coverage.

`OneFuzzConfig.json` follows the existing native OneFuzz ingestion layout and is
copied alongside the Release executable and ASan runtime for the fuzz pipeline.

The deployment harness exercises pure policy only. It does not enumerate or stop
processes, invoke MSI, verify signatures/UIAccess, or establish that an engine or
guardian restored the live desktop. Runtime availability is restricted to the
supported Program Files, Program Files (x86), or System32 machine paths; the
native module and managed UI enforce the same original/resolved-path boundaries
and existing global utilities policy. Native resource loading/localization and
live settings application have separate unit/integration coverage.

This README specifies harness scope and commands, not completed validation.
Actual Windows input ordering, mixed-DPI rendering, multiple sessions, secure
desktop recovery, and installer upgrade/uninstall remain outside this fuzzer.
