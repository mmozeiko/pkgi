# Changelog

## 0.15

What's new:

- Further improved HTTP error reporting

## 0.14

What's new:

- Added support for PSP games
- Improved HTTP error reporting

## 0.13

Bug fixes:

- It is now possible to download more than 4 packages

## 0.12

What's new:

- More design in LiveArea screen from @JesterIOS
- Added download error reporting to user
- Do not rely on TSV column header anymore to determine the mode

Removed features:

- Removed support for old pkgi database format

## 0.11

What's new:

- Replaced LiveArea graphics by images from @JesterIOS
- Kept compatibility with old update TSV format

## 0.10

What's new:

- Updated to new TSV format

Bug fixes:

- Fixed bug that would hide the last line of TSV files
- Fixed installation of certain games

Removed features:

- Can't resume downloads anymore (will be restored later)

## 0.09

What's new:

- Added support for PSX games installation
- Disabled version check because of a SSL issue preventing me from testing the
  code

## 0.08

What's new:

- Items with missing URL or zRIF string are now discarded (instead of shown on a
  red background)

Bug fixes:

- Fixed crash when refreshing updates
- Fixed bug where some item where shown as installed when they were not or
  "corresponding game not installed" when it actually is

## 0.07

What's new:

- It is now possible to cancel downloads by pressing X once again on a package
- The current download is now shown at the bottom
- A progress bar of the current download is now shown at the bottom
- Games that can't be installed because the URL or the zRIF string is missing
  are now shown on red background

Bug fixes:

- Fixed a crash when trying to download a package that has no URL
