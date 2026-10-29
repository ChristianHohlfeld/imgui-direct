# Component state audit

The original eight-category gallery demonstrated normal examples, not a complete
state audit. A green build or a normal-state screenshot did not establish complete
state behaviour. The default gallery now opens the **Zustandsprüfung** inspector.

## Reproducible scope

The catalogue in `src/state_gallery.cpp` contains all **48 existing component
families** and **273 applicable component/state combinations**. Every combination
is submitted in light/dark themes and desktop/touch layouts: **1,092 native render
cases**, plus the same browser screenshot matrix. The CI artifact contains named
PNG files and `states/manifest.json`, rather than one generic screenshot per page.
The matrix copies the completed real WebGL framebuffer before the browser discards
it, with independent browser-screenshot pixel comparisons. It is not a replacement
renderer or a mock UI. Crops include the visible component/state labels; popup and
notification cases include the complete frame.

Hover, pressed and keyboard focus are tested with real input separately; they are
not flags that paint a simulated state onto the component.

Choose a component, its state, size XS–XL and theme. Buttons additionally expose all
ten variants. Reset restores the example's actual application data. Examples remain
editable where permitted. The inspector reports changes, hover, active, focus and
lock state from the actual component, before helper/validation content is drawn.
The ordinary usage gallery remains available under **Beispiele anzeigen**.

| Family | Applicable persistent states / scenarios |
|---|---|
| Button | normal, selected, disabled, readonly, loading, error, warning, success, long label; 10 variants, 5 sizes |
| Input / Textarea | normal, empty, readonly, disabled, loading, required, error, warning, success, long content |
| Select | normal, selected, empty options, readonly, disabled, loading, required, error, warning, success; popup through real activation |
| Checkbox | off/on, mixed, readonly, disabled, loading, required, error, warning, success |
| Radio | unselected/selected, readonly, disabled, loading, required, error, warning, success |
| Switch | off/on, readonly, disabled, loading, required, error, warning, success, wrapped label |
| Slider / Number / Integer / Range | normal, minimum, maximum, readonly, disabled, loading, required, error, warning, success |
| Vector3 / ColorEdit | normal, readonly, disabled, loading, required, error, warning, success |
| FileInput | no file/file selected, request, readonly, disabled, loading, required, error, warning, success; explicit host callback |
| Tabs / Navigation | selection, disabled items, long labels; tabs wrap instead of disappearing in narrow panels |
| Breadcrumbs | populated/empty, current item, long path |
| Pagination | empty, first/middle/last page, disabled |
| Accordion / Collapse | open/closed, actual toggling, disabled |
| Dropdown | open/closed, selected, readonly, disabled, loading, validation and long label; actual popup dismissal |
| ContextMenu | open/closed, enabled/disabled actions |
| MenuItem | checked/unchecked, enabled/disabled |
| Modal | closed/open, cancel/confirm, long content; actual modal focus |
| Card / Fieldset | empty/content, long content; fieldset additionally inherited disabled and readonly/validation fields |
| Table | populated/empty, selected row, disabled rows, actual sorting metadata applied by the example |
| TreeNode | open/closed, selected, leaf, disabled |
| VirtualList | populated/empty, selected, disabled; fixed-height clipping |
| Split | horizontal/vertical, bounded minimum/maximum, disabled; pointer and keyboard resizing |
| Badge / Alert / StatusDot / TimelineItem | info/success/warning/error, long content where meaningful |
| Progress / RadialProgress | zero/partial/complete, semantic status |
| Loading / Skeleton | busy / placeholder display, sizes; no pretend clickable state |
| Stat / Avatar / Kbd / Divider / Typography | normal, empty or long content where applicable |
| Steps | pending, current, completed; not started, first/middle/last and all completed |
| Tooltip | pointer hover, keyboard focus, disabled explanation, long content |
| Label / Validation | required indicator; explicit none/success/warning/error feedback |
| Toast | empty queue, visible semantic status, long message, dismissal and expiration |

Not every state is applicable to every component: a divider has no checked state,
a loading indicator is not clickable, and static text is not an editable field.
The inspector omits these meaningless combinations explicitly. This audit covers
the existing library; it does not turn the exclusions in `component-coverage.md`
into implemented DaisyUI families.

## Implementation corrections

- All editable fields share `field_detail.hpp`: semantic borders/backgrounds,
  disabled precedence, readonly locking, busy feedback, required markers and focus.
  Readonly text remains selectable/copyable; other readonly controls cannot mutate.
- Checkbox/radio have size-aware icons within full touch targets, label activation,
  mixed-checkbox rendering and genuine keyboard navigation. The icon is no longer
  inflated to the entire touch-target size. Switch labels are also clickable and wrap.
- Custom-drawn pixels use ImGui's effective alpha, including inherited disabled
  state. No custom switch remains bright inside a disabled fieldset.
- Helper text no longer replaces the native item bounds, ID or interaction state.
  `last_control`/`last_interaction` refer to the actual control. Error takes precedence
  over success when contradictory flags are supplied.
- Button loading text and spinner have separate space; long button labels get an
  ellipsis and a full tooltip. Narrow tabs wrap rather than return without rendering.
- Empty selects are visibly empty instead of vanishing. Badges and field labels wrap.
  Heading restores the host's previous window font scale.
- Scrollable tables, lists, cards and split panes share the native focus scope,
  so Tab can cross their child-window boundaries. Changing the inspector example
  reinitializes navigation rather than keeping a focus ID for a vanished widget.
- `Validation(message, ValidationState)` provides explicit success/warning/error
  presentation. The existing bool overload remains source compatible.

## Checks and limits

`ui_full_state_audit` checks all 1,092 cases for balanced style/ID/disabled scopes;
it additionally sends mouse, text and keyboard events, verifies locked fields,
label hit areas, touch hover suppression, size geometry and actual focus activation.
`state_review.mjs` renders the complete browser matrix, attempts mutations against
locked fields, captures real hover/pressed/focus, and checks touch label actions.
The earlier gallery/DPI/IME/scroll tests remain in place. Release test assertions
stay enabled.

Browser screenshots establish the tested WebGL rendering, not physical iOS/Android
keyboard, clipboard or screen-reader certification. Engine-specific validation,
files, commands and undo remain host responsibilities. The demo file-picker callback
is labelled as an example and does not pretend to open a native OS dialog.
