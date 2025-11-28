# Grid

A modern, minimal text editor for the terminal. Built in C with a focus on simplicity and Mac Terminal compatibility.

## Features

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+S` | Save file |
| `Ctrl+Q` | Quit (warns if unsaved) |
| `Ctrl+F` | Find/Search |
| `Ctrl+C` | Copy selection |
| `Ctrl+X` | Cut selection |
| `Ctrl+V` | Paste |
| `Ctrl+A` | Select all |
| `Ctrl+/` | Toggle line comment |
| `Shift+Arrow` | Select text |
| `Arrow keys` | Navigate |
| `Home/End` | Start/End of line |
| `Page Up/Down` | Scroll page |
| `Enter` | New line |
| `Backspace` | Delete before cursor |
| `Delete` | Delete at cursor |
| `Tab` | Insert tab character |

### Core Features

- **Modern UI**: Clean status bar showing filename, modification status, cursor position
- **Text Selection**: Shift+Arrow keys for selecting text with visual highlighting
- **Clipboard**: Internal copy/cut/paste system
- **Search**: Interactive search with Enter to find next
- **Comment Toggle**: Quick comment/uncomment for C-style code
- **Viewport Scrolling**: Handles files larger than the terminal window
- **Window Resize**: Automatically adjusts to terminal size changes

## Building

```bash
make
```

## Usage

```bash
./grid <filename>
```

## Requirements

- GCC compiler
- POSIX-compatible terminal (tested on Mac Terminal)

## Technical Details

### Architecture

- `main.c` - Entry point and main loop
- `common.c/h` - Shared globals and utilities
- `types.h` - Data structure definitions
- `terminal.c/h` - Raw terminal mode handling
- `buffer.c/h` - Text buffer management
- `display.c/h` - Screen rendering with efficient updates
- `input.c/h` - Keyboard input and command processing

### Code Style

Follows Linux kernel coding style:
- Tab indentation
- K&R brace style
- 80-column limit where practical

## Resources

- [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/) - Inspiration for terminal handling
- [Advanced Programming in the UNIX Environment](https://www.apuebook.com/) - Reference for terminal control
