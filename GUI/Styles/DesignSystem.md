# VPP Admin Tools — UI Design System

Reference tokens for the dark admin-tools theme (see concept: dark panels, orange section
headers, blue accents). DayZ `.layout` files have no variables, so **copy these exact values**
into layouts and scripts to keep every menu consistent.

Layout colors are `R G B A` floats (0-1). Script colors are `ARGB(a, r, g, b)` ints (0-255).

---

## 1. Color Palette

### Surfaces

| Token              | Hex     | Layout (`color R G B A`)      | Script `ARGB()`            | Usage |
|--------------------|---------|-------------------------------|----------------------------|-------|
| `bg-window`        | #0E1012 | `0.055 0.063 0.071 0.97`      | `ARGB(247, 14, 16, 18)`    | Window/root background |
| `bg-titlebar`      | #08090A | `0.031 0.035 0.039 1`         | `ARGB(255, 8, 9, 10)`      | Window title bar strip |
| `bg-panel`         | #16181B | `0.086 0.094 0.106 1`         | `ARGB(255, 22, 24, 27)`    | Column/panel surface |
| `bg-panel-header`  | #0A0B0C | `0.039 0.043 0.047 1`         | `ARGB(255, 10, 11, 12)`    | Panel header strips (PLAYERS, ACTIONS...) |
| `bg-row`           | #1B1E22 | `0.106 0.118 0.133 1`         | `ARGB(255, 27, 30, 34)`    | List rows, cards |
| `bg-input`         | #1E2126 | `0.118 0.129 0.149 1`         | `ARGB(255, 30, 33, 38)`    | Edit boxes, search fields |
| `bg-button`        | #22262B | `0.133 0.149 0.169 1`         | `ARGB(255, 34, 38, 43)`    | Default button fill |
| `bg-button-hover`  | #2C3138 | `0.173 0.192 0.220 1`         | `ARGB(255, 44, 49, 56)`    | Button hover fill |
| `border`           | #2A2E33 | `0.165 0.180 0.200 1`         | `ARGB(255, 42, 46, 51)`    | Hairline separators (1-2 px panels) |

### Accents

| Token              | Hex     | Layout (`color R G B A`)      | Script `ARGB()`            | Usage |
|--------------------|---------|-------------------------------|----------------------------|-------|
| `accent-orange`    | #E8A33D | `0.910 0.639 0.239 1`         | `ARGB(255, 232, 163, 61)`  | Section header text, active tab markers |
| `accent-blue`      | #3D7DD6 | `0.239 0.490 0.839 1`         | `ARGB(255, 61, 125, 214)`  | Vital underline bars, links, focus |
| `selection-blue`   | #2D5AA0 | `0.176 0.353 0.627 1`         | `ARGB(160, 45, 90, 160)`   | Selected list row highlight |
| `positive-green`   | #4CAF50 | `0.298 0.686 0.314 1`         | `ARGB(255, 76, 175, 80)`   | Online dots, success, self-name, good vitals |
| `warning-yellow`   | #D9B23D | `0.851 0.698 0.239 1`         | `ARGB(255, 217, 178, 61)`  | Mid vitals, warnings |
| `danger-red`       | #C24545 | `0.761 0.271 0.271 1`         | `ARGB(255, 194, 69, 69)`   | Low vitals, destructive accents |
| `danger-button`    | #5E1F1F | `0.369 0.122 0.122 1`         | `ARGB(255, 94, 31, 31)`    | Kick/Ban button fill |
| `accent-indigo`    | #4746B4 | `0.278 0.275 0.706 1`         | `ARGB(255, 71, 70, 180)`   | Context menu focused row |

### Text

| Token              | Hex     | Layout (`"text color" R G B A`) | Script `ARGB()`           | Usage |
|--------------------|---------|---------------------------------|---------------------------|-------|
| `text-primary`     | #FFFFFF | `1 1 1 1`                       | `ARGB(255, 255, 255, 255)`| Values, button labels, titles |
| `text-secondary`   | #9AA0A6 | `0.604 0.627 0.651 1`           | `ARGB(255, 154, 160, 166)`| Field labels, subtitles |
| `text-muted`       | #5C6166 | `0.361 0.380 0.400 1`           | `ARGB(255, 92, 97, 102)`  | Hints, disabled, placeholder |

