# libhal-expander

[![✅ Checks](https://github.com/libhal/libhal-expander/actions/workflows/ci.yml/badge.svg)](https://github.com/libhal/libhal-expander/actions/workflows/ci.yml)
[![GitHub stars](https://img.shields.io/github/stars/libhal/libhal-expander.svg)](https://github.com/libhal/libhal-expander/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/libhal/libhal-expander.svg)](https://github.com/libhal/libhal-expander/network)
[![GitHub issues](https://img.shields.io/github/issues/libhal/libhal-expander.svg)](https://github.com/libhal/libhal-expander/issues)

libhal compatible device drivers IO expander type devices.

## 📚 Software APIs & Usage

To learn about the available drivers and APIs see the
[API Reference](https://libhal.github.io/latest/api/)
documentation page or look at the headers in the
[`include/libhal-expander`](https://github.com/libhal/libhal-expander/tree/main/include/libhal-expander)
directory.

## 🧰 Getting Started with libhal

Checkout the
[🚀 Getting Started](https://libhal.github.io/getting_started/)
instructions.

## 🏗️ Building Demos

To build demos, start at the root of the repo and execute the following command:

```bash
conan build demos -pr lpc4078 -pr arm-gcc-12.3
```

or for the `lpc4074`

```bash
conan build demos -pr lpc4074 -pr arm-gcc-12.3
```

or for the `stm32f103c8`

```bash
conan build demos -pr stm32f103c8 -pr arm-gcc-12.3
```

## 📦 Building & installing the Library Package

To build and install the package, start at the root of the repo and execute the 
following command:

```bash
conan create . -pr lpc4078 -pr arm-gcc-12.3 --version=latest
```

To compile the package for the `stm32f103c8` or `lpc4074`, simply replace the 
`lpc4078` profile with the appropriate profile name. For example:

```bash
conan create . -pr stm32f103c8 -pr arm-gcc-12.3 --version=latest
```

> [!NOTE]
> If you are developing the code, and simply need to test that the package builds
> and that tests pass, use `conan build .` vs `conan create .`. This will build the
> package locally in the current directory. You'll find the contents in the
> `build/` directory at the root of the repo. Now links will point to the code
> in the repo and NOT the conan package directory.

## 📋 Adding `libhal-expander` to your project

Add the following to your `requirements()` method within your application or
library's `conanfile.py`:

```python
    def requirements(self):
        self.requires("libhal-expander/[^1.0.0]")
```

Replace version `1.0.0` with the desired version of the library.

Assuming you are using CMake, you'll need to find and link the package to your
executable:

```cmake
find_package(libhal-expander REQUIRED CONFIG)
target_link_libraries(app.elf PRIVATE libhal::expander)
```

Replace `app.elf` with the name of your executable.

The available headers for your app or library will exist in the
[`include/libhal-expander/`](./include/libhal-expander) directory.


## 🌟 Package Semantic Versioning Explained

In libhal, different libraries have different requirements and expectations for
how their libraries will be used and how to interpret changes in the semantic
version of a library.

If you are not familiar with [SEMVER](https://semver.org/) you can click the
link to learn more.

### 💥 Major changes

The major number will increment in the event of:

1. An API break
2. A behavior change

We define an API break as an intentional change to the public interface, found
within the `include/` directory, that removes or changes an API in such a way
that code that previously built would no longer be capable of building.

We define a "behavior change" as an intentional change to the documentation of
a public API that would change the API's behavior such that previous and later
versions of the same API would do observably different things.

The usage of the term "intentional" means that the break or behavior change was
expected and accepted for a release. If an API break occurs on accident when it
wasn't previously desired, then such a change should be rolled back and an
alternative non-API breaking solution should be found.

You can depend on the major number to provide API and behavioral
stability for your application. If you upgrade to a new major numbered version
of libhal, your code and applications may or may not continue to work as
expected or compile. Because of this, we try our best to not update the
major number.

### 🚀 Minor changes

The minor number will increment if a new interface, API, or type is introduced
into the public interface.

### 🐞 Patch Changes

The patch number will increment if:

1. Bug fixes that align code to the behavior of an API, improves performance
   or improves code size efficiency.
2. Any changes occur within the `/include/libhal-expander/experimental`
   directory.
3. An ABI break

For now, you cannot expect ABI or API stability with anything in the
`/include/libhal-expander/experimental` directory.

ABI breaks with board libraries used directly in applications do cause no issue
and thus are allowed to be patch changes.

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details.

## License

Apache 2.0; see [`LICENSE`](LICENSE) for details.
