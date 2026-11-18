# Green hub responsive layout and native Settings hit-test

The home window targets1100x620 device-independent client pixels. Native
`AdjustWindowRectExForDpi` supplies frame extents; the resulting outer rectangle
is centered in the monitor work area and clamped with a16-DIP breathing margin.
At125% DPI the target client is1375x775 physical pixels. With a20x49 native frame,
that means1395x824 outer pixels. Actual frame extents are queried at runtime,
not assumed from that example. A smaller work area reduces the client size.

`games/hub_layout.c` is pure physical-pixel geometry, independent of Win32/SDL.
The header fits the approved three-row stone logo, then a centered version line,
two game cards, two separate Settings action rows, a short note and status.
Settings rectangles never intersect the game-card button hit rectangles. Their
explicit top z-order is also retained after each layout/inline-settings return.
The hub loads `dyna-logo-1plus2-v1.png`; the in-game DB1 title/logo asset remains
unchanged.

Typography follows DPI until the available client area needs a smaller scale.
Minimum tracking targets700x430 DIP client, with the same work-area clamp, rather
than an unconditional700px outer-height floor. DPI transitions clamp the suggested
outer rectangle to the destination monitor. Home controls retain IDs101/102 and
201/202, preserving existing keyboard and controller navigation.

The pure test exercises default and minimum layouts on1920x1080,2560x1440,
1366x768 and800x600 work areas at96/120/144 DPI:24 cases. Every rectangle must
remain inside the client and all eight home regions must be nonoverlapping.
Default/minimum outer rectangles must fit their work area.

## Native opt-in integration diagnostic

Set `DYNA_HUB_HOME_SELFTEST=1` and optionally `LNG_TEST_HIDDEN=1`. The home posts
its own owner-thread diagnostic after native child creation. For each Settings
button it computes the actual child center, calls `ChildWindowFromPointEx`, and
asserts the returned HWND is that Settings child before invoking `BM_CLICK`.
This covers the real failure that direct `WM_COMMAND` self-tests missed.

`LNG_SCRIPT` defaults to `wait:3;quit` if absent, so the real shared SDK Settings
view exits back to the hub. The diagnostic enables the existing renderer/HWND
preservation assertions, checks that each button caused a Settings visit, and
asserts return to the same home HWND. It prints actual DPI/client/outer sizes,
then closes its own window and exits. It neither launches a guest nor automates
the user's desktop.

Pure test and strict helper compile (`-Wall -Wextra -Werror`) pass locally.
The linked native diagnostic also passes both real Settings hit-tests,
`BM_CLICK` entry and same-HWND return at120 DPI: actual client1375x775 and
outer1393x822. Evidence is private `local/hub-layout-agent/native-result.json`.

Optional `DYNA_HUB_HOME_CAPTURE=absolute.bmp` records the home client itself
with native `WM_PRINT` before entering Settings. The same GDI paint function
handles normal paint and `WM_PRINTCLIENT`, with standard child buttons included.
It captures only this app's own HWND, including when hidden; it does not read the
desktop. Physical controller transport remains outside these layout/window tests.

The optional hidden-window capture is diagnostic: parent and child rendering
currently use different nonclient origins in the WM_PRINT path. Its PNG therefore
cannot certify exact visual spacing. Native hit-test/BM_CLICK, geometry and
same-HWND assertions remain independently passed. Do not publish that capture
as an exact client-area screenshot until the diagnostic origin is corrected.
