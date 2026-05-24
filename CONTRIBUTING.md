# Contributing to Wallpapi

Thank you for your interest in improving Wallpapi.

## Development setup

1. **Requirements:** Windows 10/11, Visual Studio 2022 (Desktop development with C++), CMake 3.20+, Git.
2. Clone the repository and open PowerShell in the project root.
3. Build: `.\build.ps1`
4. Run the engine: `.\run-engine.ps1`
5. In another terminal: `.\build\Release\wp-cli.exe status`

## Pull requests

1. Fork the repository and create a feature branch.
2. Keep changes focused; match existing C++ and PowerShell style.
3. Run `.\test-install.ps1` on Windows before submitting.
4. Update `CHANGELOG.md` under **Unreleased** for user-visible changes.
5. Open a pull request with a clear description and test notes.

## Reporting issues

Include:

- Windows version
- Steps to reproduce
- Output of `wp-cli status` (if the engine runs)
- Relevant lines from logs if applicable

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
