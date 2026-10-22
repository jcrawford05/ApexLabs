/* global d3 */
(() => {
  "use strict";

  const DATA_FILES = ["metadata", "results", "race_metrics", "driver_seasons", "constructor_seasons", "circuit_summary", "qualifying", "sprint_results", "weather_impact"];
  const PAGES = {
    overview: { href: "index.html", icon: "▣", label: "Race Overview", title: "Race Overview" },
    comparison: { href: "comparison.html", icon: "⇄", label: "Race Comparison", title: "Race Comparison" },
    drivers: { href: "drivers.html", icon: "◉", label: "Drivers", title: "Drivers" },
    circuits: { href: "circuits.html", icon: "⌁", label: "Circuits", title: "Circuits" },
    constructors: { href: "constructors.html", icon: "▰", label: "Constructors", title: "Constructors" },
    weather: { href: "weather.html", icon: "☂", label: "Weather Impact", title: "Weather Impact" },
    sources: { href: "sources.html", icon: "◌", label: "Sources", title: "Data Sources" },
  };
  // The standalone builder replaces these paths with embedded image data.
  const BRAND_LOGOS = window.APEX_BRAND_LOGOS || {
    light: "assets/images/apex-labs-logo-light.png",
    dark: "assets/images/apex-labs-logo-dark.png",
  };
  // The single-file build (scripts/build_standalone.py) ships every page as a <template>.
  const standalone = Boolean(document.querySelector("template[data-page-template]"));
  const state = { data: null, page: document.body.dataset.page, rerender: null, tooltip: null };
  const reducedMotion = window.matchMedia?.("(prefers-reduced-motion: reduce)").matches;
  const ms = (duration) => (reducedMotion ? 0 : duration);

  /* ---------- Formatting ---------- */
  const $ = (selector, root = document) => root.querySelector(selector);
  const byId = (items, key) => new Map(items.map((item) => [item[key], item]));
  const sum = (items, accessor) => d3.sum(items, accessor);
  const fmtNumber = d3.format(",.0f");
  const fmtOne = d3.format(",.1f");
  const fmtPct = (value) => (Number.isFinite(value) ? `${fmtOne(value)}%` : "—");
  const escape = (value) => String(value ?? "").replace(/[&<>'"]/g, (char) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", "'": "&#39;", "\"": "&quot;" })[char]);

  function lapTime(ms) {
    if (!Number.isFinite(ms) || ms < 1) return "—";
    const rounded = Math.round(ms);
    const minutes = Math.floor(rounded / 60000);
    const seconds = Math.floor((rounded % 60000) / 1000);
    return `${minutes}:${String(seconds).padStart(2, "0")}.${String(rounded % 1000).padStart(3, "0")}`;
  }
  function gapSeconds(ms) {
    if (!Number.isFinite(ms)) return "—";
    return `${ms > 0 ? "+" : ms < 0 ? "−" : "±"}${(Math.abs(ms) / 1000).toFixed(3)}s`;
  }
  function signed(value, format = fmtNumber) {
    if (!Number.isFinite(value)) return "—";
    return `${value > 0 ? "+" : value < 0 ? "−" : "±"}${format(Math.abs(value))}`;
  }
  const driverName = (id) => state.data.drivers.get(id)?.name || "Unknown driver";
  const teamName = (id) => state.data.constructors.get(id)?.name || "Unknown constructor";
  const raceLabel = (race) => `${race.year} · ${race.name}`;
  const metric = (raceId) => state.data.metrics.get(raceId) || {};
  const resultsFor = (raceId) => state.data.resultsByRace.get(raceId) || [];
  const winnerName = (raceId) => { const winner = state.data.winnerByRace.get(raceId); return winner ? driverName(winner.driverId) : "—"; };

  function weatherChip(weather) {
    if (!weather?.available) return `<span class="chip">Weather not loaded</span>`;
    const wet = weather.condition === "Wet";
    return `<span class="chip ${wet ? "wet" : "dry"}">${wet ? "☂" : "☀"} ${escape(weather.condition)} · ${fmtOne(weather.temperatureC)}°C</span>`;
  }

  /* ---------- Shared UI ---------- */
  function empty(target, message = "No records match the selected filters.") {
    target.innerHTML = `<div class="empty-state"><div><strong>Nothing to display</strong>${message}</div></div>`;
  }
  function tooltip() {
    if (!state.tooltip) {
      state.tooltip = document.createElement("div");
      state.tooltip.className = "tooltip";
      state.tooltip.setAttribute("role", "status");
      document.body.appendChild(state.tooltip);
    }
    return state.tooltip;
  }
  function hideTooltip() { if (state.tooltip) state.tooltip.style.display = "none"; }
  function hover(selection, text) {
    selection
      .on("mousemove", (event, datum) => {
        const tip = tooltip();
        tip.innerHTML = text(datum);
        tip.style.display = "block";
        // Keep the tooltip on screen near the right and bottom edges.
        const { width, height } = tip.getBoundingClientRect();
        const left = event.clientX + 14 + width > window.innerWidth ? event.clientX - width - 14 : event.clientX + 14;
        const top = event.clientY + 14 + height > window.innerHeight ? event.clientY - height - 14 : event.clientY + 14;
        tip.style.left = `${Math.max(4, left)}px`;
        tip.style.top = `${Math.max(4, top)}px`;
      })
      .on("mouseleave", hideTooltip);
  }
  function svgFor(target, height = 270, margin = { top: 14, right: 18, bottom: 38, left: 48 }) {
    target.innerHTML = "";
    const width = Math.max(target.clientWidth || 420, 300);
    const root = d3.select(target).append("svg").attr("viewBox", `0 0 ${width} ${height}`).attr("role", "img");
    const svg = root.append("g").attr("transform", `translate(${margin.left},${margin.top})`);
    return { root, svg, width: width - margin.left - margin.right, height: height - margin.top - margin.bottom, margin };
  }
  function drawAxis(group, axis, className = "") {
    group.attr("class", `axis ${className}`).call(axis);
    return group;
  }
  function animateWidth(selection, width) {
    selection.attr("width", 0).transition().duration(ms(450)).ease(d3.easeCubicOut).attr("width", width);
  }
  function animateHeight(selection, y, height, baseline) {
    selection.attr("y", baseline).attr("height", 0).transition().duration(ms(450)).ease(d3.easeCubicOut).attr("y", y).attr("height", height);
  }
  function legend(name, items) {
    const target = $(`[data-legend="${name}"]`);
    if (target) target.innerHTML = items.map(([cls, label]) => `<span><i class="${cls}"></i>${escape(label)}</span>`).join("");
  }
  function kpis(target, entries) {
    target.innerHTML = entries.map((entry) => `<article class="kpi"><span class="kpi-label">${escape(entry.label)}</span><strong class="kpi-value ${entry.html ? "pair" : ""}" title="${escape(entry.title ?? entry.value)}">${entry.html ?? escape(entry.value)}</strong><span class="kpi-detail ${entry.tone || ""}" title="${escape(entry.detail || "")}">${escape(entry.detail || "")}</span></article>`).join("");
  }
  function selectMarkup(id, label, options, selected, wide = false) {
    const markup = options.map(([value, text]) => `<option value="${escape(value)}" ${String(value) === String(selected) ? "selected" : ""}>${escape(text)}</option>`).join("");
    return `<div class="filter-field ${wide ? "wide" : ""}"><label for="${id}">${label}</label><select id="${id}">${markup}</select></div>`;
  }
  function onChange(id, handler) { $(`#${id}`).addEventListener("change", (event) => handler(event.target.value)); }
  function orderRange(filters) {
    if (+filters.start > +filters.end) [filters.start, filters.end] = [filters.end, filters.start];
  }

  /** Sortable table. columns: { key, label, numeric, value(row), html(row) }. */
  function dataTable(target, columns, rows, initialSort) {
    const sort = target.__sort || initialSort;
    target.__sort = sort;
    const column = columns.find((item) => item.key === sort.key) || columns[0];
    const value = column.value || ((row) => row[column.key]);
    const sorted = [...rows].sort((a, b) => {
      const x = value(a), y = value(b);
      if (x == null || x === "") return 1;
      if (y == null || y === "") return -1;
      return (typeof x === "string" ? x.localeCompare(y) : x - y) * (sort.dir === "asc" ? 1 : -1);
    });
    const head = columns.map((item) => {
      const active = item.key === sort.key;
      const aria = active ? ` aria-sort="${sort.dir === "asc" ? "ascending" : "descending"}"` : "";
      return `<th class="${item.numeric ? "numeric" : ""}"${aria}><button type="button" data-sort="${item.key}">${escape(item.label)}<span class="sort-icon">${active && sort.dir === "asc" ? "▲" : "▼"}</span></button></th>`;
    }).join("");
    const body = sorted.length
      ? sorted.map((row) => `<tr>${columns.map((item) => `<td class="${item.numeric ? "numeric" : ""} ${item.strong ? "strong" : ""}">${item.html ? item.html(row) : escape((item.value || ((r) => r[item.key]))(row) ?? "—")}</td>`).join("")}</tr>`).join("")
      : `<tr><td colspan="${columns.length}"><div class="empty-state">No rows match the selected filters.</div></td></tr>`;
    target.innerHTML = `<thead><tr>${head}</tr></thead><tbody>${body}</tbody>`;
    target.querySelectorAll("[data-sort]").forEach((button) => button.addEventListener("click", () => {
      const key = button.dataset.sort;
      const numeric = columns.find((item) => item.key === key)?.numeric;
      target.__sort = { key, dir: key === sort.key ? (sort.dir === "asc" ? "desc" : "asc") : (numeric ? "desc" : "asc") };
      dataTable(target, columns, rows, initialSort);
    }));
  }
  function positionBadge(position) {
    if (!position) return `<span class="pos-badge nc">NC</span>`;
    return `<span class="pos-badge ${position <= 3 ? `p${position}` : ""}">${position}</span>`;
  }
  function statusChip(status) {
    const finished = status === "Finished" || /^\+\d+ Laps?$/.test(status || "");
    return `<span class="status-chip ${finished ? "finished" : "out"}">${escape(status)}</span>`;
  }

  /* ---------- Shell, theme and routing ---------- */
  function pageHref(key) { return standalone ? `#${key}` : PAGES[key].href; }
  function renderNav() {
    const target = $("[data-shell-nav]");
    const seasons = state.data?.seasons;
    const span = seasons?.length ? `${seasons[0]}–${seasons.at(-1)}` : "1950–2024";
    target.innerHTML = `<a class="brand" href="${pageHref("overview")}" aria-label="Apex Labs F1 Analytics home"><span class="brand-logo-wrap"><img class="brand-logo brand-logo-light" src="${BRAND_LOGOS.light}" alt="Apex Labs"><img class="brand-logo brand-logo-dark" src="${BRAND_LOGOS.dark}" alt=""></span><span class="brand-mark brand-mark-compact" aria-hidden="true">A</span></a>
      <nav class="nav-links" aria-label="Dashboard pages">${Object.entries(PAGES).map(([key, item]) => `<a class="nav-link ${key === state.page ? "active" : ""}" href="${pageHref(key)}" ${key === state.page ? "aria-current=\"page\"" : ""} title="${item.label}"><span class="nav-icon" aria-hidden="true">${item.icon}</span><span>${item.label}</span></a>`).join("")}</nav>
      <p class="sidebar-foot">Formula 1 historical intelligence<br><b>${span}</b>${state.data ? ` · ${fmtNumber(state.data.races.length)} races` : ""}</p>`;
  }
  function setupTheme() {
    if (!document.documentElement.dataset.theme) document.documentElement.dataset.theme = "dark";
    document.addEventListener("click", (event) => {
      if (!event.target.closest("[data-theme-toggle]")) return;
      const next = document.documentElement.dataset.theme === "dark" ? "light" : "dark";
      document.documentElement.dataset.theme = next;
      try { localStorage.setItem("apex-theme", next); } catch (error) { /* storage can be unavailable on file:// */ }
    });
  }
  function demoBanner() {
    if (!state.data?.demo || $(".demo-banner")) return;
    const banner = document.createElement("div");
    banner.className = "demo-banner";
    banner.innerHTML = `<b>Demo data.</b> These drivers, teams and results are synthetic, generated by <code>scripts/make_sample_data.py</code>. Build with the Kaggle CSVs to see real history.`;
    $("[data-page-root]").prepend(banner);
  }
  function mount(page) {
    state.page = PAGES[page] ? page : "overview";
    document.body.dataset.page = state.page;
    if (standalone) {
      const template = $(`template[data-page-template="${state.page}"]`);
      $("[data-page-root]").replaceChildren(template.content.cloneNode(true));
      document.title = `Apex Labs | ${PAGES[state.page].title}`;
      window.scrollTo(0, 0);
    }
    renderNav();
    hideTooltip();
    state.rerender = null;
    if (!state.data) return;
    demoBanner();
    ({ overview, comparison, drivers, circuits, constructors, weather })[state.page]?.();
  }
  // Charts are sized from their container, so redraw when the width changes (height-only resizes are ignored).
  let resizeTimer, lastWidth = window.innerWidth;
  window.addEventListener("resize", () => {
    if (window.innerWidth === lastWidth) return;
    lastWidth = window.innerWidth;
    clearTimeout(resizeTimer);
    resizeTimer = setTimeout(() => state.rerender?.(), 120);
  });

  /* ---------- Data ---------- */
  async function loadData() {
    // data.js (window.APEX_DATA) works from file://; JSON fetches need a web server.
    let raw = window.APEX_DATA;
    if (!raw) {
      const loaded = await Promise.all(DATA_FILES.map((name) => fetch(`data/${name}.json`).then((response) => {
        if (!response.ok) throw new Error(`data/${name}.json returned ${response.status}`);
        return response.json();
      })));
      raw = Object.fromEntries(DATA_FILES.map((name, index) => [name, loaded[index]]));
    }
    const { metadata, results } = raw;
    const resultsByRace = d3.group(results, (entry) => entry.raceId);
    const racesWithResults = metadata.races.filter((race) => resultsByRace.has(race.raceId));
    const winnerByRace = new Map(results.filter((entry) => entry.positionOrder === 1).map((entry) => [entry.raceId, entry]));
    // Rank each season by points so tables can show where a driver or team finished.
    const rankSeasons = (rows) => {
      d3.group(rows, (row) => row.year).forEach((season) => {
        [...season].sort((a, b) => b.points - a.points || b.wins - a.wins).forEach((row, index) => { row.rank = index + 1; });
      });
      return rows;
    };
    return {
      ...metadata,
      results, resultsByRace, winnerByRace,
      races: racesWithResults,
      seasons: [...new Set(racesWithResults.map((race) => race.year))].sort((a, b) => a - b),
      raceMetrics: raw.race_metrics,
      driverSeasons: rankSeasons(raw.driver_seasons),
      constructorSeasons: rankSeasons(raw.constructor_seasons),
      circuitSummary: raw.circuit_summary,
      qualifying: raw.qualifying,
      sprintResults: raw.sprint_results,
      weatherImpact: raw.weather_impact || [],
      drivers: byId(metadata.drivers, "driverId"),
      constructors: byId(metadata.constructors, "constructorId"),
      circuits: byId(metadata.circuits, "circuitId"),
      metrics: byId(raw.race_metrics, "raceId"),
      raceById: byId(metadata.races, "raceId"),
    };
  }
  function showSetup(error) {
    const fromDisk = location.protocol === "file:";
    $("[data-page-root]").innerHTML = `<div class="setup-card"><p class="eyebrow">Setup needed</p><h2>Dashboard data has not been generated yet</h2>
      <p>Copy the Kaggle CSV files into a <code>Data</code> folder at the repository root, then build the data:</p>
      <pre>python scripts/build_data.py --data-dir Data --output-dir site/data</pre>
      <p>${fromDisk ? "Then reopen this page. The build writes <code>site/data/data.js</code>, so the pages work straight from disk." : "Then refresh this page."}</p>
      <p>No Kaggle data yet? Preview with synthetic sample data:</p>
      <pre>python scripts/make_sample_data.py --output-dir SampleData
python scripts/build_data.py --data-dir SampleData --output-dir site/data --demo</pre></div>`;
    console.warn("Could not load dashboard data.", error);
  }

  /* ---------- Race Overview ---------- */
  function overview() {
    const latest = state.data.races.at(-1);
    const filters = { year: latest.year, raceId: latest.raceId, driverId: "", constructorId: "", statusId: "" };
    const container = $("[data-overview-filters]");
    function fill() {
      const seasonRaces = state.data.races.filter((race) => race.year === +filters.year);
      const entries = resultsFor(+filters.raceId);
      const uniqueOptions = (key, label) => [...new Map(entries.map((entry) => [entry[key], label(entry)])).entries()].sort((a, b) => a[1].localeCompare(b[1]));
      container.innerHTML =
        selectMarkup("overview-year", "Season", [...state.data.seasons].reverse().map((year) => [year, year]), filters.year) +
        selectMarkup("overview-race", "Grand Prix", seasonRaces.map((race) => [race.raceId, `R${race.round} · ${race.name}`]), filters.raceId, true) +
        selectMarkup("overview-driver", "Driver", [["", "All drivers"], ...uniqueOptions("driverId", (entry) => driverName(entry.driverId))], filters.driverId, true) +
        selectMarkup("overview-constructor", "Constructor", [["", "All constructors"], ...uniqueOptions("constructorId", (entry) => teamName(entry.constructorId))], filters.constructorId, true) +
        selectMarkup("overview-status", "Result status", [["", "All statuses"], ...uniqueOptions("statusId", (entry) => entry.status)], filters.statusId, true);
      onChange("overview-year", (value) => {
        filters.year = +value;
        filters.raceId = state.data.races.filter((race) => race.year === filters.year).at(-1)?.raceId;
        resetEntrantFilters(); fill(); render();
      });
      onChange("overview-race", (value) => { filters.raceId = +value; resetEntrantFilters(); fill(); render(); });
      onChange("overview-driver", (value) => { filters.driverId = value; render(); });
      onChange("overview-constructor", (value) => { filters.constructorId = value; render(); });
      onChange("overview-status", (value) => { filters.statusId = value; render(); });
    }
    function resetEntrantFilters() { filters.driverId = ""; filters.constructorId = ""; filters.statusId = ""; }
    function render() {
      const race = state.data.raceById.get(+filters.raceId);
      if (!race) return;
      const all = resultsFor(race.raceId);
      const results = all.filter((entry) => (!filters.driverId || entry.driverId === +filters.driverId) && (!filters.constructorId || entry.constructorId === +filters.constructorId) && (!filters.statusId || entry.statusId === +filters.statusId));
      const raceMetric = metric(race.raceId);
      const winner = state.data.winnerByRace.get(race.raceId);
      const classified = all.filter((entry) => entry.position > 0).length;
      $("[data-race-strip]").innerHTML = `<strong>${escape(race.year)} ${escape(race.name)}</strong><span>Round ${race.round}</span><span class="dot-sep">${escape(race.date || "Date unknown")}</span><span class="dot-sep">${escape(race.circuitName)}${race.country ? `, ${escape(race.country)}` : ""}</span>${weatherChip(race.weather)}`;
      kpis($("[data-overview-kpis]"), [
        { label: "Winner", value: winner ? driverName(winner.driverId) : "—", detail: winner ? `${teamName(winner.constructorId)} · from P${winner.grid || "L"}` : "No winner recorded" },
        { label: "Fastest lap", value: lapTime(raceMetric.fastestLapMs), detail: raceMetric.fastestLapDriverId ? driverName(raceMetric.fastestLapDriverId) : "Not recorded for this race" },
        { label: "Average lap time", value: lapTime(raceMetric.averageLapTimeMs), detail: `${fmtNumber(raceMetric.lapSamples || 0)} recorded laps` },
        { label: "Classified finishers", value: `${classified} / ${all.length}`, detail: `${all.length - classified} retirements or non-classified`, tone: "" },
      ]);
      gridMovement($("[data-chart='grid-movement']"), results);
      constructorBars($("[data-chart='constructor-points']"), results);
      driverPace($("[data-chart='driver-pace']"), results);
      weatherPanel($("[data-weather-panel]"), race.weather);
      dataTable($("[data-results-table]"), [
        { key: "positionOrder", label: "Pos", numeric: false, html: (row) => positionBadge(row.position) },
        { key: "driver", label: "Driver", strong: true, value: (row) => driverName(row.driverId) },
        { key: "constructor", label: "Constructor", value: (row) => teamName(row.constructorId) },
        { key: "grid", label: "Grid", numeric: true, value: (row) => row.grid || null, html: (row) => row.grid || "PL" },
        { key: "gain", label: "Δ Pos", numeric: true, value: (row) => (row.position && row.grid ? row.grid - row.position : null), html: (row) => { const gain = row.position && row.grid ? row.grid - row.position : null; return gain == null ? "—" : `<span class="${gain > 0 ? "positive" : gain < 0 ? "negative" : ""}">${signed(gain)}</span>`; } },
        { key: "laps", label: "Laps", numeric: true },
        { key: "fastestLapTime", label: "Best lap", numeric: true, value: (row) => row.fastestLapTime || null, html: (row) => lapTime(row.fastestLapTime) },
        { key: "points", label: "Points", numeric: true, html: (row) => fmtOne(row.points) },
        { key: "status", label: "Status", html: (row) => statusChip(row.status) },
      ], results, { key: "positionOrder", dir: "asc" });
      $("[data-export-results]").onclick = () => exportResults(results, race);
    }
    fill(); render(); state.rerender = render;
  }
  function weatherPanel(target, weather) {
    target.classList.toggle("missing", !weather?.available);
    target.innerHTML = weather?.available
      ? `<div><div class="weather-icon ${weather.condition === "Wet" ? "wet" : "dry"}">${weather.condition === "Wet" ? "☂" : "☀"}</div><strong>${escape(weather.condition)} conditions</strong><div class="weather-metrics"><span><b>${fmtOne(weather.temperatureC)}°C</b>Temperature</span><span><b>${fmtOne(weather.precipitationMm)} mm</b>Rain</span><span><b>${fmtOne(weather.windSpeed)}</b>Wind</span></div><p>Observed ${escape(weather.observedAt?.replace(" ", " · ") || "at race time")}. Wind speed is shown in the source dataset's unit.</p></div>`
      : `<div><div class="weather-icon">☁</div><strong>Weather data not available</strong><p>Add <code>weather_features_v4.csv</code> to the <code>Data</code> folder, then rebuild the dashboard data to populate this race.</p></div>`;
  }
  function gridMovement(target, results) {
    const rows = results.filter((entry) => entry.position > 0).sort((a, b) => a.position - b.position).slice(0, 12);
    legend("grid-movement", [["ring", "Grid slot"], ["", "Finish"], ["green bar-key", "Gained"], ["bar-key", "Lost"]]);
    if (!rows.length) return empty(target);
    const { svg, width, height } = svgFor(target, 300, { top: 6, right: 18, bottom: 32, left: 136 });
    const max = Math.max(20, d3.max(rows, (row) => Math.max(row.grid || 0, row.position)));
    const x = d3.scaleLinear().domain([1, max]).range([0, width]);
    const y = d3.scaleBand().domain(rows.map((row) => row.driverId)).range([0, height]).padding(0.42);
    const cy = (row) => y(row.driverId) + y.bandwidth() / 2;
    const start = (row) => x(row.grid || max);
    const ticks = Math.max(2, Math.min(10, Math.floor(width / 48)));
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x).ticks(ticks).tickFormat((d) => `P${d}`));
    svg.append("g").attr("class", "grid").call(d3.axisBottom(x).ticks(ticks).tickSize(height).tickFormat(""));
    svg.selectAll(".move-line").data(rows).join("line")
      .attr("class", (row) => `move-line ${!row.grid || row.grid > row.position ? "gain" : row.grid < row.position ? "loss" : "same"}`)
      .attr("x1", start).attr("x2", start).attr("y1", cy).attr("y2", cy)
      .transition().duration(ms(550)).ease(d3.easeCubicOut).attr("x2", (row) => x(row.position));
    svg.selectAll(".dot-grid").data(rows).join("circle").attr("class", "dot-grid").attr("cx", start).attr("cy", cy).attr("r", 4);
    const dots = svg.selectAll(".dot").data(rows).join("circle").attr("class", "dot").attr("cx", start).attr("cy", cy).attr("r", 6);
    dots.transition().duration(ms(550)).ease(d3.easeCubicOut).attr("cx", (row) => x(row.position));
    hover(dots, (row) => `<strong>${escape(driverName(row.driverId))}</strong>${escape(teamName(row.constructorId))}<br>${row.grid ? `Grid P${row.grid}` : "Pit-lane start"} → finish P${row.position}<br><span class="muted">${row.grid ? signed(row.grid - row.position) : "—"} places</span>`);
    drawAxis(svg.append("g"), d3.axisLeft(y).tickSize(0).tickPadding(10).tickFormat((id) => driverName(id)), "y-names");
  }
  function constructorBars(target, results) {
    const rows = d3.rollups(results, (items) => sum(items, (item) => item.points), (item) => item.constructorId)
      .map(([id, points]) => ({ id, points, name: teamName(id) }))
      .filter((row) => row.points > 0).sort((a, b) => b.points - a.points).slice(0, 8);
    if (!rows.length) return empty(target, "No points were scored by the selected entries.");
    hBars(target, rows.map((row) => ({ label: row.name, value: row.points, tip: `<strong>${escape(row.name)}</strong>${fmtOne(row.points)} points` })), { format: fmtOne });
  }
  function driverPace(target, results) {
    const rows = results.filter((entry) => entry.averageLapMs > 0 && entry.lapSamples >= 3).sort((a, b) => a.averageLapMs - b.averageLapMs).slice(0, 12);
    if (!rows.length) return empty(target, "Lap-by-lap timing is not recorded for this race (the Kaggle lap_times data starts in 1996).");
    const best = rows[0].averageLapMs;
    hBars(target, rows.map((row) => ({
      label: driverName(row.driverId), value: row.averageLapMs - best,
      tip: `<strong>${escape(driverName(row.driverId))}</strong>Average ${lapTime(row.averageLapMs)}<br><span class="muted">${gapSeconds(row.averageLapMs - best)} to quickest · ${row.lapSamples} laps</span>`,
    })), { format: (value) => gapSeconds(value), height: 300, minBar: 2, labelFor: (row) => (row.value === 0 ? lapTime(best) : gapSeconds(row.value)) });
  }
  /** Horizontal bar chart used by several panels. rows: { label, value, tip, cls }. */
  function hBars(target, rows, { format = fmtNumber, height = 280, left = 120, minBar = 0, labelFor } = {}) {
    const { svg, width, height: innerHeight } = svgFor(target, height, { top: 6, right: 64, bottom: 26, left });
    const x = d3.scaleLinear().domain([0, d3.max(rows, (row) => row.value) || 1]).nice().range([0, width]);
    const y = d3.scaleBand().domain(rows.map((row) => row.label)).range([0, innerHeight]).padding(0.28);
    svg.append("g").attr("class", "grid").call(d3.axisBottom(x).ticks(4).tickSize(innerHeight).tickFormat(""));
    const bars = svg.selectAll("rect").data(rows).join("rect").attr("class", (row) => row.cls || "bar").attr("x", 0).attr("y", (row) => y(row.label)).attr("height", y.bandwidth()).attr("rx", 3);
    animateWidth(bars, (row) => Math.max(minBar, x(row.value)));
    hover(bars, (row) => row.tip);
    svg.selectAll(".bar-label").data(rows).join("text").attr("class", "bar-label")
      .attr("x", (row) => Math.max(minBar, x(row.value)) + 6).attr("y", (row) => y(row.label) + y.bandwidth() / 2).attr("dy", "0.35em")
      .text((row) => (labelFor ? labelFor(row) : format(row.value)));
    drawAxis(svg.append("g"), d3.axisLeft(y).tickSize(0).tickPadding(8).tickFormat((label) => (label.length > 17 ? `${label.slice(0, 16)}…` : label)), "y-names");
  }
  function exportResults(rows, race) {
    const header = ["position", "driver", "constructor", "grid", "laps", "best_lap", "points", "status"];
    const lines = [...rows].sort((a, b) => a.positionOrder - b.positionOrder).map((row) => [row.position || "NC", driverName(row.driverId), teamName(row.constructorId), row.grid, row.laps, lapTime(row.fastestLapTime), row.points, row.status]
      .map((value) => `"${String(value ?? "").replaceAll("\"", "\"\"")}"`).join(","));
    const link = document.createElement("a");
    link.href = URL.createObjectURL(new Blob([[header.join(","), ...lines].join("\n")], { type: "text/csv" }));
    link.download = `${race.year}-${race.name.replaceAll(/[^a-z0-9]+/gi, "-").toLowerCase()}-results.csv`;
    document.body.appendChild(link);
    link.click();
    link.remove();
    setTimeout(() => URL.revokeObjectURL(link.href), 1000);
  }

  /* ---------- Race Comparison ---------- */
  function comparison() {
    const races = [...state.data.races].sort((a, b) => b.year - a.year || b.round - a.round);
    const byName = d3.group(races, (race) => race.name);
    const repeatNames = [...byName.entries()].filter(([, list]) => list.length > 1).map(([name]) => name).sort();
    const first = races[0];
    const sameEvent = races.find((race) => race.name === first.name && race.raceId !== first.raceId) || races[1] || first;
    const filters = { mode: byName.get(first.name)?.length > 1 ? "event" : "any", a: first.raceId, b: sameEvent.raceId, event: first.name };
    const target = $("[data-comparison-filters]");
    function fill() {
      const mode = `<div class="filter-group"><span>Compare</span><div class="segmented" role="group" aria-label="Comparison mode"><button type="button" data-mode="event" aria-pressed="${filters.mode === "event"}">Same Grand Prix</button><button type="button" data-mode="any" aria-pressed="${filters.mode === "any"}">Any two races</button></div></div>`;
      if (filters.mode === "event") {
        const seasons = byName.get(filters.event) || [];
        target.innerHTML = mode +
          selectMarkup("compare-event", "Grand Prix", repeatNames.map((name) => [name, `${name} (${byName.get(name).length} seasons)`]), filters.event, true) +
          selectMarkup("compare-a", "Season A", seasons.map((race) => [race.raceId, race.year]), filters.a) +
          selectMarkup("compare-b", "Season B", seasons.map((race) => [race.raceId, race.year]), filters.b);
        onChange("compare-event", (value) => {
          filters.event = value;
          const list = byName.get(value);
          filters.a = list[0].raceId; filters.b = (list[1] || list[0]).raceId;
          fill(); render();
        });
      } else {
        const options = races.map((race) => [race.raceId, `${raceLabel(race)} — ${race.circuitName}`]);
        target.innerHTML = mode + selectMarkup("compare-a", "Race A", options, filters.a, true) + selectMarkup("compare-b", "Race B", options, filters.b, true);
      }
      onChange("compare-a", (value) => { filters.a = +value; render(); });
      onChange("compare-b", (value) => { filters.b = +value; render(); });
      target.querySelectorAll("[data-mode]").forEach((button) => button.addEventListener("click", () => {
        filters.mode = button.dataset.mode;
        if (filters.mode === "event") {
          const raceA = state.data.raceById.get(filters.a);
          filters.event = byName.get(raceA?.name)?.length > 1 ? raceA.name : repeatNames[0];
          const list = byName.get(filters.event) || [];
          if (!list.some((race) => race.raceId === filters.a)) filters.a = list[0]?.raceId;
          if (!list.some((race) => race.raceId === filters.b) || filters.b === filters.a) filters.b = (list.find((race) => race.raceId !== filters.a) || list[0])?.raceId;
        }
        fill(); render();
      }));
    }
    function render() {
      const a = state.data.raceById.get(+filters.a), b = state.data.raceById.get(+filters.b);
      if (!a || !b) return;
      const ma = metric(a.raceId), mb = metric(b.raceId);
      const ea = resultsFor(a.raceId), eb = resultsFor(b.raceId);
      const finishers = (rows) => rows.filter((row) => row.position > 0).length;
      const lapDelta = ma.averageLapTimeMs && mb.averageLapTimeMs ? mb.averageLapTimeMs - ma.averageLapTimeMs : NaN;
      $("[data-comparison-labels]").innerHTML = [[a, "A"], [b, "B"]].map(([race, key]) => `<div class="race-label ${key.toLowerCase()}">RACE ${key}<strong>${escape(raceLabel(race))}</strong><small><span class="chip">${escape(race.circuitName)}</span>${weatherChip(race.weather)}</small></div>`).join("");
      const pair = (x, y) => `<span>${escape(x)}</span><span class="vs">/</span><span class="b">${escape(y)}</span>`;
      kpis($("[data-comparison-kpis]"), [
        { label: "Average lap time", html: pair(lapTime(ma.averageLapTimeMs), lapTime(mb.averageLapTimeMs)), title: `${lapTime(ma.averageLapTimeMs)} / ${lapTime(mb.averageLapTimeMs)}`, detail: Number.isFinite(lapDelta) ? `Race B is ${gapSeconds(lapDelta)} per lap` : "Lap timing missing for one race" },
        { label: "Fastest lap", html: pair(lapTime(ma.fastestLapMs), lapTime(mb.fastestLapMs)), title: "Fastest lap A / B", detail: ma.fastestLapMs && mb.fastestLapMs ? `B vs A: ${gapSeconds(mb.fastestLapMs - ma.fastestLapMs)}` : "Not recorded for one race" },
        { label: "Finishers", html: pair(`${finishers(ea)}/${ea.length}`, `${finishers(eb)}/${eb.length}`), title: "Classified / starters", detail: `Retirements: ${ea.length - finishers(ea)} vs ${eb.length - finishers(eb)}` },
        { label: "Winners", value: winnerName(a.raceId), detail: `Race B: ${winnerName(b.raceId)}` },
      ]);
      legend("comparison-pace", [["", "Race A driver"], ["b", "Race B driver"], ["muted bar-key", "Median"]]);
      legend("comparison-finish", [["bar-key", "Race A"], ["b bar-key", "Race B"]]);
      legend("comparison-constructors", [["bar-key", "Race A"], ["b bar-key", "Race B"]]);
      comparisonPace($("[data-chart='comparison-pace']"), ea, eb);
      finishProfile($("[data-chart='comparison-finish']"), ea, eb);
      comparisonConstructors($("[data-chart='comparison-constructors']"), ea, eb);
    }
    fill(); render(); state.rerender = render;
  }
  /** Strip plot of each driver's average lap in race A and race B, with medians. */
  function comparisonPace(target, a, b) {
    const pick = (rows, key) => rows.filter((row) => row.averageLapMs > 0 && row.lapSamples >= 3).map((row) => ({ ...row, key }));
    const rows = [...pick(a, "Race A"), ...pick(b, "Race B")];
    if (!rows.length) return empty(target, "No lap-time records exist for these races (lap timing starts in 1996).");
    const { svg, width, height } = svgFor(target, 280, { top: 16, right: 24, bottom: 40, left: 70 });
    const extent = d3.extent(rows, (row) => row.averageLapMs);
    const pad = Math.max(500, (extent[1] - extent[0]) * 0.06);
    const x = d3.scaleLinear().domain([extent[0] - pad, extent[1] + pad]).range([0, width]);
    const y = d3.scaleBand().domain(["Race A", "Race B"]).range([0, height]).padding(0.25);
    svg.append("g").attr("class", "grid").call(d3.axisBottom(x).ticks(6).tickSize(height).tickFormat(""));
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x).ticks(6).tickFormat(lapTime));
    drawAxis(svg.append("g"), d3.axisLeft(y).tickSize(0).tickPadding(10), "y-names");
    const jitter = d3.randomLcg(42);
    const dots = svg.selectAll(".pace-dot").data(rows).join("circle")
      .attr("class", (row) => (row.key === "Race A" ? "dot-a" : "dot-b"))
      .attr("cx", (row) => x(row.averageLapMs))
      .attr("cy", (row) => y(row.key) + y.bandwidth() * (0.25 + 0.5 * jitter()))
      .attr("r", 0);
    dots.transition().duration(ms(400)).delay((_, i) => ms(i * 6)).attr("r", 5.5);
    hover(dots, (row) => `<strong>${escape(driverName(row.driverId))}</strong>${escape(row.key)} · ${escape(teamName(row.constructorId))}<br>Average lap ${lapTime(row.averageLapMs)}<br><span class="muted">${row.position ? `Finished P${row.position}` : escape(row.status)}</span>`);
    d3.groups(rows, (row) => row.key).forEach(([key, list]) => {
      const median = d3.median(list, (row) => row.averageLapMs);
      svg.append("line").attr("class", "median-mark").attr("x1", x(median)).attr("x2", x(median)).attr("y1", y(key) + 4).attr("y2", y(key) + y.bandwidth() - 4);
      svg.append("text").attr("class", "chart-label").attr("x", x(median) + 6).attr("y", y(key) + 10).text(`median ${lapTime(median)}`);
    });
    if (!pick(a, "A").length || !pick(b, "B").length) svg.append("text").attr("class", "chart-label").attr("x", width / 2).attr("y", y(pick(a, "A").length ? "Race B" : "Race A") + y.bandwidth() / 2).attr("text-anchor", "middle").text("No lap timing recorded for this race");
  }
  function finishProfile(target, a, b) {
    const groups = ["Finished", "Lapped", "Retired"];
    const bucket = (rows, name) => rows.filter((row) => (name === "Retired" ? !row.position : name === "Lapped" ? row.position && row.status !== "Finished" : row.position && row.status === "Finished")).length;
    const rows = groups.flatMap((group) => [{ group, key: "A", value: bucket(a, group) }, { group, key: "B", value: bucket(b, group) }]);
    const { svg, width, height } = svgFor(target, 280, { top: 18, right: 12, bottom: 34, left: 34 });
    const x0 = d3.scaleBand().domain(groups).range([0, width]).padding(0.24);
    const x1 = d3.scaleBand().domain(["A", "B"]).range([0, x0.bandwidth()]).padding(0.1);
    const y = d3.scaleLinear().domain([0, d3.max(rows, (row) => row.value) || 1]).nice().range([height, 0]);
    svg.append("g").attr("class", "grid").call(d3.axisLeft(y).ticks(5).tickSize(-width).tickFormat(""));
    const bars = svg.selectAll("rect").data(rows).join("rect").attr("class", (row) => (row.key === "A" ? "bar" : "bar-b"))
      .attr("x", (row) => x0(row.group) + x1(row.key)).attr("width", x1.bandwidth()).attr("rx", 3);
    animateHeight(bars, (row) => y(row.value), (row) => height - y(row.value), height);
    hover(bars, (row) => `<strong>Race ${row.key} · ${row.group}</strong>${row.value} drivers`);
    svg.selectAll(".bar-label").data(rows).join("text").attr("class", "bar-label").attr("text-anchor", "middle")
      .attr("x", (row) => x0(row.group) + x1(row.key) + x1.bandwidth() / 2).attr("y", (row) => y(row.value) - 5).text((row) => row.value);
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x0).tickSize(0).tickPadding(10), "y-names");
    drawAxis(svg.append("g"), d3.axisLeft(y).ticks(5));
  }
  function comparisonConstructors(target, a, b) {
    const byTeam = (rows) => new Map(d3.rollups(rows, (items) => sum(items, (item) => item.points), (item) => item.constructorId));
    const pointsA = byTeam(a), pointsB = byTeam(b);
    const ids = [...new Set([...pointsA.keys(), ...pointsB.keys()])]
      .map((id) => ({ id, a: pointsA.get(id) || 0, b: pointsB.get(id) || 0 }))
      .filter((row) => row.a + row.b > 0).sort((x, y) => y.a + y.b - (x.a + x.b)).slice(0, 10);
    if (!ids.length) return empty(target, "Neither race awarded constructor points.");
    const rows = ids.map((row) => ({ ...row, name: teamName(row.id) }));
    const { svg, width, height } = svgFor(target, Math.max(220, rows.length * 34 + 40), { top: 6, right: 48, bottom: 28, left: 140 });
    const y = d3.scaleBand().domain(rows.map((row) => row.name)).range([0, height]).padding(0.24);
    const x = d3.scaleLinear().domain([0, d3.max(rows, (row) => Math.max(row.a, row.b)) || 1]).nice().range([0, width]);
    svg.append("g").attr("class", "grid").call(d3.axisBottom(x).ticks(6).tickSize(height).tickFormat(""));
    const half = y.bandwidth() / 2;
    [["a", "bar", 0], ["b", "bar-b", half]].forEach(([key, cls, offset]) => {
      const bars = svg.selectAll(`.${cls}-${key}`).data(rows).join("rect").attr("class", cls).attr("x", 0).attr("y", (row) => y(row.name) + offset).attr("height", half - 1).attr("rx", 2);
      animateWidth(bars, (row) => x(row[key]));
      hover(bars, (row) => `<strong>${escape(row.name)}</strong>Race A: ${fmtOne(row.a)} pts<br>Race B: ${fmtOne(row.b)} pts<br><span class="muted">Change ${signed(row.b - row.a, fmtOne)}</span>`);
      svg.selectAll(`.label-${key}`).data(rows).join("text").attr("class", "bar-label").attr("x", (row) => x(row[key]) + 5).attr("y", (row) => y(row.name) + offset + half / 2).attr("dy", "0.35em").text((row) => (row[key] ? fmtOne(row[key]) : ""));
    });
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x).ticks(6));
    drawAxis(svg.append("g"), d3.axisLeft(y).tickSize(0).tickPadding(10), "y-names");
  }

  /* ---------- Weather impact ---------- */
  function weather() {
    const allRows = state.data.weatherImpact.filter((row) => Number.isFinite(row.representativeLapMs));
    const filterTarget = $("[data-weather-filters]");
    if (!allRows.length) {
      empty(filterTarget, "Add <code>weather_features_v4.csv</code> to <code>Data</code>, then rebuild the dashboard data.");
      $("[data-weather-kpis]").innerHTML = "";
      document.querySelectorAll("[data-chart^='weather-'], [data-weather-table]").forEach((target) => empty(target, "No constructor lap-time records with matched weather are available."));
      return;
    }

    const years = [...new Set(allRows.map((row) => row.year))].sort((a, b) => a - b);
    const circuits = d3.rollups(allRows, (rows) => rows.length, (row) => row.circuitId)
      .sort((a, b) => state.data.circuits.get(a[0])?.name.localeCompare(state.data.circuits.get(b[0])?.name));
    const constructors = [...new Set(allRows.map((row) => row.constructorId))]
      .sort((a, b) => teamName(a).localeCompare(teamName(b)));
    const filters = { start: years[0], end: years.at(-1), circuitId: "", constructorId: "", condition: "" };

    function fill() {
      filterTarget.innerHTML =
        selectMarkup("weather-start", "From season", years.map((year) => [year, year]), filters.start) +
        selectMarkup("weather-end", "To season", years.map((year) => [year, year]), filters.end) +
        selectMarkup("weather-circuit", "Circuit", [["", "All circuits"], ...circuits.map(([id, count]) => [id, `${state.data.circuits.get(id)?.name || id} (${count})`])], filters.circuitId, true) +
        selectMarkup("weather-constructor", "Constructor", [["", "All constructors"], ...constructors.map((id) => [id, teamName(id)])], filters.constructorId, true) +
        selectMarkup("weather-condition", "Conditions", [["", "Wet and dry"], ["Dry", "Dry only"], ["Wet", "Wet only"]], filters.condition) +
        `<button class="filter-reset" type="button" data-reset-weather>Reset filters</button>`;
      onChange("weather-start", (value) => { filters.start = +value; orderRange(filters); fill(); render(); });
      onChange("weather-end", (value) => { filters.end = +value; orderRange(filters); fill(); render(); });
      onChange("weather-circuit", (value) => { filters.circuitId = value; render(); });
      onChange("weather-constructor", (value) => { filters.constructorId = value; render(); });
      onChange("weather-condition", (value) => { filters.condition = value; render(); });
      $("[data-reset-weather]").addEventListener("click", () => {
        Object.assign(filters, { start: years[0], end: years.at(-1), circuitId: "", constructorId: "", condition: "" });
        fill(); render();
      });
    }

    function baseRows() {
      return allRows.filter((row) => row.year >= filters.start && row.year <= filters.end &&
        (!filters.circuitId || row.circuitId === +filters.circuitId) &&
        (!filters.constructorId || row.constructorId === +filters.constructorId));
    }
    function render() {
      const base = baseRows();
      const rows = filters.condition ? base.filter((row) => row.condition === filters.condition) : base;
      const dry = base.filter((row) => row.condition === "Dry");
      const wet = base.filter((row) => row.condition === "Wet");
      const averageGap = d3.mean(rows, (row) => row.paceGapPercent);
      const wetDryGap = wet.length && dry.length ? d3.mean(wet, (row) => row.paceGapPercent) - d3.mean(dry, (row) => row.paceGapPercent) : NaN;
      const entries = sum(rows, (row) => row.entryCount);
      const dnfs = sum(rows, (row) => row.dnfCount);
      kpis($("[data-weather-kpis]"), [
        { label: "Constructor-race samples", value: fmtNumber(rows.length), detail: `${fmtNumber(sum(rows, (row) => row.lapSamples))} recorded laps` },
        { label: "Average pace gap", value: Number.isFinite(averageGap) ? fmtPct(averageGap) : "—", detail: "Gap to fastest constructor in each race" },
        { label: "Wet vs. dry gap", value: Number.isFinite(wetDryGap) ? signed(wetDryGap, fmtOne) + "%" : "—", detail: wet.length && dry.length ? "Wet average minus dry average" : "Both conditions are required" },
        { label: "DNF rate", value: entries ? fmtPct((100 * dnfs) / entries) : "—", detail: `${fmtNumber(dnfs)} non-classified entries` },
      ]);
      legend("weather-precipitation", [["dry", "Dry race"], ["wet", "Wet race"]]);
      legend("weather-temperature", [["dry", "Dry race"], ["wet", "Wet race"]]);
      legend("weather-condition", [["dry", "Dry"], ["wet", "Wet"]]);
      weatherScatter($("[data-chart='weather-precipitation']"), rows, { xKey: "precipitationMm", xLabel: "Precipitation (mm)", title: "Precipitation vs. constructor pace" });
      weatherConditionBars($("[data-chart='weather-condition']"), base, "pace");
      weatherScatter($("[data-chart='weather-temperature']"), rows, { xKey: "temperatureC", xLabel: "Temperature (°C)", title: "Temperature vs. constructor pace" });
      weatherConditionBars($("[data-chart='weather-reliability']"), base, "reliability");
      dataTable($("[data-weather-table]"), [
        { key: "year", label: "Year", strong: true },
        { key: "race", label: "Grand Prix", value: (row) => row.raceName },
        { key: "circuit", label: "Circuit", value: (row) => row.circuitName },
        { key: "constructor", label: "Constructor", value: (row) => teamName(row.constructorId) },
        { key: "condition", label: "Conditions", html: (row) => weatherChip({ available: true, condition: row.condition, temperatureC: row.temperatureC }) },
        { key: "precipitation", label: "Precip.", numeric: true, value: (row) => row.precipitationMm, html: (row) => `${fmtOne(row.precipitationMm)} mm` },
        { key: "temperature", label: "Temp.", numeric: true, value: (row) => row.temperatureC, html: (row) => `${fmtOne(row.temperatureC)}°C` },
        { key: "averageLap", label: "Avg. lap", numeric: true, value: (row) => row.teamAverageLapMs, html: (row) => lapTime(row.teamAverageLapMs) },
        { key: "paceGap", label: "Pace gap", numeric: true, value: (row) => row.paceGapPercent, html: (row) => `${fmtOne(row.paceGapPercent)}%` },
        { key: "dnfs", label: "DNFs", numeric: true, value: (row) => row.dnfCount },
      ], rows, { key: "paceGap", dir: "desc" });
    }
    fill(); render(); state.rerender = render;
  }

  function weatherScatter(target, rows, { xKey, xLabel, title }) {
    const data = rows.filter((row) => Number.isFinite(row[xKey]) && Number.isFinite(row.paceGapPercent));
    if (data.length < 2) return empty(target, "At least two matched constructor-race records are required for this view.");
    const { svg, width, height } = svgFor(target, 290, { top: 16, right: 20, bottom: 46, left: 54 });
    const extent = d3.extent(data, (row) => row[xKey]);
    const xPad = Math.max(0.5, (extent[1] - extent[0]) * 0.08);
    const yMax = d3.max(data, (row) => row.paceGapPercent) || 1;
    const x = d3.scaleLinear().domain(extent[0] === extent[1] ? [extent[0] - xPad, extent[1] + xPad] : [extent[0] - xPad, extent[1] + xPad]).nice().range([0, width]);
    const y = d3.scaleLinear().domain([0, yMax * 1.08]).nice().range([height, 0]);
    svg.append("g").attr("class", "grid").call(d3.axisLeft(y).ticks(5).tickSize(-width).tickFormat(""));
    const dots = svg.selectAll("circle").data(data).join("circle")
      .attr("class", (row) => (row.condition === "Wet" ? "weather-dot wet" : "weather-dot dry"))
      .attr("cx", (row) => x(row[xKey])).attr("cy", (row) => y(row.paceGapPercent)).attr("r", 0);
    dots.transition().duration(ms(350)).delay((_, index) => ms(Math.min(index * 2, 360))).attr("r", 4.2);
    hover(dots, (row) => `<strong>${escape(teamName(row.constructorId))}</strong>${escape(row.year)} ${escape(row.raceName)} · ${escape(row.condition)}<br>${escape(xLabel)}: ${fmtOne(row[xKey])}<br><span class="muted">Pace gap ${fmtOne(row.paceGapPercent)}% · Avg. lap ${lapTime(row.teamAverageLapMs)}</span>`);
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x).ticks(6));
    drawAxis(svg.append("g"), d3.axisLeft(y).ticks(5).tickFormat((value) => `${value}%`));
    svg.append("text").attr("class", "chart-label").attr("x", width / 2).attr("y", height + 38).attr("text-anchor", "middle").text(xLabel);
    svg.append("text").attr("class", "chart-label").attr("transform", `translate(${-42},${height / 2}) rotate(-90)`).attr("text-anchor", "middle").text("Pace gap to fastest constructor");
  }

  function weatherConditionBars(target, rows, mode) {
    const groups = ["Dry", "Wet"].map((condition) => {
      const values = rows.filter((row) => row.condition === condition);
      const value = mode === "pace"
        ? d3.mean(values, (row) => row.paceGapPercent)
        : 100 * sum(values, (row) => row.dnfCount) / sum(values, (row) => row.entryCount);
      return { condition, rows: values.length, value: Number.isFinite(value) ? value : null };
    }).filter((row) => row.value != null);
    if (!groups.length) return empty(target, "No wet or dry records match the selected filters.");
    const { svg, width, height } = svgFor(target, 290, { top: 16, right: 12, bottom: 38, left: 44 });
    const x = d3.scaleBand().domain(groups.map((row) => row.condition)).range([0, width]).padding(0.38);
    const y = d3.scaleLinear().domain([0, d3.max(groups, (row) => row.value) || 1]).nice().range([height, 0]);
    svg.append("g").attr("class", "grid").call(d3.axisLeft(y).ticks(5).tickSize(-width).tickFormat(""));
    const bars = svg.selectAll("rect").data(groups).join("rect").attr("class", (row) => `weather-bar ${row.condition.toLowerCase()}`).attr("x", (row) => x(row.condition)).attr("width", x.bandwidth()).attr("rx", 3);
    animateHeight(bars, (row) => y(row.value), (row) => height - y(row.value), height);
    hover(bars, (row) => `<strong>${row.condition} conditions</strong>${mode === "pace" ? `${fmtOne(row.value)}% average pace gap` : `${fmtPct(row.value)} DNF rate`}<br><span class="muted">${row.rows} constructor-race records</span>`);
    svg.selectAll(".bar-label").data(groups).join("text").attr("class", "bar-label").attr("text-anchor", "middle").attr("x", (row) => x(row.condition) + x.bandwidth() / 2).attr("y", (row) => y(row.value) - 6).text((row) => mode === "pace" ? `${fmtOne(row.value)}%` : fmtPct(row.value));
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x).tickSize(0).tickPadding(9));
    drawAxis(svg.append("g"), d3.axisLeft(y).ticks(5).tickFormat((value) => `${value}%`));
  }

  /* ---------- Drivers & Constructors (shared season views) ---------- */
  function careerSpan(rows) { const years = rows.map((row) => row.year); return [d3.min(years), d3.max(years)]; }
  function seasonFilters(target, prefix, label, ranked, filters, render, nameFor) {
    function fill() {
      const span = careerSpan(filters.seasons());
      const years = d3.range(span[0], span[1] + 1);
      target.innerHTML =
        selectMarkup(`${prefix}-id`, label, ranked.map(([id, points]) => [id, `${nameFor(id)} · ${fmtNumber(points)} pts`]), filters.id, true) +
        selectMarkup(`${prefix}-start`, "From season", years.map((year) => [year, year]), filters.start) +
        selectMarkup(`${prefix}-end`, "To season", years.map((year) => [year, year]), filters.end) +
        `<button class="filter-reset" type="button" data-reset-range>Full career</button>`;
      onChange(`${prefix}-id`, (value) => { filters.id = +value; [filters.start, filters.end] = careerSpan(filters.seasons()); fill(); render(); });
      onChange(`${prefix}-start`, (value) => { filters.start = +value; orderRange(filters); fill(); render(); });
      onChange(`${prefix}-end`, (value) => { filters.end = +value; orderRange(filters); fill(); render(); });
      $("[data-reset-range]", target).addEventListener("click", () => { [filters.start, filters.end] = careerSpan(filters.seasons()); fill(); render(); });
    }
    fill();
  }

  function drivers() {
    const ranked = d3.rollups(state.data.driverSeasons, (rows) => sum(rows, (row) => row.points), (row) => row.driverId).sort((a, b) => b[1] - a[1] || driverName(a[0]).localeCompare(driverName(b[0])));
    const filters = { id: ranked[0]?.[0], seasons: () => state.data.driverSeasons.filter((row) => row.driverId === filters.id) };
    [filters.start, filters.end] = careerSpan(filters.seasons());
    seasonFilters($("[data-driver-filters]"), "driver", "Driver", ranked, filters, render, driverName);
    function render() {
      const rows = filters.seasons().filter((row) => row.year >= filters.start && row.year <= filters.end).sort((a, b) => a.year - b.year);
      const results = state.data.results.filter((row) => {
        if (row.driverId !== filters.id) return false;
        const race = state.data.raceById.get(row.raceId);
        return race && race.year >= filters.start && race.year <= filters.end;
      });
      const name = driverName(filters.id);
      const driver = state.data.drivers.get(filters.id) || {};
      const starts = results.length;
      const classified = results.filter((row) => row.position > 0);
      const gains = results.filter((row) => row.grid > 0 && row.position > 0).map((row) => row.grid - row.position);
      const best = d3.min(rows, (row) => row.rank);
      const wins = sum(rows, (row) => row.wins), podiums = sum(rows, (row) => row.podiums), points = sum(rows, (row) => row.points);
      kpis($("[data-driver-kpis]"), [
        { label: "Total points", value: fmtOne(points), detail: `${name}${driver.nationality ? ` · ${driver.nationality}` : ""}` },
        { label: "Wins", value: fmtNumber(wins), detail: starts ? `${fmtPct((100 * wins) / starts)} win rate` : "No starts" },
        { label: "Podiums", value: fmtNumber(podiums), detail: starts ? `${fmtPct((100 * podiums) / starts)} of starts` : "No starts" },
        { label: "Starts", value: fmtNumber(starts), detail: `${rows.length} season${rows.length === 1 ? "" : "s"} · ${filters.start}–${filters.end}` },
        { label: "Average finish", value: classified.length ? `P${fmtOne(d3.mean(classified, (row) => row.position))}` : "—", detail: `${classified.length} classified finishes` },
        { label: "Positions gained", value: gains.length ? signed(d3.mean(gains), fmtOne) : "—", detail: "Average per classified race" },
        { label: "DNF rate", value: starts ? fmtPct((100 * (starts - classified.length)) / starts) : "—", detail: `${starts - classified.length} non-classified results` },
        { label: "Best season", value: best ? `P${best}` : "—", detail: best ? `Points rank in ${rows.filter((row) => row.rank === best).map((row) => row.year).join(", ")}` : "" },
      ]);
      const note = $("[data-chart-note='driver-points']"); if (note) note.textContent = "Ranks are by points scored that season";
      lineChart($("[data-chart='driver-points']"), rows, { yLabel: "Points", title: name, extra: (row) => `${row.wins} wins · points rank P${row.rank}` });
      finishBars($("[data-chart='driver-finishes']"), results);
      dataTable($("[data-driver-table]"), [
        { key: "year", label: "Season", strong: true },
        { key: "rank", label: "Rank", numeric: true, html: (row) => `P${row.rank}` },
        { key: "points", label: "Points", numeric: true, html: (row) => fmtOne(row.points) },
        { key: "wins", label: "Wins", numeric: true },
        { key: "podiums", label: "Podiums", numeric: true },
        { key: "starts", label: "Starts", numeric: true },
        { key: "dnfs", label: "DNFs", numeric: true },
        { key: "averageFinish", label: "Avg. finish", numeric: true, html: (row) => (row.averageFinish ? fmtOne(row.averageFinish) : "—") },
        { key: "averageGridGain", label: "Avg. gain", numeric: true, html: (row) => (row.averageGridGain == null ? "—" : `<span class="${row.averageGridGain > 0 ? "positive" : row.averageGridGain < 0 ? "negative" : ""}">${signed(row.averageGridGain, fmtOne)}</span>`) },
      ], rows, { key: "year", dir: "desc" });
    }
    render(); state.rerender = render;
  }

  function constructors() {
    const ranked = d3.rollups(state.data.constructorSeasons, (rows) => sum(rows, (row) => row.points), (row) => row.constructorId).sort((a, b) => b[1] - a[1] || teamName(a[0]).localeCompare(teamName(b[0])));
    const filters = { id: ranked[0]?.[0], seasons: () => state.data.constructorSeasons.filter((row) => row.constructorId === filters.id) };
    [filters.start, filters.end] = careerSpan(filters.seasons());
    seasonFilters($("[data-constructor-filters]"), "constructor", "Constructor", ranked, filters, render, teamName);
    function render() {
      const rows = filters.seasons().filter((row) => row.year >= filters.start && row.year <= filters.end).sort((a, b) => a.year - b.year);
      const name = teamName(filters.id);
      const team = state.data.constructors.get(filters.id) || {};
      const starts = sum(rows, (row) => row.starts), dnfs = sum(rows, (row) => row.dnfs);
      const wins = sum(rows, (row) => row.wins), podiums = sum(rows, (row) => row.podiums), points = sum(rows, (row) => row.points);
      const best = d3.min(rows, (row) => row.rank);
      const raceIds = new Set(state.data.results.filter((row) => row.constructorId === filters.id && rows.some((season) => season.year === state.data.raceById.get(row.raceId)?.year)).map((row) => row.raceId));
      kpis($("[data-constructor-kpis]"), [
        { label: "Total points", value: fmtOne(points), detail: `${name}${team.nationality ? ` · ${team.nationality}` : ""}` },
        { label: "Wins", value: fmtNumber(wins), detail: raceIds.size ? `${fmtPct((100 * wins) / raceIds.size)} of races entered` : "" },
        { label: "Podiums", value: fmtNumber(podiums), detail: `${fmtNumber(raceIds.size)} races entered` },
        { label: "Best season", value: best ? `P${best}` : "—", detail: best ? `Points rank in ${rows.filter((row) => row.rank === best).map((row) => row.year).join(", ")}` : "" },
        { label: "Race entries", value: fmtNumber(starts), detail: "Individual car starts" },
        { label: "Points per entry", value: starts ? fmtOne(points / starts) : "—", detail: "Across selected seasons" },
        { label: "DNF rate", value: starts ? fmtPct((100 * dnfs) / starts) : "—", detail: `${fmtNumber(dnfs)} non-classified results` },
        { label: "Seasons", value: fmtNumber(rows.length), detail: `${filters.start}–${filters.end}` },
      ]);
      const note = $("[data-chart-note='constructor-points-trend']"); if (note) note.textContent = "Ranks are by points scored that season";
      lineChart($("[data-chart='constructor-points-trend']"), rows, { yLabel: "Points", title: name, extra: (row) => `${row.wins} wins · points rank P${row.rank}` });
      reliabilityBars($("[data-chart='constructor-reliability']"), rows);
      dataTable($("[data-constructor-table]"), [
        { key: "year", label: "Season", strong: true },
        { key: "rank", label: "Rank", numeric: true, html: (row) => `P${row.rank}` },
        { key: "points", label: "Points", numeric: true, html: (row) => fmtOne(row.points) },
        { key: "wins", label: "Wins", numeric: true },
        { key: "podiums", label: "Podiums", numeric: true },
        { key: "starts", label: "Entries", numeric: true },
        { key: "dnfs", label: "DNFs", numeric: true },
        { key: "dnfRate", label: "DNF rate", numeric: true, value: (row) => (row.starts ? row.dnfs / row.starts : null), html: (row) => (row.starts ? fmtPct((100 * row.dnfs) / row.starts) : "—") },
      ], rows, { key: "year", dir: "desc" });
    }
    render(); state.rerender = render;
  }

  /* ---------- Circuits ---------- */
  function circuits() {
    const hosted = d3.rollups(state.data.races, (rows) => rows.length, (race) => race.circuitId).sort((a, b) => b[1] - a[1]);
    const filters = { id: hosted[0]?.[0], seasons: () => state.data.races.filter((race) => race.circuitId === filters.id) };
    [filters.start, filters.end] = careerSpan(filters.seasons());
    const target = $("[data-circuit-filters]");
    const circuitLabel = (id) => { const circuit = state.data.circuits.get(id); return circuit ? `${circuit.name} · ${circuit.country}` : `Circuit ${id}`; };
    function fill() {
      const span = careerSpan(filters.seasons());
      const years = [...new Set(filters.seasons().map((race) => race.year))].sort((a, b) => a - b);
      target.innerHTML =
        selectMarkup("circuit-id", "Circuit", hosted.map(([id, count]) => [id, `${circuitLabel(id)} (${count})`]), filters.id, true) +
        selectMarkup("circuit-start", "From season", years.map((year) => [year, year]), filters.start) +
        selectMarkup("circuit-end", "To season", years.map((year) => [year, year]), filters.end) +
        `<button class="filter-reset" type="button" data-reset-range>All years</button>`;
      onChange("circuit-id", (value) => { filters.id = +value; [filters.start, filters.end] = careerSpan(filters.seasons()); fill(); render(); });
      onChange("circuit-start", (value) => { filters.start = +value; orderRange(filters); fill(); render(); });
      onChange("circuit-end", (value) => { filters.end = +value; orderRange(filters); fill(); render(); });
      $("[data-reset-range]", target).addEventListener("click", () => { [filters.start, filters.end] = span; fill(); render(); });
    }
    function render() {
      const races = filters.seasons().filter((race) => race.year >= filters.start && race.year <= filters.end).sort((a, b) => a.year - b.year || a.round - b.round);
      const circuit = state.data.circuits.get(filters.id) || {};
      const winners = d3.rollups(races.map((race) => state.data.winnerByRace.get(race.raceId)).filter(Boolean), (rows) => rows.length, (row) => row.driverId).sort((a, b) => b[1] - a[1]);
      const lapRecord = d3.least(races.filter((race) => metric(race.raceId).fastestLapMs), (race) => metric(race.raceId).fastestLapMs);
      const poleWins = races.filter((race) => state.data.winnerByRace.get(race.raceId)?.grid === 1).length;
      kpis($("[data-circuit-kpis]"), [
        { label: "Races hosted", value: fmtNumber(races.length), detail: `${filters.start}–${filters.end} · ${circuit.location || ""}` },
        { label: "Most wins", value: winners[0] ? driverName(winners[0][0]) : "—", detail: winners[0] ? `${winners[0][1]} win${winners[0][1] === 1 ? "" : "s"} at this circuit` : "" },
        { label: "Fastest race lap", value: lapTime(lapRecord ? metric(lapRecord.raceId).fastestLapMs : NaN), detail: lapRecord ? `${driverName(metric(lapRecord.raceId).fastestLapDriverId)} · ${lapRecord.year}` : "Not recorded in this range" },
        { label: "Won from pole", value: races.length ? fmtPct((100 * poleWins) / races.length) : "—", detail: `${poleWins} of ${races.length} races` },
      ]);
      const pace = races.map((race) => ({ ...race, averageLapTimeMs: metric(race.raceId).averageLapTimeMs, fastestLapMs: metric(race.raceId).fastestLapMs, drivers: metric(race.raceId).driverCount })).filter((row) => row.averageLapTimeMs);
      legend("circuit-pace", [["", "Average lap"], ["b", "Fastest lap"]]);
      lineChart($("[data-chart='circuit-pace']"), pace, { yKey: "averageLapTimeMs", y2Key: "fastestLapMs", yFormat: lapTime, zeroBase: false, yLabel: "Average lap", title: circuit.name, extra: (row) => `Fastest lap ${lapTime(row.fastestLapMs)} · ${row.drivers} drivers` }, "Lap-by-lap timing is only recorded from 1996 onward.");
      const winnerRows = winners.slice(0, 8).map(([id, value]) => ({ label: driverName(id), value, tip: `<strong>${escape(driverName(id))}</strong>${value} win${value === 1 ? "" : "s"} at ${escape(circuit.name)}` }));
      if (winnerRows.length) hBars($("[data-chart='circuit-winners']"), winnerRows, { left: 112 }); else empty($("[data-chart='circuit-winners']"));
      dataTable($("[data-circuit-table]"), [
        { key: "year", label: "Year", strong: true },
        { key: "name", label: "Grand Prix" },
        { key: "winner", label: "Winner", value: (row) => winnerName(row.raceId) },
        { key: "team", label: "Constructor", value: (row) => { const winner = state.data.winnerByRace.get(row.raceId); return winner ? teamName(winner.constructorId) : "—"; } },
        { key: "grid", label: "From grid", numeric: true, value: (row) => state.data.winnerByRace.get(row.raceId)?.grid || null, html: (row) => { const grid = state.data.winnerByRace.get(row.raceId)?.grid; return grid ? `P${grid}` : "—"; } },
        { key: "drivers", label: "Drivers", numeric: true, value: (row) => metric(row.raceId).driverCount || null },
        { key: "avg", label: "Average lap", numeric: true, value: (row) => metric(row.raceId).averageLapTimeMs || null, html: (row) => lapTime(metric(row.raceId).averageLapTimeMs) },
        { key: "fast", label: "Fastest lap", numeric: true, value: (row) => metric(row.raceId).fastestLapMs || null, html: (row) => lapTime(metric(row.raceId).fastestLapMs) },
      ], races, { key: "year", dir: "desc" });
    }
    fill(); render(); state.rerender = render;
  }

  /* ---------- Generic charts ---------- */
  let gradientId = 0;
  /** Line chart over seasons. options: yKey, y2Key (secondary dashed series), yFormat, zeroBase, yLabel, title, extra(row). */
  function lineChart(target, rows, options, emptyMessage) {
    const { yKey = "points", y2Key, yFormat = fmtOne, zeroBase = true, yLabel, title, extra } = options;
    if (!rows.length) return empty(target, emptyMessage);
    const { root, svg, width, height } = svgFor(target, 290, { top: 14, right: 20, bottom: 34, left: 62 });
    const [minYear, maxYear] = d3.extent(rows, (row) => row.year);
    const x = d3.scaleLinear().domain(minYear === maxYear ? [minYear - 1, maxYear + 1] : [minYear, maxYear]).range([0, width]);
    const values = rows.flatMap((row) => [row[yKey], y2Key ? row[y2Key] : null]).filter((value) => Number.isFinite(value) && value > 0);
    const [low, high] = d3.extent(values);
    const pad = (high - low || high || 1) * 0.08;
    const y = d3.scaleLinear().domain(zeroBase ? [0, high || 1] : [low - pad, high + pad]).nice().range([height, 0]);
    const id = `area-gradient-${++gradientId}`;
    const gradient = root.append("defs").append("linearGradient").attr("id", id).attr("x1", 0).attr("x2", 0).attr("y1", 0).attr("y2", 1);
    gradient.append("stop").attr("offset", "0%").attr("class", "area-stop-top");
    gradient.append("stop").attr("offset", "100%").attr("class", "area-stop-bottom");
    svg.append("g").attr("class", "grid").call(d3.axisLeft(y).ticks(5).tickSize(-width).tickFormat(""));
    if (zeroBase) svg.append("path").datum(rows).attr("fill", `url(#${id})`).attr("d", d3.area().x((row) => x(row.year)).y0(height).y1((row) => y(row[yKey])).curve(d3.curveMonotoneX));
    if (y2Key) svg.append("path").datum(rows.filter((row) => row[y2Key] > 0)).attr("class", "line").attr("stroke-dasharray", "4 4").style("stroke", "var(--series-b)").style("stroke-width", 2).attr("d", d3.line().x((row) => x(row.year)).y((row) => y(row[y2Key])).curve(d3.curveMonotoneX));
    const path = svg.append("path").datum(rows).attr("class", "line").attr("d", d3.line().x((row) => x(row.year)).y((row) => y(row[yKey])).curve(d3.curveMonotoneX));
    const length = path.node().getTotalLength?.() || 0;
    if (length) path.attr("stroke-dasharray", `${length} ${length}`).attr("stroke-dashoffset", length).transition().duration(ms(700)).ease(d3.easeCubicOut).attr("stroke-dashoffset", 0).on("end", () => path.attr("stroke-dasharray", null));
    const dots = svg.selectAll(".dot").data(rows).join("circle").attr("class", "dot").attr("cx", (row) => x(row.year)).attr("cy", (row) => y(row[yKey])).attr("r", rows.length > 30 ? 3 : 4.5);
    hover(dots, (row) => `<strong>${escape(title)} · ${row.year}</strong>${escape(yLabel)}: ${yFormat(row[yKey])}${extra ? `<br><span class="muted">${escape(extra(row))}</span>` : ""}`);
    const tickCount = Math.min(10, Math.max(1, maxYear - minYear + 1));
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x).ticks(tickCount).tickFormat((d) => (Number.isInteger(d) ? d : "")), "x-base");
    drawAxis(svg.append("g"), d3.axisLeft(y).ticks(5).tickFormat(yFormat));
  }
  function finishBars(target, results) {
    if (!results.length) return empty(target);
    const keys = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "11+", "NC"];
    const buckets = keys.map((key) => ({ key: String(key), value: results.filter((row) => (key === "NC" ? !row.position : key === "11+" ? row.position > 10 : row.position === key)).length }));
    const { svg, width, height } = svgFor(target, 280, { top: 16, right: 8, bottom: 32, left: 32 });
    const x = d3.scaleBand().domain(buckets.map((row) => row.key)).range([0, width]).padding(0.22);
    const y = d3.scaleLinear().domain([0, d3.max(buckets, (row) => row.value) || 1]).nice().range([height, 0]);
    svg.append("g").attr("class", "grid").call(d3.axisLeft(y).ticks(5).tickSize(-width).tickFormat(""));
    const bars = svg.selectAll("rect").data(buckets).join("rect").attr("class", (row) => (row.key === "NC" || row.key === "11+" ? "bar-alt" : "bar")).attr("x", (row) => x(row.key)).attr("width", x.bandwidth()).attr("rx", 2);
    animateHeight(bars, (row) => y(row.value), (row) => height - y(row.value), height);
    hover(bars, (row) => `<strong>${row.key === "NC" ? "Not classified (DNF/DSQ)" : row.key === "11+" ? "Finished 11th or lower" : `Finished P${row.key}`}</strong>${row.value} race${row.value === 1 ? "" : "s"} · ${fmtPct((100 * row.value) / results.length)}`);
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x).tickSize(0).tickPadding(8)).selectAll("text").style("font-size", "10px");
    drawAxis(svg.append("g"), d3.axisLeft(y).ticks(5));
  }
  function reliabilityBars(target, rows) {
    const data = rows.filter((row) => row.starts).map((row) => ({ year: row.year, value: (100 * row.dnfs) / row.starts, dnfs: row.dnfs, starts: row.starts }));
    if (!data.length) return empty(target);
    const { svg, width, height } = svgFor(target, 280, { top: 14, right: 10, bottom: 34, left: 40 });
    const x = d3.scaleBand().domain(data.map((row) => row.year)).range([0, width]).padding(0.2);
    const y = d3.scaleLinear().domain([0, d3.max(data, (row) => row.value) || 1]).nice().range([height, 0]);
    const average = (100 * sum(data, (row) => row.dnfs)) / sum(data, (row) => row.starts);
    svg.append("g").attr("class", "grid").call(d3.axisLeft(y).ticks(5).tickSize(-width).tickFormat(""));
    const bars = svg.selectAll("rect").data(data).join("rect").attr("class", "bar").attr("x", (row) => x(row.year)).attr("width", x.bandwidth()).attr("rx", 2);
    animateHeight(bars, (row) => y(row.value), (row) => height - y(row.value), height);
    hover(bars, (row) => `<strong>${row.year}</strong>${fmtPct(row.value)} DNF rate<br><span class="muted">${row.dnfs} of ${row.starts} entries</span>`);
    svg.append("line").attr("class", "ref-line").attr("x1", 0).attr("x2", width).attr("y1", y(average)).attr("y2", y(average));
    svg.append("text").attr("class", "chart-label").attr("x", width).attr("y", y(average) - 5).attr("text-anchor", "end").text(`avg ${fmtPct(average)}`);
    const step = Math.ceil(data.length / 8);
    drawAxis(svg.append("g").attr("transform", `translate(0,${height})`), d3.axisBottom(x).tickSize(0).tickPadding(8).tickValues(data.filter((_, i) => i % step === 0).map((row) => row.year)).tickFormat(d3.format("d")));
    drawAxis(svg.append("g"), d3.axisLeft(y).ticks(5).tickFormat((d) => `${d}%`));
  }

  /* ---------- Start ---------- */
  async function start() {
    setupTheme();
    if (standalone) {
      const initial = location.hash.slice(1);
      state.page = PAGES[initial] ? initial : state.page;
      window.addEventListener("hashchange", () => mount(location.hash.slice(1)));
      mount(state.page);
    } else {
      renderNav();
    }
    // Sources are static attribution and should remain available before data is built.
    if (state.page === "sources") {
      document.body.classList.remove("is-loading");
      return;
    }
    try {
      state.data = await loadData();
    } catch (error) {
      document.body.classList.remove("is-loading");
      showSetup(error);
      return;
    }
    document.body.classList.remove("is-loading");
    mount(state.page);
  }
  start();
})();
