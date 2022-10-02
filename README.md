
[![Chalet logo](chalet-banner.jpg)](https://www.chalet-work.space)


<div align="center">
<p>A cross-platform project format & build tool for C/C++ focused on usability and interoperability.</p>

<hr />

[![Github Releases](https://img.shields.io/github/v/release/chalet-org/chalet?style=for-the-badge&color=orange&labelColor=323232)](https://github.com/chalet-org/chalet/releases)
[![License](https://img.shields.io/badge/license-bsd3clause-orange.svg?style=for-the-badge&labelColor=323232)](https://github.com/chalet-org/chalet/blob/main/LICENSE.txt)
[![Discussions](https://img.shields.io/badge/-discussion-black?style=for-the-badge&color=323232)](https://github.com/chalet-org/chalet/discussions)
[![Examples](https://img.shields.io/badge/-examples-black?style=for-the-badge&color=323232)](https://github.com/orgs/chalet-org/repositories?q=chalet-example)

</div>

<br />

## Introduction

Chalet is a command-line application that reads from a JSON project file (chalet.json), and compiles your C/C++ projects. Behind the scenes, Chalet generates either a [Ninja](https://ninja-build.org/) or [Make](https://en.wikipedia.org/wiki/Make_(software)) build file and performs some caching. Chalet project files focus on intuitive language designed to be easy to understand, while at the same time providing lots of control. Because the projects are JSON, they can be easily integrated with IDEs.

Visit [https://www.chalet-work.space](https://www.chalet-work.space) to learn more about Chalet.

Visit [https://www.chalet-work.space/docs](https://www.chalet-work.space/docs) for the full documentation.

Explore some of the example repositories at: [https://github.com/orgs/chalet-org/repositories?q=chalet-example](https://github.com/orgs/chalet-org/repositories?q=chalet-example)

## Download

Grab the latest version from [https://www.chalet-work.space/download](https://www.chalet-work.space/download) or the [Releases](https://github.com/chalet-org/chalet/releases) section of this repository.

The actual release builds are still being fine-tuned, especially on Linux, so there may still be issues on various Linux distros.

## License

Chalet is released under the [BSD 3-Clause](https://opensource.org/licenses/BSD-3-Clause) license. See [LICENSE.txt](LICENSE.txt). 

## External Libraries that make Chalet possible

-   [JSON for Modern C++](https://github.com/nlohmann/json) - [MIT license](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)
-   [JSON schema validator for JSON for Modern C++](https://github.com/pboettch/json-schema-validator) - [MIT license](https://github.com/pboettch/json-schema-validator/blob/main/LICENSE)
-   [fmt](https://github.com/fmtlib/fmt) - [MIT license](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)
-   [stduuid](https://github.com/mariusbancila/stduuid) - [MIT license](https://github.com/mariusbancila/stduuid/blob/master/LICENSE)
-   [Catch2](https://github.com/catchorg/Catch2) - [BSL-1.0 license](https://github.com/catchorg/Catch2/blob/devel/LICENSE.txt)

## Build from Source

If you would like to build from source, and track the latest changes as they come out, please review the [COMPILE.md](COMPILE.md) document.

## Contributing

If you would like to contribute, please review the [CONTRIBUTING.md](CONTRIBUTING.md) document.

