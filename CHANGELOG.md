# Changelog

## 0.20

What's new:

- List content is now cached and there is an explicit command to refresh it

## 0.19

What's new:

- Added option `install_psp_psx_location` to install games to other partitions
  than `ux0:`

Removed features:

- pkgj will not try to load pkgi.txt anymore

## 0.18

What's new:

- PSP games now default to an ISO installation. Add `install_psp_as_pbp 1` in
  your config.txt to install EBOOT.PBP files.
- Added support for PC Engine games.

## 0.17

What's new:

- PSP games are now installed as EBOOT.pbp files. They are smaller and faster
  to install. You will need to install the npdrm_free plugin in VSH to make
  them work.
- Added support for PSP-Mini games
- Increased menu size

## 0.16

What's new:

- Added automatic creation of PSP and PSX directories
- Improved installation error reporting

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
