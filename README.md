## 0xegg

![Preview](preview.png)

## Building

Requires CMake 3.10+ and a C++17 compiler. Windows only — the tool is built on the Win32 `SendInput` API.

```sh
cmake -B build
cmake --build build --config Release
```

The executable lands in `build/Release/egg.exe`.

## License

This project is licensed under the [MIT License](LICENSE).

Big thanks to George Wong @ http://www.gnwong.com/