### Vital threshold tints (script side)

Applied to vital icons/values by percentage of max:

| Range        | Token            | `ARGB()`                  |
|--------------|------------------|---------------------------|
| 75 - 100 %   | `positive-green` | `ARGB(255, 76, 175, 80)`  |
| 50 - 74 %    | `warning-yellow` | `ARGB(255, 217, 178, 61)` |
| 25 - 49 %    | `accent-orange`  | `ARGB(255, 232, 163, 61)` |
| 0 - 24 %     | `danger-red`     | `ARGB(255, 194, 69, 69)`  |

---

## 2. Typography

Font: vanilla SDF font **`gui/fonts/sdf_MetronLight72`** for *every* text widget
(the bible — established in `MenuPlayerManager.layout` and its child layouts).
Always pair it with `"exact text" 1` and an explicit `"exact text size"`;
bold roles add `"bold text" 1`. Never use the legacy InterDisplay fonts.

```text
font "gui/fonts/sdf_MetronLight72"
"exact text" 1
"exact text size" 14
"bold text" 1        <- bold roles only
```

| Role                   | Exact size | Bold | Notes |
|------------------------|-----------|------|-------|
| Window title           | 22        | yes  | UPPERCASE, `text-primary` |
| Window subtitle        | 14        | no   | `text-secondary` |
| Panel header           | 18        | yes  | UPPERCASE, `text-primary` |
| Section header / tab   | 14-16     | yes  | UPPERCASE, `accent-orange` (tabs: `text-primary` active / `text-secondary` inactive) |
| Body / field label     | 14        | no   | `text-secondary` for labels |
| Field value / row text | 14        | no   | `text-primary` |
| Button label           | 14-16     | yes  | `text-primary`; 16 for primary actions (SPAWN ITEM) |
| Emphasis value (names) | 16        | yes  | selected-item name, vital values (tinted) |
| Small print / counters | 14        | no   | `text-secondary` |
| GUID / classname small | 12        | no   | exception: stays small to avoid clipping |

---

## 3. Iconography

Custom Lucide-based atlas: `set:vpp_icons image:<name>` (see
`GUI/Textures/vpp_icons.imageset`, generated by `tools/build_icon_atlas.py`).

- Icons are rendered white; tint at runtime/layout with widget `color`.
- Standard sizes: 16 px inline (labels, rows), 20 px buttons, 24 px panel headers.
- `ImageWidgetClass` settings: `mode blend`, `"src alpha" 1`, `filter 1`, `"stretch mode" stretch_w_h`.

Icon usage map (Player Manager):

| Purpose                  | Sprite                |
|--------------------------|-----------------------|
| Player Manager / players | `users`, `user`       |
| Search                   | `search`              |
| Refresh                  | `refresh_cw`          |
| Close                    | `x`                   |
| Copy to clipboard        | `copy`                |
| Section chevrons         | `chevron_down`, `chevron_up` |
| Online dot               | `dot` (filled circle) |
| Health                   | `heart_pulse`         |
| Blood                    | `droplets`            |
| Shock                    | `zap`                 |
| Water                    | `glass_water`         |
| Energy                   | `flame`               |
| Heal                     | `heart`               |
| Kill                     | `skull`               |
| Send message             | `message_square`      |
| Spectate                 | `eye`                 |
| Invisible                | `eye_off`             |
| Teleport to player       | `arrow_up_from_line`  |
| Bring player             | `arrow_down_to_line`  |
| Return player            | `rotate_ccw`          |
| Freeze                   | `snowflake`           |
| Force vomit              | `activity`            |
| God mode                 | `shield`              |
| Unlimited ammo           | `infinity`            |
| Set scale                | `scaling`             |
| Kick                     | `log_out`             |
| Ban                      | `ban`                 |
| Weapon in hands          | `sword`               |
| Temperature              | `thermometer`         |
| Apply / confirm          | `check`               |
| Move / drag              | `move`                |
| Pin                      | `pin`                 |
| Gavel (admin action)     | `gavel`               |

Icon usage map (Item Manager):

