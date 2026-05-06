# slippi-cpp-renamer

A C++ command-line tool for parsing, renaming, and analyzing [Slippi](https://slippi.gg) `.slp` replay files for Super Smash Bros. Melee.

## What it does

Reads the binary header of a `.slp` replay file and extracts game metadata:

- Slippi version
- Stage
- Per-player info: character, display name, stock count, costume

## Planned features

- **Batch rename** — rename a folder of replays to a clean, human-readable format (e.g. `2024-01-15_Fox-vs-Marth_Battlefield.slp`)
- **Replay filtering** — search a folder of replays by character, stage, or opponent
- **Session stats** — print win/loss record, most played characters and stages across a session

## Why C++

Slippi replay files use a custom binary format (UBJSON wrapper + raw event stream). Parsing them in C++ produces a single native binary with no runtime dependencies — no Node.js, no Python install required. Just download and run.

A JavaScript-based renamer already exists ([slippi-renamer](https://github.com/mtimkovich/slippi-renamer)). This project is a C++ alternative focused on performance and zero-dependency distribution.

## Build

Requires a C++17 compiler.

```
make
```

## Usage

```
./slippi-parser <file.slp>
```

Example output:
```
Version:  3.17.0
Stage:    Battlefield (0x1F)
Teams:    No
PAL:      No

Port 1:   Kevlar  |  Fox  |  type=Human  |  stocks=4  |  costume=2
Port 4:   nxr     |  Fox  |  type=Human  |  stocks=4  |  costume=3
```

## File format

`.slp` files begin with a UBJSON wrapper containing a `raw` byte array of game events followed by a `metadata` object. The raw section starts with an Event Payloads block (command `0x35`) that defines the size of each subsequent event type, followed by a Game Start event (`0x36`) containing full match metadata.

Reference: [Slippi Replay Spec](https://github.com/project-slippi/slippi-wiki/blob/master/SPEC.md)
