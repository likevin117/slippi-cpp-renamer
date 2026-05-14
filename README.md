# slippi-cpp-renamer

A C++ command-line tool for renaming and analyzing [Slippi](https://slippi.gg) `.slp` replay files for Super Smash Bros. Melee.

## What it does

Renames `.slp` replay files to a clean, human-readable format using metadata extracted from the binary file header:

```
20240322T231153 - Falco (KillemDafoe) vs Fox (Red,kev11) - Final Destination.slp
```

Extracted per file:
- Timestamp (from filename or metadata)
- Stage name
- Per-player: character, costume color, nametag, netplay display name

## Usage

```
./slippi-parser [-n] [-r] <file.slp | directory>
```

| Flag | Description |
|------|-------------|
| `-n` | Dry run — print what would be renamed without making changes |
| `-r` | Recurse into subdirectories |

### Examples

Rename a single file:
```
./slippi-parser Game_20240322T231153.slp
```

Dry-run a whole folder:
```
./slippi-parser -n /path/to/replays
```

Recursively rename all replays under a directory:
```
./slippi-parser -r /path/to/replays
```

## Planned features

- **Replay filtering** — search a folder of replays by character, stage, or opponent
- **Session stats** — print win/loss record, most played characters and stages across a session

## Why C++

Slippi replay files use a custom binary format (UBJSON wrapper + raw event stream). Parsing them in C++ produces a single native binary with no runtime dependencies — no Node.js, no Python install required. Just download and run.

The parser only reads the file header and metadata tail, skipping the large frame data block in the middle. This makes batch processing fast even on large replay collections.

A JavaScript-based renamer already exists ([slippi-renamer](https://github.com/mtimkovich/slippi-renamer)). This project is a C++ alternative focused on performance and zero-dependency distribution.

## Build

Requires a C++17 compiler.

```
make
```

## File format

`.slp` files begin with a UBJSON wrapper containing a `raw` byte array of game events followed by a `metadata` object. The raw section starts with an Event Payloads block (command `0x35`) that defines the size of each subsequent event type, followed by a Game Start event (`0x36`) containing full match metadata.

Reference: [Slippi Replay Spec](https://github.com/project-slippi/slippi-wiki/blob/master/SPEC.md)