| Purpose                  | Sprite                |
|--------------------------|-----------------------|
| Item Manager / generic item | `package`          |
| Add to preset            | `package_plus`        |
| Favorite toggle          | `star` (orange = on)  |
| Recents category         | `history`             |
| Spawn / run              | `play` (positive-green) |
| Quantity stepper         | `minus`, `plus`       |
| Save preset changes      | `save`                |
| New preset               | `plus`                |
| Delete preset / row      | `trash_2` (danger-red)|
| Preset parent badge      | `crown` (accent-orange) |
| Type: weapon             | `sword`               |
| Type: clothing           | `shirt`               |
| Type: food/drink         | `apple`               |
| Type: vehicle            | `car`                 |
| Type: structure          | `house`               |
| Type: AI/animal          | `paw_print`           |

Icon usage map (Teleport Manager):

| Purpose                  | Sprite                |
|--------------------------|-----------------------|
| Teleport Manager / saved location | `map_pin` (accent-orange) |
| World map panel / map header | `map`             |
| Add location             | `map_pin_plus`        |
| Locate (center map on row)| `locate_fixed`       |
| Teleport-to (row + primary)| `crosshair` / `locate_fixed` (positive-green) |
| Edit location            | `square_pen`          |
| Delete location / recents | `trash_2` (danger-red)|
| Copy cursor coords       | `copy`                |
| Target = self            | `user`                |
| Target = selected players| `users`               |

Icon usage map (ESP Tools):

| Purpose                          | Sprite                |
|----------------------------------|-----------------------|
| ESP Tools / window               | `eye`                 |
| Scan toggle (reflects on/off)    | `eye` (on) / `eye_off` (off) |
| Filters section                  | `crosshair`           |
| Selected-items section           | `package`             |
| Controls section                 | `scaling`             |
| Add filter                       | `plus` (positive-green) |
| Restore default filters          | `rotate_ccw`          |
| Refresh (clear & rescan)         | `refresh_cw`          |
| Per-filter color swatch picker   | `chevron_down`        |
| Bring item to me                 | `arrow_down_to_line` (positive-green) |
| Return item to previous position | `rotate_ccw`          |
| Remove item from list            | `x`                   |
| Delete (filter / item / all / radius) | `trash_2` (danger-red) |
| Tooltip / info                   | `info`                |
| Close                            | `x`                   |

---

## 4. Widget Styles (`vpp_widgets.styles`)

Custom widget chrome, replacing vanilla `Editor` / `editor_quad_dark` / `Default` styles.
Defined in `GUI/Styles/vpp_widgets.styles` (registered via `class widgetStyles` in
`assets/config.cpp`), drawing pre-colored sprites from the `vpp_ui` imageset
(generated by `tools/build_ui_atlas.py`). Because the sprites carry the design-token
colors, widgets using these styles take `color 1 1 1 1` — no per-widget tinting.
Tinting still multiplies, so `VPPFill` (pure white) is the style to use when a script
or layout must drive the color.

| Style            | Widget type      | States covered                              | Use for |
|------------------|------------------|---------------------------------------------|---------|
| `VPPButton`      | ButtonWidget     | Normal / Highlight / Pushed / Focus / Disabled | Standard action buttons |
| `VPPButtonDanger`| ButtonWidget     | same                                        | Destructive actions (Kick, Ban) |
| `VPPButtonGhost` | ButtonWidget     | transparent Normal, hover/pushed fill       | Close, refresh, section headers |
| `VPPInput`       | EditBoxWidget    | Normal / Focus (orange border) / Disabled   | Standalone edit boxes |
| `VPPInputBare`   | EditBoxWidget    | all empty                                   | Edit boxes inside a `VPPInputPanel` |
| `VPPInputPanel`  | PanelWidget      | input chrome on a panel                     | Search field wrapper (icon + editbox) |
| `VPPCheckBox`    | CheckBoxWidget   | Normal / Highlight / Mark / Disabled        | All checkboxes |
| `VPPSlider`      | SliderWidget     | Normal / Focus / Disabled, blue fill bar    | Modifier sliders |
| `VPPPanel`       | PanelWidget      | bordered `bg-panel` fill                    | Window + column panels |
| `VPPHeader`      | PanelWidget      | flat `bg-row` fill                          | Cards/strips at #1B1E22 |
| `VPPCard`        | PanelWidget      | flat `bg-input`-dark fill                   | Recessed wells |
| `VPPFill`        | PanelWidget      | flat white fill (tint with `color`)         | Script-colored vital bars (too small to round) |
| `VPPFillRound`   | PanelWidget      | rounded white fill (tint with `color`)      | Header strips, cards, player rows |
| `VPPScroll`      | ScrollWidget     | invisible chrome                            | Scroll areas (like vanilla `blank`) |
| `VPPListbox`     | TextListboxWidget| Normal / Focus / Disabled                   | Item catalog: `vpp_input` 9-slice frame, `vpp_select` row highlight, slim `vpp_scroll_*` scrollbar, empty separators/headers |

