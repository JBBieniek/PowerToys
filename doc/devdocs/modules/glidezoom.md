# GlideZoom

GlideZoom is an independent PowerToys utility for interactive desktop
magnification. Its source originates from the contributor-supplied GlideZoom
0.6.2 archive. ZoomIt remains a separate utility.

## Architecture

- `src/modules/GlideZoom/GlideZoom` builds `PowerToys.GlideZoom.exe` from the
  native magnification engine. Windows Graphics Capture, Direct3D 11 and
  DirectComposition present a magnified view on one selected monitor or separate
  views on independent monitors. A linked mode uses the Windows full-screen
  magnification API for the virtual desktop.
- `src/modules/GlideZoom/GlideZoomModuleInterface` implements
  `PowertoyModuleIface` and starts the engine with `--powertoys <runner PID>`.
- `src/settings-ui/Settings.UI/SettingsXAML/Views/GlideZoomPage.xaml` provides
  the PowerToys Settings page. GlideZoom starts disabled.
- `src/modules/GlideZoom/Tests` contains the original state, geometry and input
  suites with Visual Studio Test adapters, plus PowerToys integration tests.
- `src/modules/GlideZoom/Fuzzing` exercises settings, touchpad parsing, input
  routing, shortcuts, wheel intent, pointer policies, and deployment policies.

The module interface creates auto-reset events named
`Local\PowerToys_GlideZoom_Exit_<runner PID>` and
`Local\PowerToys_GlideZoom_Settings_<runner PID>` before starting the engine.
The engine holds a process handle to Runner. An exit signal or Runner termination
requests clean engine shutdown. A settings signal queues a validated reload at a
safe boundary. Compatible changes retain the current zoom, cameras, and parked
views. Input-affecting changes discard old gesture/pointer history; preview-only
changes retain the input epoch. Changing scope or lowering the zoom ceiling below
a live camera requires restoration first. Capture, handoff, restoration, and
suspended states defer unsafe changes; a settings update does not override a user
pause. The interface does not forcibly terminate the engine while it owns desktop
input mapping.

GlideZoom retains its separate recovery guardian, launched through `--watch`.
It journals recognized input/cursor state and attempts recovery after confirmed
engine termination. Recovery is bounded by the underlying shared Windows APIs;
other magnification applications can change the same system state.

## Settings contract

Settings use `%LOCALAPPDATA%\Microsoft\PowerToys\GlideZoom\settings.json`,
with `name` equal to `GlideZoom`, schema `version` equal to `1.0`, and ordinary
PowerToys `properties` objects containing a `value`. The engine accepts `1` as an
equivalent initial version. All fields are validated before application.

| Property | Default | Values |
| --- | --- | --- |
| `WheelModifier` | 0 | 0: Left Ctrl + Left Alt; 1: Ctrl; 2: Left Alt; 3: Windows |
| `MaximumZoom` | 8 | 2, 4, 8, 16 |
| `Sensitivity` | 1 | 0: gentle; 1: balanced; 2: fast |
| `DoubleControl` | false | Double-tap Left Ctrl, hold second press |
| `ModifiedDoubleClick` | false | Modified double-click, hold second click |
| `ReverseDirection` | false | Reverse wheel and drag direction |
| `CenterFollow` | false | Keep the pointer centered |
| `ThreeFingerTouchpad` | false | Opt-in raw touchpad gesture |
| `ZoomScaledPointer` | true | Automatically adjust pointer speed with magnification |
| `PointerSpeed` | 100 | Integer 25 through 300, percent of pointer movement on magnified views |
| `ZoomMultiplier` | 1 | Integer 1 through 100 |
| `EdgePanMultiplier` | 1 | Integer 1 through 10 |
| `ZoomScope` | 2 | 0: selected monitor (one view); 1: all monitors together; 2: independent monitors |
| `ScopeIndicatorMode` | 0 | 0: off; 1: while moving or zooming; 2: shortcut only; 3: always while zoomed |
| `ScopeIndicatorStyle` | 0 | 0: monitor overview; 1: full-screen scope outline; 2: overview and screen outline |
| `ScopeIndicatorSize` | 20 | Integer 10 through 35, percent of monitor width |
| `ScopeIndicatorOpacity` | 55 | Integer 10 through 90, screen outline opacity percent |
| `OverviewOpacity` | 55 | Integer 0 through 100, overview opacity percent |
| `ScopeIndicatorBorder` | 2 | Integer 1 through 8, pixels |
| `ScopeIndicatorDuration` | 1500 | Integer 500 through 5000, milliseconds |
| `ScopeIndicatorDetails` | true | Show source X/Y, width, height, and magnification |
| `ScopeIndicatorX`, `ScopeIndicatorY` | 100 | Integer 0 through 100, percent of available overview space |
| `ScopeIndicatorFade` | 120 | Integer 0 through 400, milliseconds; 0 changes immediately |
| `WheelAcceleration` | false | Add an optional boost for fast wheel scrolling |
| `WheelAccelerationLimit` | 3 | Integer 1 through 4, maximum boost multiplier |

