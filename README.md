## 0xegg

## Building

Requires CMake 3.10+ and a C++17 compiler. Windows only the tool is built on the Win32 `SendInput` API.

```sh
cmake -B build
cmake --build build --config Release
```

The executable lands in `build/Release/egg.exe`.

## Macros

A macro is a JSON file describing a sequence of actions, loadable from the "Load macro" menu option. See [examples/hello.json](examples/hello.json):

```json
{
  "name": "hello example",
  "repeat": 1,
  "humanization": "human",
  "maxRuntimeMs": 60000,
  "actions": [
    { "type": "wait", "ms": 500 },
    { "type": "move", "x": 800, "y": 500 },
    { "type": "click", "button": "left" },
    { "type": "type", "text": "hello", "msPerChar": 40, "pressEnter": true }
  ]
}
```

Top-level fields (all except `actions` are optional):

| Field | Default | Meaning |
| --- | --- | --- |
| `name` | `""` | Display name only |
| `repeat` | `1` | Times to run the whole action list |
| `humanization` | `"off"` | `"off"` \| `"subtle"` \| `"human"` — overrides the session-wide preset for this macro only |
| `maxRuntimeMs` | 5 minutes | Hard stop for total execution time, capped at a 30-minute engine ceiling regardless of what's requested |

Action types:

| `type` | Fields | Effect |
| --- | --- | --- |
| `move` | `x`, `y` | Move the cursor |
| `click` | `button` (`"left"` \| `"right"`, default `"left"`) | Click |
| `wait` | `ms` | Pause |
| `type` | `text`, `msPerChar` (default 30), `pressEnter` (default false) | Type text, optionally followed by Enter |

Every delay is floored at 5ms regardless of what a macro requests, so a macro can't flood input with near-zero waits. Every executed action is appended to `egg.log` in the working directory, timestamped.

## License

This project is licensed under the [MIT License](LICENSE).

Big thanks to George Wong @ http://www.gnwong.com/