> **Listbox font exception**: `TextListboxWidget` ignores `"exact text"` scaling and renders the
> style font at native size. `VPPListbox` therefore uses fixed-size `gui/fonts/MetronLight16`
> instead of the SDF-72 bible font. Never put `font` / `"exact text"` props on a listbox in layouts.

9-slice sprite geometry mirrors vanilla `menuButton*`: 2x2 px corners, 1 px edges,
1x1 center (`vpp_btn_tl`, `vpp_btn_t`, ... `vpp_btn_br`). Patch families:
`vpp_btn`, `vpp_btn_hover`, `vpp_btn_push`, `vpp_btn_danger`, `vpp_btn_danger_hover`,
`vpp_input`, `vpp_input_focus`, `vpp_panel`. Flat swatches: `vpp_fill_header`,
`vpp_fill_track`, `vpp_fill_accent`, `vpp_fill_white`, `vpp_fill_transparent`.
Checkboxes: `vpp_check_off`, `vpp_check_off_hover`, `vpp_check_on`, `vpp_check_disabled` (26x26).

To change the look of every button/input/panel at once: edit the color constants in
`tools/build_ui_atlas.py`, re-run it, and re-convert `vpp_ui.png` to `vpp_ui.edds`.

---

## 5. Layout Metrics

| Token                | Value      | Usage |
|----------------------|------------|-------|
| Title bar height     | 36 px      | exact size (`vexactsize 1`, `size 1 36`) |
| Panel header height  | 28 px      | exact size |
| Section header height| 24 px      | exact size |
| Row height           | 26 px      | player list / preset library / preset contents rows |
| Two-line row height  | 40 px      | teleport location rows (name over coords) on a `VPPFillRound` row bg |
| Button height        | 28 px      | action buttons |
| Icon button          | 26 x 26 px | per-row actions (favorite, add-to-preset, spawn, delete, crown) with 16 px icon |
| Option row height    | 30 px      | spawn option rows (label + control: stepper, dropdown, checkbox) |
| Dropdown selector    | 24 px high | `VPPDropDownMenu` host panel; 26 px element rows, 160 px max list |
| Primary action button| 34 px high | full-width SPAWN ITEM style buttons |
| Panel gap            | 8 px       | between columns/panels |
| Panel padding        | 8-10 px    | inner content inset |

## 6. Conventions

- Structure panels as: header strip (`bg-panel-header`) + body (`bg-panel`).
- Prefer `VPP*` widget styles over vanilla ones; only fall back to `blank`/`Colorable`
  for invisible structural widgets.
- Destructive actions (Kick, Ban) use `VPPButtonDanger` with `text-primary`.
- Disabled buttons: engine dims them; avoid manual alpha fighting `Enable()`.
- Prefab layouts for new UI live in `GUI/Layouts/UIHelpers/DesignSystem/`.
- Collapsible sections use `VPPCollapsibleSection` (5_Mission `GUI/UIHelpers`).
- Multi-state images: a second `image1` declared in a `.layout` does **not** register --
  load both slots from script with `LoadImageFile(0/1, "set:vpp_icons image:...")`, then
  switch with `SetImage()`. Layouts carry only `image0` for the initial look
  (see `VPPCollapsibleSection` chevrons).
