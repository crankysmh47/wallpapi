# Changelog

All notable changes to Wallpapi are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.0.0] - 2025-05-24

### Added

- Minimal live wallpaper engine (`wp-engine`) using Windows Media Foundation
- Full `wp-cli` with list, set, config, toggles, add, open, next, random, pause/resume
- Graphical control panel (`wp-ui`) for non-terminal users
- One-command installer (`install.ps1`) with license acceptance, PATH, and startup registration
- Portable packaging script (`package.ps1`) and GitHub Release workflow
- MIT License, End User License Agreement, and third-party notices
- Automated install smoke tests in CI (`test-install.ps1`)

### Changed

- Stripped legacy libmpv-based engine in favor of MF-based playback

[1.0.0]: https://github.com/crankysmh47/wallpapi/releases/tag/v1.0.0
