# Intro

Raider is a terminal file browser inspired by
[ranger](https://github.com/ranger/ranger) and
[nnn](https://github.com/jarun/nnn), it is written in C, it tries to be minimal
and fast, there is no configuration. It is still very much **work in progress**.


It has very little functionality: it is basically a file browser with preview,
instead of providing functions to operate on files you just drop into a shell
and work from there.

# Installation

To compile just do

    $ mkdir build
    $ cd ./build
    $ cmake ..
    $ make

then just copy the `raider` executable to a directory in your PATH.

There are no specific dependencies, preview try to use whatever is available on
your system.

# Usage

Raider is a two pane file browser with a look and feel similar to ranger. On the
left is the current directory, on the right the preview.

You can navigate with the cursor arrows or the `hjkl` keys

- `s` drops you into a shell in the current directory
- `e` opens the current file/directory in an editor (vim or emacs depending on
  what's avaliable)
- `tT` sorts directory by *ctime* (ascending/descending)
- `nN` sorts directory by *name* (ascending/descending)
- `sS` sorts directory by *size* (ascending/descending)
- `/` use fzf (if it's installed) to search for files/directories
- `space` select files

a word about selection: you cannot do much with selected files *inside* raider
but when you drop into a shell selected files are available in a file this way
if for example you want to delete them (or move, copy, ...) you just do
something like

    $ rm `cat ~/.raider-sel-xxx`
    $ cp `cat ~/.raider-sel-xxx` /some/dest
    $ etc etc

# Previews

There are a few preview options, they are selected with the `-p` option:

- `x11`: if you are using X and have the W3M text browser (and `w3mimgdisplay`)
  installed raider will use it to display images in you terminal

- `sixel`: if your terminal supports displaying sixel images (see
  [https://www.arewesixelyet.com/](are we sixel yet?)) and you have
  [libsixel](https://github.com/saitoha/libsixel) installed raider will try to
  display images directly on the terminal (this works also in ssh)

- `chafa`: if you have [chafa](https://github.com/hpjansson/chafa/) installed
  you can preview images everywhere (approximately)

- `none`: no preview, try to convert file to text

Note that if you have xterm and you want to view sixel images probably you'll
have to add the following lines to yout `.Xresources` file:

    xterm*decTerminalID:     vt340
    xterm*sixelScrolling:    1
    xterm*sixelScrollsRight: 1
    xterm*numColorRegisters: 256

also for reason still not entirely clear to me sixel preview work best in tmux.