- Avoid `color 1 0 0 1` debug markers on root frames; roots should be `0 0 0 0` or a token.
- **Dropdowns**: one system everywhere — `VPPDropDownMenu` (5_Mission `GUI/UIHelpers/DropDownMenu.c`,
  prefabs in `GUI/Layouts/VPPDropdown/`). Host it in a chrome-less `PanelWidget` placed *last*
  among its siblings so the expanded list draws on top. No `XComboBoxWidget` in new UI.
  **The host panel and every small ancestor (row frames) MUST set `clipchildren 0`** —
  otherwise the 160px expanded list is clipped to the 24-30px host/row bounds and never shows.
- **Tabs**: pairs of `VPPButton`s in the panel header; the script swaps two root frames and
  tints labels `text-primary` (active) / `text-secondary` (inactive) — see `SetPanelTab`
  in MenuPlayerManager/MenuItemManager.
- **Segmented toggle**: a mutually-exclusive choice rendered as two adjacent `VPPButton`s
  (e.g. Self | Selected target toggle in the Teleport Manager). The script tints the active
  side's label + icon `accent-orange` and the inactive side `text-secondary` (`SetTarget`).
  Use instead of a lone checkbox when both states deserve an explicit, labelled button.
- **Always-on map HUD**: hint/coords strips floated over a `MapWidget` MUST set
  `ignorepointer 1` (and live on a `VPPFillRound` panel pinned with `valign bottom_ref`) so
  they never swallow map clicks/drags. Map gestures stay on the `MapWidget`
  (double-click = teleport, Ctrl+double-click = add, right-click = copy coords).
- **Catalog category filter**: reuse the single `VPPDropDownMenu` for list scoping
  (All / Towns / Custom / Recent in Teleport, item categories in Item Manager). Switching
  category rebuilds the list source; the search box then toggles per-row visibility on top.
- **Local client prefs**: per-player history/favorites live in `$profile:VPPAdminTools/*ClientPrefs.json`
  (`ItemManagerClientPrefs`, `TeleportManagerClientPrefs`) — JSON via `JsonFileLoader`, capped &
  deduped newest-first, never sent to the server. List rows built from prefs use a read-only row
  "mode" (no multi-select/edit; Delete = remove-from-history).
- **Quantity stepper**: `[-] [value] [+] [MAX]` — 26x26 `VPPButton`s around a `VPPInput`;
  "MAX"/empty resolves to the item's maximum at spawn time.
- **Preview card**: `ItemPreviewWidget` inside a `VPPCard` well with a corner `VPPCheckBox`
  toggle; drag-rotate via `GetDragQueue()`.
- **Explicit row actions** replace hidden mouse gestures (right-click/Ctrl gestures):
  every list row carries its own 26x26 icon buttons (spawn / delete / make-parent / favorite).
- **EditBox `"Use default text" 1`** turns the `text` value into a placeholder hint:
  `GetText()` returns `""` until the user types. Only use it for hint text (search fields),
  never on inputs that scripts read/write (e.g. the quantity stepper).
- **Confirmation dialogs** (`GetVPPUIManager().HookConfirmationDialog(...)`): the *target*
  button MUST declare `scriptclass "ConfirmationEventHandler"` in its `.layout`. The hook
  does `target.GetScript(confirmClass)` — with no `scriptclass`, the handler is null and the
  click silently does nothing. The dialog parents to a `FindAnyWidget("Main")` (fallback
  `PanelConfirmationBox`) on the menu root, so the menu layout needs a `Main` panel.
  YES/NO callbacks use `void Func(int result)`; input dialogs (`DIAG_OK_CANCEL_INPUT`,
  `allowchars = true`) use `void Func(int result, string input)`.
- **Dialog box chrome** (`GUI/Layouts/UIHelpers/VPPDialogBox.layout`): `VPPPanel` window,
  rounded `bg-titlebar` `VPPFillRound` title strip (title 18 bold), `bg-input` `VPPInput`
  field (16), affirmative buttons (`ButtonYES`/`ButtonOK`) = `VPPButton`, dismiss buttons
  (`ButtonNO`/`ButtonCANCEL`/`ButtonPaste`) = `VPPButtonGhost`, labels 16 bold. Never rename
  the script-bound widgets (`TitleText`, `ContentText`, `InputBox`, `BorderOutline`, the five
  `Button*`) or drop their scriptclasses.

### Context menu