The default zoom target applies at startup and when that target setting changes. The
scope toggle shortcuts can override it for the current session. Fresh settings
use independent monitors; missing `ZoomScope` preserves selected-monitor behavior
in older JSON files. Standalone INI version 11 writes all current fields. Version
9 supports scopes 0 through 2, version 8 retains its explicit 0/1 scope, and
versions before 8 migrate to scope 0. Missing new indicator and acceleration
options retain their defaults. INI versions through 10 and JSON files without
`PointerSpeed` use 100. Missing `OverviewOpacity` inherits the earlier
`ScopeIndicatorOpacity` value, preserving a customized overview's appearance.

On magnified views, pointer movement uses the Windows-processed delta multiplied
by `PointerSpeed / 100`, then divided by zoom when `ZoomScaledPointer` is enabled.
The speed adjustment remains available when automatic scaling is off. At 1x and
on unmagnified monitors, Windows pointer movement is unchanged. These options do
not change the Windows mouse preferences.

Independent monitors retain their own zoom and source region as the pointer
crosses screens. Selected-monitor mode owns one view: starting zoom on another
monitor restores the previous one before transferring it. All-monitor mode uses
one shared desktop transform. Only one Windows input mapping can be active, so
monitor handoffs publish the new mapping before clicks are forwarded. A first
click during a handoff can be consumed. Dragging a window can pan beyond the
current crop within its monitor. A forwarded drag stays on its original monitor
until its physical button is released; monitor handoff then becomes available.

Independent-view startup retains camera-only wheel intent for at most two
seconds and 64 detents. Acceleration uses the original hook timestamps across
the startup barrier. Additional travel is coalesced until the first frame and
input mapping are verified. Releasing the modifier ends admission while keeping
already accepted travel; reset, timeout, or leaving that monitor cancels it.
Clicks and button releases are never replayed. Application-owned cursor clipping
is respected during window drags, and an uncertain boundary correction does not
terminate the hook or swallow a physical release owed to the application.

The view indicator is drawn by the GPU renderer in selected-monitor and
independent-monitor modes. Linked all-monitor mode has no indicator. The overview
shows the whole captured monitor with a proportional frame around the magnified
source region. The full-screen scope outline draws that proportional region at
its original position on the monitor. Style 2 shows both together with the same
movement visibility, shortcut reveal, and fade timing. Overview and screen outline
opacity are independently adjustable; overview opacity 0 hides the preview.
Existing styles
0 and 1 retain their original meaning and the default remains 0. Optional
details show the source coordinates, dimensions, and current zoom. Overview X/Y
position is measured within the space left after its size is applied: 0 aligns
left/top and 100 aligns right/bottom. All styles use the configured fade duration.
These options are editable in PowerToys Settings; the engine window shows the
core controls read-only in PowerToys mode. Ctrl + Left Alt + V reveals an enabled
indicator for its configured duration.

The existing gesture engine retains its fixed shortcuts:

| Shortcut | Action |
| --- | --- |
| Left Win + Left Alt + `=` / `-` | Zoom in/out; hold to repeat |
| Ctrl + Alt + `=` / `-` | Backup zoom controls |
| Ctrl + Alt + Z | Toggle zoom on the current monitor using the configured per-monitor mode |
| Ctrl + Alt + Shift + Z | Select/toggle all-monitor zoom |
| Ctrl + Alt + I | Select/toggle independent-monitor zoom |
| Ctrl + Alt + V | Reveal the enabled view indicator |
| Ctrl + Alt + Esc | Reset all zoom to 1x |
| Left Ctrl + Left Alt + Space, held | Vertical mouse movement changes zoom |
| Ctrl + Alt + S | Copy zoomed view |
| Ctrl + Alt + Shift + F | Fit the foreground resizable window to the committed source crop |

Both zoom-key chords also accept numpad Add/Subtract. Shift plus the `=` key
is accepted for `+`; Shift is unnecessary for the ordinary `=` shortcut.
The Windows chord uses the left Windows and left Alt keys. The Ctrl + Alt commands
use Left Alt; Right Alt/AltGr remains available for text entry. An occupied backup
shortcut is left with its owner. Window fitting does not activate a window or
change fullscreen/maximized windows. It makes one asynchronous request, respects
application sizing constraints, and reports the observed result without forcing
a resize or rolling application geometry back.
A temporary stale input heartbeat suspends zoom and discards pending input;
controls resume only after the live worker and desktop are verified again.
An actual input-thread failure still requires restarting the engine.

Three-finger touchpad input requires compatible raw contacts. Windows'
three-finger taps and swipes must be set to Nothing to avoid duplicate actions;
the raw observer cannot suppress Windows' gestures.

## Deployment, policy, and resources

