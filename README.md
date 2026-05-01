# ash

a small, fast terminal emulator for linux.  
named after the tree — light, strong, does one thing well.

built on GTK3 + VTE. all settings live in `config.h`. edit and recompile.

## install deps (arch)

```sh
sudo pacman -S gtk3 vte3 pkgconf gcc
```

## build

```sh
make
```

## install system-wide

```sh
sudo make install
```

installs the binary, icon, and `.desktop` file so ash shows up in your app launcher.

## usage

```
ash [OPTIONS] [-- COMMAND [ARGS…]]
```

| flag                        | what it does                                   |
|-----------------------------|------------------------------------------------|
| `-e COMMAND [ARGS…]`        | run a command instead of the shell             |
| `--hold`                    | keep the window open when the command exits    |
| `--title TEXT` / `-T TEXT`  | set a fixed window title (not overridden by shell) |
| `--working-directory DIR`   | start in a specific directory                  |

examples:

```sh
ash                               # open a shell (default)
ash -e htop                       # open htop; close when it exits
ash --hold -- bash -c 'ls -la'    # run ls, then drop into $SHELL
ash --title "logs" -e journalctl -f
```

## terminal identification

ash sets the following environment variables for every child process:

| variable       | value           | used by                              |
|----------------|-----------------|--------------------------------------|
| `TERM`         | `xterm-256color`| broad compatibility                  |
| `TERM_PROGRAM` | `ash`           | fastfetch, neofetch, oh-my-posh, … |
| `COLORTERM`    | `truecolor`     | signals 24-bit RGB color support     |
| `VTE_VERSION`  | e.g. `7400`     | shell integrations, vim, etc.        |

fastfetch will now show **ash** as your terminal.

## customization

open `config.h`. everything is there:

| option                | what it does                                       |
|-----------------------|----------------------------------------------------|
| `FONT`                | font family + size (pango string)                  |
| `FONT_SCALE`          | multiplier on top of the size                      |
| `PADDING`             | inner padding (css shorthand)                      |
| `CURSOR_SHAPE`        | `BLOCK`, `IBEAM`, or `UNDERLINE`                   |
| `CURSOR_BLINK`        | `ON`, `OFF`, or `SYSTEM`                           |
| `BOLD_IS_BRIGHT`      | `1` = bold uses bright palette; `0` = bold weight only |
| `WORD_CHAR_EXCEPTIONS`| chars included in double-click word selection      |
| `SCROLLBACK_LINES`    | lines kept in the scroll buffer                    |
| `HIDE_DECORATIONS`    | `1` = borderless window                            |
| `DEFAULT_WIDTH/HEIGHT`| initial window size                                |
| `BG_ALPHA`            | `1.0` = opaque, `0.0` = fully transparent          |
| `ALWAYS_SHOW_TABS`    | `1` = always show tab bar                          |
| `COLOR_BG/FG`         | background and foreground                          |
| `COLOR_*`             | full 16-color palette                              |

four themes included: **tokyo night dark** (default), **gruvbox dark**, **dracula**, **solarized light**.  
uncomment a block in `config.h` to switch.

## keybindings

| key                  | action                    |
|----------------------|---------------------------|
| `Ctrl+Shift+C`       | copy selection            |
| `Ctrl+Shift+V`       | paste                     |
| `Ctrl+=` / `Ctrl++`  | increase font size        |
| `Ctrl+-`             | decrease font size        |
| `Ctrl+0`             | reset font size           |
| `Shift+PgUp/PgDn`    | scroll half page          |
| `Ctrl+Shift+↑/↓`     | scroll 3 lines            |
| `Ctrl+Shift+T`       | new tab                   |
| `Ctrl+Shift+W`       | close tab                 |
| `Ctrl+PgUp/PgDn`     | previous / next tab       |
| `Alt+1–9`            | jump to tab N             |

## right-click menu

right-click anywhere in the terminal for a context menu:  
**Copy**, **Paste**, and (if the cursor is over a URL) **Open: …**

## new tab cwd

new tabs automatically inherit the **current working directory** of the active tab's foreground process (reads `/proc/<pid>/cwd` on Linux).