The right-click context action menu (`5_Mission/.../GUI/UIHelpers/ContextMenu/`, layouts in
`GUI/Layouts/UIHelpers/ContextMenu/`). Metrics live in `VPPContextMenuStyle`.

- **Metrics**: width 280, header strip 26, rows 30 with a 2 px gap, separators 9, footer 20,
  4 px padding, at most 14 visible items (more scroll in a window with a 3 px thumb).
  Created once at workspace root with `SetSort(1024)` - above the admin HUD, Stats HUD and
  notifications, and never inside a scroller.
- **Focus** = `accent-indigo` row background; idle rows `bg-row`; danger rows tint label + icon
  `danger-red` while not focused.
- **Confirm-armed** = `danger-red` row background. The label is KEPT (the admin always sees which
  action is armed) and `#VSTR_CTX_CONFIRM_AGAIN` ("Click again") shows in the hint slot.
- **Toggle rows** use the `vpp_ui` check sprites (`vpp_check_on` / `vpp_check_off`).
- **Chevrons** are `chevron_down` rotated with `SetRotation(0, 0, +-90)` (sign constants in
  `VPPContextMenuStyle`, flip if wrong in game).
- **Drill-down pages**: a submenu replaces the panel content and gets a Back row; no cascading flyouts.
- **Clip frames**: labels and hints sit in `clipchildren 1` frames; hints are truncated to
  18 characters (16 + ".."), so long translations never overlap.
- **DELIBERATE DEVIATION from the reference image**: the header strip shows the target name and
  icon. It is needed in look-mode (crosshair, no cursor) to know what the menu acts on. Do not remove it.
- **Third-party rules** (full list A1-A9 in the context menu contract / `4_World/.../ContextMenu/Examples`):
  1. Actions are stateless singletons; register server-logic actions from 4_World so client and server both know the id (unknown ids fail as INVALID).
  2. Provider rows should reference a REGISTERED action (`GetVPPContextActionManager().GetAction(id)`); a row runs server-side only if `IsTargetType` holds for the server-resolved target.
  3. Never `AddRPC` on `RPC_VPPContextMenu` / `RPC_VPPContextMenuClient` (CF AddRPC is last-wins); use your own namespace. `CallbackInst` is weak and must outlive the open menu.
  4. Guard integration code with `#ifdef VPPADMINTOOLS` and add `requiredAddons "DZM_VPPAdminTools"`.
  5. New permissions are not granted to existing groups (admins grant them in the Permissions Editor); if `AutoRegisterPermission()` returns false, register the string yourself with `AddPermissionType`.

### XML Editor

The reworked XML Editor (`5_Mission/.../SubMenus/MenuXMLEditor/`, layouts in `GUI/Layouts/XMLEditorUI/`,
contract in the XML Editor INTERFACES v2.1). It follows the content-dominant layout: a slim rail on the left,
one big surface on the right, and the controls in narrow strips.

- **Tab-host pattern**: `MenuXMLEditor.layout` holds four empty frames (`TabHostTypes`, `TabHostMap`,
  `TabHostBackups`, `TabHostFiles`). Every tab is its own layout (`VPPATUIConstants.XMLEditor*Tab`), created
  into its host by a `ScriptedWidgetEventHandler` class that takes `(Widget host, MenuXMLEditor owner)`.
  A tab sets its root handler to itself, returns `false` from `OnMouseButtonDown` (so the window can
  still be dragged), registers its own server-to-client receivers, and exposes `Show`, `OnResize`,
  `OnUpdate(float)`, `OnSessionChanged` and `Refresh`. The shell only forwards. The header `BtnRefresh`
  calls `Refresh()` on the active tab. Tabs are sized to their translated labels, and the rail shows on
  TYPES and MAP only.
