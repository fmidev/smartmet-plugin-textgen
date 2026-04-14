# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with this repository.

## What this is

SmartMet Server plugin that generates natural-language weather forecast text (Finnish, Swedish, English) from meteorological data. HTTP requests to `/textgen` specify a location and product configuration; the plugin returns formatted forecast prose.

## Build commands

```bash
make                # Build textgen.so (shared library plugin)
make clean          # Clean build artifacts
make format         # Run clang-format on source
make test           # Run integration tests (requires geonames/gis engines + test data)
make rpm            # Build RPM package
```

## Testing

Tests are integration tests that spin up a SmartMet reactor with the plugin loaded. There are no unit tests.

```bash
cd test && make test    # Runs smartmet-plugin-test against /textgen endpoint
```

**How it works**: `smartmet-plugin-test` starts a reactor (config: `test/cnf/reactor.conf`), sends HTTP requests from `test/input/*.get` files, and diffs responses against `test/output/` golden files.

**Test input format**: Raw HTTP GET requests, e.g.:
```
GET /textgen?formatter=plainlines&area=Uusimaa&product=iltaan_asti&forecasttime=200808060800 HTTP/1.0
```

**Database requirement**: Tests need a PostgreSQL geonames database. In CI, a local instance is created automatically. Locally, `test/cnf/geonames.conf.in` and `gis.conf.in` are templates that get processed into `.conf` files — the host is `smartmet-test` by default (overridden in CI to a local temp DB).

## Architecture

### Engine dependencies

The plugin requires three SmartMet engines (loaded at runtime by the server):

- **Querydata engine** — provides forecast model data that the text generation library reads
- **Geonames engine** — resolves location names, geocodes, and reverse-geolocation
- **GIS engine** — retrieves PostGIS geometries for area definitions, coordinate transforms

### Request flow

```
HTTP Request → Plugin::requestHandler()
  → parse location (area name / bbox / WKT / lon,lat / place geoids)
  → resolve via Geonames + GIS engines
  → TextGen::TextGenerator (from smartmet-library-textgen) produces text
  → TextGen::Dictionary (file-based or database-backed, with geonames integration)
  → TextGen::TextFormatter (plainlines / html / css)
  → cached response (60-second TTL)
```

### Key source files

All source is in `textgen/`:

- **Plugin.cpp/h** — HTTP handler, request parsing, location resolution, caching, response formatting
- **Config.cpp/h** — Configuration management including product configs, PostGIS identifiers, geometry storage, hot-reload via DirectoryMonitor. `Config` holds global state; `ProductConfig` holds per-product settings (formatter, language, forecast data sources, area timezones, masks)
- **FileDictionaryPlusGeonames.cpp/h** — File-based dictionary extended with geonames engine for location-aware lookups
- **FileDictionariesPlusGeonames.cpp/h** — Multi-language variant of the above
- **DatabaseDictionariesPlusGeonames.cpp/h** — Database-backed (PostgreSQL/MySQL) dictionary with geonames integration

### Configuration

Uses libconfig++ format. Main config is `textgen.conf`, which references per-product config files:

- `dictionary` setting chooses backend: `multifileplusgeonames`, `multimysqlplusgeonames`, or `multipostgresqlplusgeonames`
- `product_config` maps product names to config files (each defines formatter, language, forecast data sources, area definitions, unit formats)
- Product configs are hot-reloaded when files change on disk

### HTTP API parameters

`GET /textgen?area=Uusimaa&product=iltaan_asti&forecasttime=200808060800&formatter=plainlines&language=fi`

Location can be specified as: `area=`, `bbox=`, `places=` (with geoids), `lonlat=`, or `wkt=`.

## Namespace

`SmartMet::Plugin::Textgen` — follows the SmartMet convention where plugins live under `SmartMet::Plugin::<Name>`.
