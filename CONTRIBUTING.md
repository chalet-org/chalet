# Contributing

Thank you for taking the time to check out Chalet! If you'd like to contribute, there's several ways to do so. In no particular order:

* If you have questions, comments, or suggestions related to the future direction of Chalet, please start with the [Discussions](https://github.com/chalet-org/chalet/discussions) board. If it makes sense for the growth project, an enhancement issue will be created.

* The biggest contribution you can make right now is testing and using hte application. If during your testing, you find a bug -- or find something that doesn't make sense enough for the discussions board, but can be classified more as a bug or a design issue -- it can be brought up as an [Issue](https://github.com/chalet-org/chalet/issues). In it, please describe the bug as much as possible, the platform it was found on, the compiler toolchains and architecture. The more specific the description, the quicker it can be identified and fixed. 

* If you feel like diving into the C++ portion of the code, find something to fix or improve, and want to open a Pull request, feel free to do so. The codebase is still hot off the presses, so there's certainly plenty of areas to improve. PRs may take some time to approve if for instance, it addresses one area of the codebase, but not others. For instance, adding a new command-line argument is not exactly trivial, because it needs the argument itself, state to read into, an option for it in settings, an entry in the settings schema, global settings (~/.chalet/config.json) handling, and the actual behavior of that setting itself. Ultimately, the PR will be reviewed based on the type of feature/bug it is, and suggestions will be provided. Changes will need careful considerations because they might affect other platforms and parts of the codebase that may not be obvious.

* Shell tab-completions are current provided for `sh`, `bash`, `zsh` and `fish`. They don't provide completion for everything yet, so they're certainly a good place to contribute. Please be mindful of the different shell types and their syntax.

* Also be aware there are [various other Chalet repositories](https://github.com/orgs/chalet-org/repositories) to contribute to -- if it doesn't make sense to contribute to the main repo, but rather one of the others, feel free!

## C++ Style Guide

***Work in-progress***

It's going to be tough to explain every bit of the style choices in this guide, but the biggest point to be made is observe how the code is structured, Google anything that is unfamiliar. This will probably end up more as an FAQ than a full-blown style guide. Here are some of the basics:

* All code uses the `chalet` namespace

* TitleCase for enumerations, class & struct names, and one per file for the most part. File names must match.

* Compilation units must have the *.cpp file extensions

* Header files must use *.hpp with inline and constexpr code split between *.hpp and *.inl files. Yes, this is more verbose, but it's done to keep the codebase structured and organized. If *.inl files are new to you, review how the existing ones are included at the bottom of their respective header files. Header files also use `#ifdef` blocks as opposed to `#pramga once`. this is for maximum compiler compatibility, but probably isn't necessary...

* `#include` declarations are split between headers and compilation units where needed. Forward declarations are used in headers for pointers and references. This is done to prevent files unnecessarily compiling due to header changes.

* `State` source files (and others) use getters/setters. This may be exhaustive, but it simply done to keep responsibilty in the class/struct's compilation unit. Occasionally, plain structs will be used - mostly for intermediate data types, created to pass data from one place to another.

* A lot of the codebase has various kinds of string mainpulation. Plain `std::string` was chosen instead of a `String` class mainly to keep the data lean. Static/plain functions (like in `String.hpp` and `List.hpp`) are used for functionality not otherwise included in `std::string`

* The codebase is currently C++17 since it's supported on all the major compilers. This will be bumped up to C++20 eventually when it makes sense to. The biggest reason to now would be `fmt`'s contexpr functionality, but there are certainly parts of the codebase that could be done at compile-time.

## Code of Conduct

This project abides by the [Contributor Covenant Code of Conduct (2.1)](https://www.contributor-covenant.org/version/2/1/code_of_conduct.html). Please review the accompanying [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) file.

## License

Chalet is released under the [BSD 3-Clause](https://opensource.org/licenses/BSD-3-Clause) license. See [LICENSE.txt](LICENSE.txt). Contributers will be added to this file unless otherwise requested.