- **Chips** (`XMLEditorChip.layout`: `ChipRoot` ButtonWidget > `ChipFill` + `ChipText`; control
  `VPPXEChipGroup`, laid out in a WrapSpacer): used for category, usage, value and tag, plus the bulk
  editor's tri-state lists. There are three modes: radio (exactly one ON), multi (ON/OFF) and tri-state
  (keep, ADD, REMOVE). The width follows the text: `Math.Clamp(20 + 7 * label.Length(), 56, 220)` px.

  | State | Fill `ARGB()` | Text |
  |-------|---------------|------|
  | OFF (idle) | `bg-row` `ARGB(255, 27, 30, 34)` | `text-secondary` |
  | ON (set in the edited definition) | `accent-orange` `ARGB(255, 232, 163, 61)` | `text-primary` |
  | MIXED (inherited from an earlier definition, shown but not set here) | `accent-blue` at alpha 150 `ARGB(150, 61, 125, 214)` | `text-primary` |
  | ADD (bulk) | `positive-green` | `text-primary` |
  | REMOVE (bulk) | `danger-red` | `text-primary` |
  | disabled | unchanged fill | `text-muted` |

  Clicking an inherited (MIXED) chip turns it off and writes the inherited list minus that name into the
  edited definition.
- **Numeric inputs** (`Input{F}`, scriptclass `VPPXENumericEditHandler`, never `"Use default text"`):
  the box accepts empty text, a lone `-` while typing, and an optional minus followed by digits inside
  `SetRange`. Anything else restores the last valid text, and the box never resets to 0. That matters
  because `quantmin`/`quantmax` -1 must be typeable, and the shared EditBox handler rejects `-`. The mouse
  wheel steps by `SetStep`, and leaving the box clears focus. Steppers are `[-] [value] [+]`, using 26 px
  `minus` / `plus` buttons.
- **Map canvas overlay** (`VPPXEMapTab` + `VPPXEMapRenderer`):
  - Structure: a `CanvasWidget` (`XeMapCanvas`) is a later sibling of the `MapWidget` (`XeMapWidget`)
    with `ignorepointer 1` and `priority 101` (the vanilla `day_z_map.layout` tools container
    precedent). The HUD strip, busy and notice overlays are `ignorepointer 1` as well.
  - Coordinates: the canvas draws in its own local pixels. Every redraw calibrates world-to-canvas from
    two `MapToScreen` samples and detects whether the answer is absolute or widget-local.
  - Levels: `SetData` precomputes 100 m, 500 m and 2 km aggregates per cell layer. A redraw picks the
    finest level whose cell is at least 6 px on screen, then rebins to 12 px screen bins at that level
    only.
  - Throttle: the view is polled every frame, but it redraws at most every 100 ms while the view changes,
    plus one final redraw 150 ms after it settles. A redraw draws at most 6000 lines (heat stops at 4000
    so markers and circles still fit).
  - Styling: the heat ramp is `accent-blue`, `warning-yellow`, `accent-orange` and `danger-red` at
    alpha 140; clusters use a green ramp. Markers are 6 px squares, drawn only when fewer than 600 are
    visible. Infected zones are 32-segment circles.
  - Pins fallback: `BtnRenderMode` (`flame`) switches to `AddUserMark` pins for the top 300 candidates.
  - Gestures: click shows the hit in the HUD, and double-click teleports.
  - Dialogs: the map and the canvas are hidden while any dialog is open (a map draws over popups).
- **OpenConfirm with a weak dialog handle**: every confirmation goes through `MenuXMLEditor.OpenConfirm(title,
  body, diagType, cbInst, cbFunc, allowChars)`.
  - The shell keeps a WEAK `VPPDialogBox` reference, hides the map, and forwards the result as
    `void cbFunc(int result, string input)`. `VPPDialogBox` calls back with one int for YES/NO/CANCEL
    and with `Param2` for input OK, so the forwarder always uses `CallFunctionParams` with a `Param2`.
  - `IsDialogOpen()` means the handle is not null.
  - `OnMenuShow`, `ShowTab` and `OnUpdate` restore the map when the handle became null without a
    callback (the dialog was closed some other way).
  - The dialog buttons are literally `YES`/`NO`, so translated bodies name them unchanged.
