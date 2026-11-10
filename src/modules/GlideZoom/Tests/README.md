# GlideZoom tests

The native Visual Studio test projects retain the portable GlideZoom 0.6.2
regression suites and expose each suite as a test in Test Explorer/vstest.
The source import changes include paths, renames each standalone `main`, adds
explicit success returns, and translates assertion exits into exceptions with
captured diagnostics. The standalone installer/setup tests are excluded because
PowerToys owns installation.

`GlideZoom.UnitTests` covers portable suites and focused native tests: zoom math, input ownership and
queues, settings migrations, session restoration, monitor geometry, renderer
policies, cursor ownership, pointer motion, touchpad recognition/assembly,
timing, capture policy, tray state, and the 3,780-row composition matrix.
Overlay-order scenarios include hidden initialization, arriving menus, bounded
contention, capture-protection observations, retained frames, and mapping recovery.
Input-health tests distinguish delayed workers from stopped or faulted workers
and verify cancellation and physical release ownership. Focused suites cover live
settings classification without resetting compatible zoom, independent-monitor
handoffs and return crops, camera-only wheel activation, AltGr-safe shortcut
ownership, and pointer gain/echo/deferred-readback policies. Native pointer shims
model delayed writes and historical targets; they do not prove Windows callback
or magnification ordering on hardware.

`DeploymentPolicyTests` covers canonical secure-root boundaries and refusal of
aliases/unsupported namespaces, plus bounded engine/guardian shutdown observation.
Uncertainty, backward time, timeout, and a reappearing process cannot satisfy the
500-millisecond empty-scan requirement. These are policy tests, not an invocation
of MSI or the live process scanner. Upgrade detection additionally requires a
verified previous installation directory; a missing discovery result cannot
silently omit the old engine/guardian path when the new target differs.

Five separate test DLLs (`Capture`, `Hid`, `Latency`, `Monitor`, and `Paint`)
isolate the upstream Windows API shims. Only the corresponding suite translation
unit sees its mock SDK headers. These tests exercise production boundary code;
they do not validate real Windows drivers, GPU presentation, or compositor state.

`GlideZoom.Integration.UnitTests` uses the real Windows SDK to verify the
PowerToys JSON contract, strict Runner PID parsing, and the settings/exit events.
Its native resource suite loads the built engine/interface as PE data, checks
embedded English fallback strings, preserves prior native error codes, and tests
validated translated-format reordering. It does not execute the loaded engine,
start zoom hooks, or change the desktop. The engine and interface project
references generate the resources before these tests run.

Build all seven test projects from this folder using the repository script:

```powershell
& ..\..\..\..\tools\build\build.ps1 -Platform x64 -Configuration Debug
```

After the build succeeds with exit code 0, run the generated
`x64\Debug\tests\GlideZoom\*.UnitTests.dll` with Visual Studio Test Explorer or
`vstest.console.exe`. Test output is attached to the corresponding test result.
Use the tests folder's build logs under `artifacts\logs\<project>\` for failures.

Managed GlideZoom tests also cover global-policy locks, preservation of saved
preferences in unsupported installations, secure original/resolved path pairs,
and missing or redirected runtime files. Other module types remain available by
default. Build the managed test project successfully and use the repository's
VS Test/vstest workflow for its results.

Real hardware acceptance still requires multi-monitor and mixed-DPI checks,
touchpad gesture checks, display changes, sleep/session transitions, desktop
restoration after failure, and coexistence with other magnifiers. First-release
runtime acceptance uses a signed machine installation beneath Program Files,
Program Files (x86), or System32; per-user/unsupported locations remain unavailable.
The installer's read-only process-quiescence guard also needs end-to-end upgrade,
uninstall, protected-process, and multiple-session checks. Pure tests do not
provide a launch fence across the MSI transaction or prove physical recovery.

This file documents coverage and commands. It does not assert that a pending
build, test run, release-signing run, or hardware acceptance pass has completed.
