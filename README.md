Abuse README
============

This is a fork of the original Abuse SDL port from <http://abuse.zoy.org/>. It
switches the build system to CMake to make crossplatform building easier, and
ports the original SDL version to SDL2. This version also removes some of the
content that was in the original abuse.zoy.org version (namely, fRaBs), as the
licensing information for it had some conflicting information.

Restoring fRaBs should be as simple as copying the data file over to the
install directory, but they're not being included in the GitHub "official"
builds. Likewise, the sound and music files are currently not included, as
it's unclear if they were ever really allowed to be distributed.

Placing those files into the "sfx" and "music" directories (or copying them
into the installed data directory) should re-enable sound and music.

----

1. Introduction
2. Additional Features
3. Requirements
4. Running Abuse
5. Configuration
6. Installing the datafiles
7. Notes
8. Special Thanks
9. Feedback

----

## 1. INTRODUCTION

Welcome to Abuse, the port of the classic game Abuse to the Simple
DirectMedia Layer. Abuse was originally developed by Crack dot Com and
released in 1995 for MS-DOS. A Linux version was also made available at
a later date. It had a few limitations the most restrictive of which was
that it only ran on an 8-bit display, and only in a window.

The version of Abuse will run at any color depth and supports fullscreen mode,
as well as many other new features. It should also be more portable and
hopefully run on a variety of *nix variants, as well as Windows and macOS.

## 2. ADDITIONAL FEATURES

Abuse has the following extra features over the original:

  * Runs at a screen bit depth of 8, 16, 24 or 32.
  * Fullscreen display.
  * Scaling by any amount (eg. 2, 3 or 4)
  * SDL2 support for hardware scaling and anti-aliasing.
  * Stereo sound with panning.
  * Mouse wheel support for changing weapons.
  * Customizable keys.

## 3. REQUIREMENTS

Abuse has the following requirements:

  * SDL2 2.0.3 or above.
  * SDL_mixer 2.0.0 or above.

## 4. RUNNING ABUSE

Generally, just launch it however is appropriate for your platform after
running the `install` build.
If Abuse has been installed properly, the command:

    abuse

will start the game.

The following command-line switches can be used:

    -datadir <arg>    Set the location of the datafiles
    -edit             Start in editor mode
    -f <arg>          Load the map file named <arg>
    -fullscreen       Enable fullscreen mode
    -antialias        Enable anti-aliasing
    -lisp             Start in lisp interpreter mode
    -mono             Disable stereo sound
    -nodelay          Run at maximum speed
    -nosound          Disable sound
    -scale <arg>      Scale by <arg> amount

## 5. CONFIGURATION

Abuse also has a configuration file where these options can be set
instead of using the command line. The file "abuserc" will be created in
your ~/.abuse directory the first time Abuse is run.

Lines starting with a ';' are comments.
Setting an option to '1' turns it on, and '0' turns it off.

To change the keys used in the game, simply type the key after the option.
The following special keys can also be used:

| Code                          | Represents
|-------------------------------|-----------------------
| `LEFT`, `RIGHT`, `UP`, `DOWN` | Cursor keys and keypad.
| `CTRL_L`, `CTRL_R`            | Left and right Ctrl keys.
| `ALT_L`, `ALT_R`              | Left and right Alt keys.
| `SHIFT_L`, `SHIFT_R`          | Left and right Shift keys.
| `F1` - `F10`                  | Function keys 1 through 10.
| `TAB`                         | Tab key.
| `BACKSPACE`                   | Backspace key.
| `ENTER`                       | Enter key
| `INSERT`, `DEL`               | Insert and Delete keys.
| `PAGEUP`, `PAGEDOWN`          | Page Up and Page Down keys.
| `CAPS`, `NUM_LOCK`            | Caps-Lock and Num-Lock keys.
| `SPACE`                       | Spacebar.

The default key settings are as follows:

| Action      | Bound to
|-------------|---------
| Left        | Left arrow, A
| Right       | Right arrow, D
| Up/Jump     | Up arrow, W
| Down/Use    | Down arrow, S
| Prev Weapon | Left or Right Ctrl
| Next Weapon | Insert

The mouse always controls your aim, with Left button for fire and
Right button for special.  The mouse wheel can be used for changing weapons.

## 6. INSTALLING THE DATAFILES

This repository contains the majority of the data files. The only data currently
missing are the sound effects and the music as they weren't released into the
public domain.

You can still grab them off of <http://abuse.zoy.org/> and extract them into
the data directory, and they will be used by the build script.

Under Windows and macOS, Abuse looks for the data files in a path relative to
the executable. Using the CMake `install` and `publish` targets will set up
the directory in the correct way. (See BUILDING.md for more information about
that.)

Under Linux, by default, Abuse expects the datafiles to be installed in the
following location:

    /usr/local/share/games/abuse

However it is possible to change this and tell Abuse where to find the
files if they are in another location.

The location can be set when Abuse is compiled. See the INSTALL.md file for
instructions on how to do this.

You can also specify the location with the -datadir argument when
Abuse is run. See section 4 above.

Finally, the location can be set in the configuration file. See section
5 above.

## 7. NOTES

### Scaling:
  Scaling is still experimental and not very fast.  Try scaling by different
  amounts to see which ones work the best.

## 8. SPECIAL THANKS

Go to Jonathan Clark, Dave Taylor and the rest of the Crack Dot Com team
for making the best 2D platform shooter ever, and then releasing the
code that makes Abuse possible.

Also, thanks go to Jonathan Clark for allowing Anthony to distribute the
original datafiles with Abuse.

Thanks also to everyone who has contributed ideas, bug reports and patches.
See the AUTHORS file for details.

## 9. FEEDBACK

For this fork, please use the [GitHub page](https://github.com/Xenoveritas/abuse)
if you have any questions, comments, or find bugs.

The original code was taken from <http://abuse.zoy.org/>, but any issues on
non-Linux platforms should be directed to the GitHub page.

----

Have fun!