- **Dropdown z-order rule** (extends the Dropdowns convention above): `VPPDropDownMenu` creates its popup
  INSIDE the host (`DropDownMenu.c:20`), and widgets draw in tree order. So every later sibling of the
  host, or of any of its ancestors, draws over the open popup.
  1. The hosts of one parent form the trailing sibling group. A host whose popup can cover another host
     comes after it (`FilterHost` before `FileScopeHost`).
  2. From the host's parent up to (not including) its drop container, every widget is the LAST child of
     its own parent. The drop container is the smallest ancestor whose rect holds the whole open popup.
  3. `clipchildren 0` is set on the host and on every widget of that chain.

  Drop containers: `FileScopeHost`/`FilterHost` use `PanelRail`, `DefSelectHost`/`MoveTargetHost` use
  `InspectorRoot`, `DrawerModeHost` uses `MapDrawer`, `BackupsFileHost` uses `BackupsLeft`, and
  `IssuesSeverityHost` uses `FilesRight`. A strip that must sit at the TOP but hold a host is placed by
  offset and kept last in the tree (`InspHeader` is the last child of `InspectorRoot`, `IssuesToolbar`
  the last child of `FilesRight`). `tools/verify_xml_editor.py` check 9 enforces all three parts.
- **Width budgets** (checked at 1280x720, 1366x768 and 1920x1080; 7 px per character at 12 px bold):
  - Window and rail: the window is 0.70 of the screen width (896 / 956 / 1344 px). The rail is 0.28
    (251 / 268 / 376 px). The content next to the rail is about 645 / 688 / 968 px.
  - Rail footer (56 px): `TxtRowCount` sits on its own 18 px line (`ROWCOUNT` / `ROWS_CAPPED`, at most
    16 characters without the `%N` placeholders), above `BtnAddType` and `BtnBulkEdit`. Those two are
    sized to their text: `Math.Clamp(36 + 7 * Tr(label).Length(), 56, 122)` px. `BTN_ADD` and
    `BTN_BULK` are at most 12 characters.
  - Map toolbar row 1 (30 px): `TxtMapType` 0.34, five 44 px tier chips, three 26 px ghost buttons and an
    info icon. `TIER_*` labels are at most 4 characters (T1..T4, U = unique).
  - Map toolbar row 2 (28 px): 7 layer chips at 1/7 of the width. A chip shows its count only when name
    plus count fit. `LAYER_*` labels are at most 12 characters.
  - Map drawer (0.30 of the map area): `DrawerActions` is a 3-row grid of 26 px rows with 4 px gaps:
    TELEPORT | NEXT, then SCAN LIVE (full width), then DELETE | DELETE ALL. A half-width button hides
    its icon when `22 + 7 * label length` exceeds its width. `MAP_TP`, `MAP_NEXT`, `MAP_DEL_SEL` and
    `MAP_DEL_ALL` are at most 10 characters, and `MAP_LIVESCAN` at most 18.
  - Inspector header (44 px): the 40 px preview card, then the name (18 bold) over `Editing in <file>`
    (12). On the right sit `DefSelectHost` (180 x 24 exact; `MoveTargetHost` shares the rect) and four
    26 px icon buttons. Dropdown entries are fitted into 21 characters, keeping the status word.
  - Backups list: the When column shows `MM-DD HH:MM`, and the full stamp stays in `TxtBkMeta`.

  `tools/_append_xmle_strings.py` and verifier check 7 enforce every cap in all 13 languages.
- **Icon map (XML Editor)**:

  | Purpose | Sprite |
  |---------|--------|
  | Window / TYPES tab / empty inspector | `file_code` |
  | MAP tab | `map` |
  | BACKUPS tab / backups header | `history` |
  | FILES tab / files header | `file_text` |
  | Rail toggle, rail header, drawer toggle, diff header | `list` |
  | Search | `search` |
  | Help and info icons (tooltips only here) | `info` |
  | Refresh (header, map, issues) | `refresh_cw` |
  | Close | `x` |
  | Add type, stepper + | `plus` |
  | Stepper - | `minus` |
  | Bulk edit | `boxes` |
  | Duplicate / move / rename / delete type | `copy` / `move` / `square_pen` / `trash_2` (danger-red) |
  | Revert type / restore backup | `rotate_ccw` |
  | Save | `save` |
  | Section chevrons | `chevron_up` / `chevron_down` |
  | Render mode heat / pins | `flame` / `map_pin` |
  | Teleport to selection | `locate_fixed` |
  | Next instance | `crosshair` |
  | Live scan | `activity` |
  | Delete live selection / delete all live | `trash_2` / `package_x` |
  | Snapshot / pin backup | `folder_plus` / `pin` |
