# tym

`tym` is a tiny VTE-based terminal emulator written in C and configurable by lua.

## Motivation

[`termite`](https://github.com/thestinger/termite) is very good but `vte-ng` is conflict with `vte` and its slection mode feature is not needed for me because tmux has almost same (or a bit better) one.

So I want a terminal emulator which is
- original VTE-based
- configurable with text file, which can be managed by personal so called `dotfiles` repository like [this](https://github.com/endaaman/dotfiles)

but such a terminal does not exist so I created.

## Dependencies

- [GTK+3](https://www.gtk.org/)
- [VTE](https://github.com/GNOME/vte)
- [lua](https://www.lua.org/)

## Configration

`tym` reads `$XDG_CONFIG_HOME/tym/config.lua` if it exists. Available options are below.

- `shell`  
  Set your prefered shell. (default: check `$SHELL`, if not set, check `vte_get_user_shell()` and if it NULL use `'/bin/sh'`  )

- `font`  
  Set `'FAMILY-LIST [SIZE]'` like `'Ubuntu Mono 12'`. This option value is internally passed to [`pango_font_description_from_string()`](https://developer.gnome.org/pango/stable/pango-Fonts.html#pango-font-description-from-string). If set `''`(empty string), system default fixed width font will be used. (default: `''`)

- `cursor_blink`  
  `'system'`, `'on'` or `'off'` are available. (default: `'system'`)

- `cjk_width`  
  `'narrow'` or `'wide'` are available (default: `'narrow'`)

- `color_foreground`, `color_background`, `color_cursor`, `color_cursor_foreground`, `color_highlight`, `color_highlight_foreground` and `color_0` from `color_255`  
  you can specify standard color string like `'#f00'`, `'#ff0000'` or `'red'` whick can be parsed by `gdk_rgba_parse()`. If set `''`(empty string), VTE default color will be used.  (default: `''`)


### Example confing

Here is an example.

```lua
config.shell = '/bin/fish'
config.font = 'DejaVu Sans Mono 10'
config.cursor_blink_mode = 'system'
config.cjk_width = 'narrow'

config.color_foreground        = '#d0d0d0'
config.color_foreground_bold   = '#d0d0d0'
config.color_cursor            = '#d0d0d0'
config.color_cursor_foreground = '#181818'
config.color_background        = '#181818'

config.color_0  = '#151515'
config.color_1  = '#ac4142'
config.color_2  = '#90a959'
config.color_3  = '#f4bf75'

-- SNIP
```

## Key bindings

| Key              | Action                |
|:---------------- |:--------------------- |
| Ctrl + Shift + v | Pasete from clipboard |
| Ctrl + Shift + r | Reload config file    |

## Build and install

Download source from [release page](https://github.com/endaaman/tym/releases), unarchive it and

```
$ ./configure --prefix=/usr/local/
$ make
$ sudo make install
```

## TODOs

- Configurable features
  - Geometry
  - Custom key bindings
    - Reload config file
    - Paste clipboard

## License

MIT
