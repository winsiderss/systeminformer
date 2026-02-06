# Contribution Guidelines

Thank you for your interest in contributing to System Informer!

## Getting Started

1. Fork and clone the repository.
2. Run `build_init.cmd` in the `build` directory to initialize dependencies.
3. Build with `build_release.cmd` or open `SystemInformer.sln` in Visual Studio 2022+.

See the [build readme](./build/README.md) for detailed build instructions.

## Reporting Issues

Use the [GitHub issue tracker](https://github.com/winsiderss/systeminformer/issues)
for reporting bugs or suggesting new features. Please search for existing issues
before creating a new one.

## Code Style

System Informer follows strict coding conventions documented in [HACKING.md](./HACKING.md).
Key points:

- **Naming**: `CamelCase` for functions/parameters, `lowerCamelCase` for locals,
  `CAPS_WITH_UNDERSCORES` for types/structs.
- **Prefixes**: Use `Ph`/`PH_` for public names, `Php`/`PHP_` for private names.
- **Types**: Use NT types (`BOOLEAN`, `ULONG`, `PWSTR`, etc.) instead of standard C types.
- **Annotations**: All functions must use SAL annotations (`_In_`, `_Out_`, etc.).
- **Error handling**: Return `NTSTATUS`, `BOOLEAN`, or `HRESULT` to indicate success/failure.

## Submitting Changes

1. Create a branch for your changes.
2. Ensure your code follows the project conventions in [HACKING.md](./HACKING.md).
3. Test your changes on supported platforms (Windows 10+, x64/ARM64).
4. Submit a pull request with a clear description of the changes.

## Kernel Driver Changes

Changes to `KSystemInformer` require special attention due to security implications.
Please refer to the [kernel driver readme](./KSystemInformer/README.md) for security
policies around the driver. Built plugins with unsigned code will have restricted
driver access.

## License

By contributing to System Informer, you agree that your contributions will be
licensed under the [MIT License](./LICENSE.txt). Please review the
[Contributor License Agreement](./CLA.md) before submitting.
