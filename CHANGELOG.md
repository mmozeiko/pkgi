# Changelog

## 0.43

What's new:

- Database is now stored as plain files, refresh has become much faster

## 0.42

What's new:

- When in Japanese mode, O now works for confirm in game view
- Reduced size of the game view window to show the progress bar below

Bug fixes:

- Fixed cancelation of DLC and PSP and PSX games

## 0.41

Bug fixes:

- Do not crash if ur0:app does not exist

## 0.40

What's new:

- It is now possible to filter by installed games
- When changing mode, the list scrolls back to the top

## 0.39

Bug fixes:

- Fixed installation of DLCs and PSX games

## 0.38

What's new:

- There is a new window when clicking on a game which makes it easier to install
    games and compatibility packs
- Added a diagnosis about game installation that helps detecting
    game/compatibility pack version mismatch
- Download cancelation is now more reactive

Removed features:

- PKGj can't download game updates anymore as they were outdated. Use the
    livearea to update your games.

## 0.37

What's new:

- PKGj will now use `ux0:pkgj` and `ur0:pkgj` as configuration and temporary
    file folder. If the configuration file can't be found, it will fallback to
    `pkgi` folders

Bug fixes:

- Fixed crash on application start or on download cancel

## 0.36

What's new:

- It is now possible to download the new version of pkgj when one becomes
    available

## 0.35

Bug fixes:

- Fixed bad_alloc error that happened when installing some PSP games

## 0.34

What's new:

- A message will appear upon start when a new pkgj version is available (unless
    `no_version_check 1` is in the config file)
- Dialog messages have been reworked, they include word wrapping for error
    messages that are too long
- The menu's refresh item now refreshes all lists at once, including
    compatibility packs
- "Show updates" has been renamed to "Show legacy updates" to reflect the fact
    that they are not the last available version. Use the livearea to install
    the last version of a game.

## 0.33

What's new:

- Added PSM support

## 0.32

What's new:

- Added support for downloading patches compatibility packs. No changes to
    config.txt
- Added app version and required firmware on game update list
- When updates are installed, they now correctly appear as a white bullet
- New LiveArea design from JesterIOS

Bug fixes:

- Sort by date option is now correctly saved to configuration

## 0.31

What's new:

- Added support for downloading compatibility packs with the `url_comppack`
    configuration option
- All games are now listed, reguardless of firmware version

## 0.30

What's new:

- Temporary data is now also deleted when deleting resume data after a resume
    failure
- Implemented update of Shell's database when installing an update so that
    livearea doesn't say that an update is available when it's already installed

Bug fixes:

- Resume data is now deleted after a successful installation to avoid an error
    when reinstalling

## 0.29

Bug fixes:

- Fixed error when unsafe mode is not activated
- Fixed downloading of games that contain a single file bigger than 2GB

## 0.28

What's new:

- Resume data is now deleted automatically when resuming fails

Bug fixes:

- Downloaded package integrity is now correctly checked

## 0.27

What's new:

- Improved various errors
- Current mode now appears in the top bar
- Added support for downloading packages without digest

## 0.26

What's new:

- ux0:patch is created on first update installation if it doesn't exist yet

Bug fixes:

- Search feature now works

## 0.25

Bug fixes:

- Fix error messages from previous version

## 0.24

What's new:

- Download resume feature was restored (except when downloading PSP games as ISO
  files)
- Made a couple errors more explicit

## 0.23

Bug fixes:

- Sorting by size is now fixed

## 0.22

What's new:

- Allowed sorting by last modification date
- Games that are not supported on the current firmware are now hidden
- When the list fails to download, there is no more need to exit pkgj

Bug fixes:

- Fixed crash when pressing X on an empty list

## 0.21

What's new:

- Percentage and speed are now shown during download

Bug fixes:

- When changing list, the list scrolls back up

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
