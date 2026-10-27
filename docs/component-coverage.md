# Component coverage and selection

Reference: [official DaisyUI component catalogue](https://daisyui.com/components/),
checked 2026-10-27. This is an independent native implementation of useful UI
behaviours, not CSS compatibility or a claim of identical DaisyUI appearance.
DaisyUI is not downloaded, bundled or required. The catalogue has 68 families;
the table explicitly records implementation, composition and exclusions.

| DaisyUI family | C++ implementation / deliberate selection |
|---|---|
| Accordion | `Accordion`; caller owns the single expanded index |
| Alert | `Alert` |
| Aura | Excluded: decorative light effect |
| Avatar | `Avatar` initials; host textures through ImGui::Image |
| Badge | `Badge` |
| Breadcrumbs | `Breadcrumbs`; ancestor activation returned to host |
| Button | `Button`; sizes, variants and interaction states |
| Calendar | Deferred: date/calendar semantics are not needed for the initial game/editor core |
| Card | `BeginCard` / `EndCard` |
| Carousel | Excluded from core: asset grids/lists cover editor selection |
| Chat bubble | Deferred: in-game chat layout requires actual product use and message model |
| Checkbox | `Checkbox` |
| Collapse | `Collapse`; caller owns expanded state |
| Countdown | Compose formatted host time with text/progress; no animated numeral system |
| Diff | Deferred: no source/image diff algorithm or comparison renderer claimed |
| Divider | `Divider()` / `Divider(label)` |
| Dock | Compose horizontal `Navigation` in host-positioned window; not ImGui docking |
| Drawer sidebar | Host boolean + `BeginSplit` / `NextSplit` / `EndSplit`; gallery shows hide/show |
| Dropdown | `BeginDropdown` / `EndDropdown` |
| FAB / Speed Dial | Excluded: use explicit toolbar/actions |
| Fieldset | `BeginFieldset` / `EndFieldset`; inherited disabled state |
| File Input | `FileInput` + explicit asynchronous `FilePicker` host capability |
| Filter | Compose `Radio`, `Select` or `Navigation`; filtering belongs to host data |
| Footer | Compose text/status in a host-positioned area |
| Hero | Excluded: marketing layout |
| Hover 3D Card | Excluded: decoration, incompatible with touch-first purpose |
| Hover Gallery | Excluded: hover-only asset navigation |
| Indicator | `StatusDot` / `Badge` composition; no arbitrary overlay API claimed |
| Text Input | `Input`; UTF-8 buffer, readonly/disabled/required/invalid/help |
| Join | Adjacent buttons/fields using ImGui layout; no separate retained group state |
| Kbd | `Kbd` shortcut display; actual shortcut binding remains host-owned |
| Label | `Label` with required marker |
| Link | Compose a ghost `Button` with explicit host action; no implicit OS URL launch |
| List | `VirtualList` fixed-height rows; caller data and selection |
| Loading | `Loading` and button loading state |
| Mask | Excluded: decorative shape cropping |
| Megamenu | Compose dropdown/menu for actual commands; no desktop-only megamenu system |
| Menu | `MenuItem`, `Navigation`, `BeginContextMenu` / `EndContextMenu` |
| Browser mockup | Excluded: presentation decoration |
| Code mockup | Excluded: presentation decoration; use a proper text/editor integration if needed |
| Phone mockup | Excluded: presentation decoration |
| Window mockup | Excluded: actual host windows provide layout |
| Modal | `OpenModal`, `BeginModal`, `EndModal`; real popup focus/input handling |
| Navbar | Horizontal `Navigation` in host layout |
| OTP | Excluded from engine core: application-specific authentication |
| Pagination | `Pagination`; bounded page state, zero-items and overflow-safe page count |
| Progress | `Progress` |
| Radial progress | `RadialProgress`; non-finite and out-of-range values handled |
| Radio | `Radio` |
| Range slider | `Slider` and `Range` with two bounded handles |
| Rating | Compose radio/selection when an actual product needs it; no star-animation system |
| Select | `Select`; clipped options, stable per-row IDs |
| Skeleton | `Skeleton`; static, without decorative idle animation |
| Stack | Compose groups/children using host layout; no artificial overlapping card stack |
| Stat | `Stat` |
| Status | `StatusDot` |
| Steps | `Steps`; compact vertical layout for narrow panels |
| Swap | Host boolean selects content; no duplicate internal state |
| Tabs | `Tabs` / `Navigation` |
| Table | `BeginTable` / `EndTable`; caller-owned sort, resizable columns, scrolling header |
| Text Rotate | Excluded: decorative animated text |
| Textarea | `Textarea`; active bounds reported to platform IME adapter |
| Theme Controller | `set_mode`, `set_custom_palette`, `set_scale`; shared palette, applied at frame boundary |
| Timeline | `TimelineItem` |
| Toast | `ToastQueue` / `Toasts`; bounded queue, expiration, persistence and dismissal |
| Toggle | `Switch`; mouse, keyboard and touch |
| Tooltip | `Tooltip`; supplementary hints, never required for primary operation |
| Validator | `Field.invalid`, `Field.help`, `Validation`; domain rules remain in host |

Additional engine/editor primitives: `TreeNode` / `EndTreeNode`, resizable
horizontal/vertical split panes, `Number`, `Integer`, `Vector3`, `ColorEdit`.
No property store, undo engine, docking workspace, node editor, animation editor,
file browser or calendar is implied by these primitives.

## Verification

Headless C++ tests exercise real ImGui frame submission, mouse/keyboard switch
activation, popup ID balance, readonly range handling, palette/scale/touch layouts,
split scopes, tree scopes, table submission, virtual-list callback bounds,
notification expiry and capacity. Release builds keep assertions enabled in tests.
Browser QA compiles the actual WASM gallery, visits all eight categories, exercises
mobile text focus, scrolling and actions, and compares canvas/render/screenshot
pixel dimensions across desktop/mobile/tablet and fractional DPI.
Cross-platform CI also builds Windows, Linux, macOS, iOS simulator and Android.
CI builds are not a substitute for physical-device IME/clipboard/accessibility QA.