The first PowerToys integration supports signed builds in standard secure machine
locations: strict subdirectories of the Windows-known Program Files, Program Files
(x86), or System32 directories. The native interface checks the actual Runner,
interface DLL, and engine paths before enabling. Settings, Dashboard, and Quick
Access check the same fixed runtime files. Both original paths and handle-resolved
paths must remain under the corresponding secure root. Per-user locations,
portable/custom locations outside those roots, UNC paths, and unverifiable paths
are unavailable; the saved enable preference is retained. These checks do not
grant privileges: Windows must still validate the signature and provide the
engine's actual `uiAccess="true"` token. Unknown installation scope is not inferred
from unrelated registry entries. No security policy is relaxed.

GlideZoom honors the existing global utilities policy,
`ConfigureGlobalUtilityEnabledState`. Settings and shared module toggles reflect
its enforced state. There is no dedicated GlideZoom enterprise policy yet, and
policy cannot make an unsupported installation available.

The release signing list includes the engine and interface DLL. The installer
harvests them from the root build output. Its checked, read-only
`VerifyGlideZoomStoppedCA` runs after the existing process-shutdown action and before
`InstallValidate`, including before the default major-upgrade removal. It matches
the current and previous installation image paths across sessions, retains process
handles, and allows up to 30 seconds for both engine and same-image recovery
guardian to exit, including 500 milliseconds of fresh empty scans. Uncertain
queries or timeout refuse the installation/update. When MSI reports an upgrade,
the previous installation path must also be present, canonical, an existing
directory, and resolvable through a directory handle; discovery failure is not
treated as a first install. A new target directory may be absent on first install.
The verifier never terminates
GlideZoom, sends input/window messages, or restores magnification itself. Secure
desktop recovery or another session may require the user to close PowerToys and
retry. This observes process quiescence at the check; it is not a transaction-wide
launch lock or proof that Windows restored the physical display correctly.

Native UI strings live in the engine and interface `Resource.resx` files and use
the repository's `tools/build/convert-resx-to-rc.ps1` build pipeline. Generated
resource headers/RC files are build outputs. Engine resource lookups preserve
native error codes and provide English fallback; dynamic messages validate
indexed placeholders so translations can reorder fields without interpreting
arguments as format strings. This makes the UI localizable; it does not imply
that translations for every language have already shipped. Diagnostic logs remain
separate from UI strings.

## Build and validation

Build native projects and Settings using `tools/build/build.ps1`, following the
repository build guidelines. Build the test projects successfully before running
their DLLs through Visual Studio Test Explorer or `vstest.console.exe`.

A compiled unsigned executable cannot provide a usable UIAccess session. Runtime
acceptance requires a correctly signed build in a supported installation location.
The integration does not import the standalone installer or create/trust signing
certificates.

Hardware validation must include multiple monitors with different DPI settings,
cursor/click alignment, keyboard and touchpad input, coexistence with other
magnifiers, display changes, session transitions, and engine/Runner termination.
API mocks and portable tests cannot establish compositor smoothness or physical
input latency or the ordering of real native cursor writes and hook callbacks.
Results must identify the exact built revision; earlier physical checks do not
establish acceptance of later source changes. End-to-end MSI upgrade/uninstall,
cross-session shutdown, and secure-desktop recovery also require separate checks.
This document describes implemented behavior and test scope, not a record of a
completed build or hardware acceptance run.

The selected-monitor renderer checks overlapping windows above its overlay,
raises only its own window without activation, and verifies the resulting order.
Repeated contention, detected capture protection, or an unverifiable order suspend zoom
and restore the owned mapping. Native menus are never hidden or moved.

The capture-excluded magnification overlay may be absent from third-party
screenshots/recordings. Exclusion is required to prevent the renderer from
capturing itself. All-monitor zoom uses a different Windows magnification path;
recording behavior depends on the recorder's capture API. GlideZoom does not
currently encode video or export a stream to external recorders.

Live selected-monitor frames occupy a bounded three-frame Windows Graphics
Capture pool, a private working texture, and a two-buffer composition swap chain.
Those desktop textures use FP16 scRGB; cursor pixels use a separate BGRA texture.
GPU textures are reused; there is no recording history or continuous CPU frame
archive. Capture sessions and their pool close when stopped; reusable render
resources remain until cleanup. Copy zoomed view reconstructs a fresh desktop
crop into a clipboard bitmap and can briefly show the unzoomed desktop while
capturing. The clipboard image remains until replaced or cleared by Windows.

The global system cursor is hidden only while the submitted GPU frame covers its
current hotspot. Eligibility is bound to submitted geometry and cursor shape,
checked with fresh native monitor/position data, and rechecked around a hide.
Ordinary monitor crossings release the cursor before GPU presentation can wait;
fully restored coverage loss is not classified as an engine fault. A pending
frame cannot use an unsubmitted camera or cursor shape as proof of replacement.

Engine startup, shortcut availability, and terminal errors
are logged under `%LOCALAPPDATA%\Microsoft\PowerToys\GlideZoom\Engine\Logs`.
The module interface separately records engine PID, lifetime, exit code, and
whether shutdown was requested. Frame contents and input text are not logged.
