# Review and changes (2026-10-20)

## Branding
- Replaced the text-only sidebar mark with the supplied Apex Labs logo. Light mode uses the original red-and-dark wordmark; dark mode uses a matching red-and-white version for legibility. The standalone builder embeds both logo files so exports remain self-contained.

## Weather impact
- Added a dedicated **Weather Impact** page for constructor-level analysis. It joins the supplied weather records to race results by season and round, compares each constructor's normalized race pace against precipitation and temperature, and shows wet/dry pace and DNF-rate benchmarks.
- Added `weather_impact.json` to the build output. Its pace estimate uses each driver's fastest-quarter median lap time and compares constructors within the same race, avoiding misleading raw lap-time comparisons between circuits.

## Data attribution
- Added a **Sources** page to the dashboard navigation. It credits the Formula 1 World Championship dataset by Rohan Rao and the Formula 1 Weather Info dataset by Mariya Kostyrya, links directly to both Kaggle pages, and documents the dashboard’s data-joining logic.

## Bugs fixed
- **Lap time formatting was wrong.** `time()` rounded to whole seconds and then appended the milliseconds, so 1:30.600 showed as 1:31.600. Replaced with `lapTime()`.
- **Fastest lap and qualifying times were always 0.** The build read `fastestLapTime.milliseconds` / `fastestLapSpeed.speed`, which aren't Kaggle columns, and ran `int()` on `"1:26.572"` strings. Added `lap_ms()` to parse them (it still reads the dotted columns if a different export has them).
- **Race Comparison pace bars were misleading.** The y-axis started at the smaller of the two values, so one race always drew a zero-height bar. Replaced with a per-driver pace strip plot with medians.
- **Overview filters.** The driver and constructor lists showed every entrant in history (about 860 drivers), and a filter left over from one race could empty the next. Filters now list only that race's entrants and reset when the race changes. Changing season now opens that season's latest race.
- **Latest race could be empty.** The default race is now the latest one that has results.
- **Season range.** A "From" later than "To" gave a blank page; the range is now swapped automatically.
- **Finish distribution** dropped finishes below 10th; added an `11+` bucket.
- **Pages were limited to 2000 onward.** The README and sidebar say 1950–2024; every page now covers all seasons.
- **CSV export** revoked the download URL immediately, which can cancel the download in Firefox.
- Charts with no data drew empty axes; every chart now shows an empty state.

## Requirements filled
- **Comparison of the same Grand Prix in two seasons** (README) now has its own mode.
- **Race Overview "pace level"** now has an average-lap-by-driver chart, plus fastest-lap and finisher KPIs.
- **Drivers "career performance measures"** now has eight KPIs, season points rank, and a fuller season table.
- **Circuits "participation"** is shown in the pace tooltip and race table, alongside the fastest-lap trend and wins from pole.
- **Works without a server or network.** D3 7.9.0 is vendored, the build writes `data.js`, and `build_standalone.py` produces a single-file HTML.

## Visual and UX
- Readable, commented `dashboard.js` (it was minified one-liners), with shared helpers for bar charts, tables, and legends.
- Legends on every multi-series chart, value labels on bars, animated transitions (off when the system asks for reduced motion), and tooltips that stay on screen.
- Sortable tables with sticky headers, podium badges, coloured position changes, and status chips.
- Race header strip with a weather chip, a segmented mode switch on Comparison, and "Full career" / "All years" reset buttons.
- Theme follows the system setting, has no flash on load, and uses an icon toggle. Includes focus rings, `aria-current` nav, a loading shimmer, and a setup card in place of the old error panels.
- Fixed horizontal overflow at phone width.

## New scripts
- `scripts/make_sample_data.py` generates synthetic data in the Kaggle CSV layout, for previews and testing.
- `scripts/build_standalone.py` bundles everything into one HTML file.
