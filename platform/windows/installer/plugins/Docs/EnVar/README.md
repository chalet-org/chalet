# EnVar Plugin for NSIS

![Build NSIS Plugin Dll's and Artifacts](https://github.com/GsNSIS/EnVar/workflows/Build%20NSIS%20Plugin%20Dll's%20and%20Artifacts/badge.svg)

Originally coded by Jason Ross aka JasonFriday13 on the forums.

* Copyright (C) 2014-2016, 2018  MouseHelmet Software
* Copyright (C) 2019-2020        Gilbertsoft LLC

The original code was downloaded from the [NSIS Wiki](https://nsis.sourceforge.io/EnVar_plug-in).

## Introduction

This plugin manages environment variables. It allows checking for environment
variables, checking for paths in those variables, adding paths to variables,
removing paths from variables, removing variables from the environment, and
updating the installer enviroment from the registry. This plugin is provided
with 32bit ansi and unicode versions, as well as a 64bit unicode version.

## Functions

There are eight functions in this plugin. Returns an error code on the
stack (unless noted otherwise), see below for the codes.

### Error Codes

| Constant       | Code | Description
| -------------- | ----:| -----------------------------------------------------
| ERR_SUCCESS    |    0 | Function completed successfully.
| ERR_NOREAD     |    1 | Function couldn't read from the environment.
| ERR_NOVARIABLE |    2 | Variable doesn't exist in the current environment.
| ERR_NOVALUE    |    3 | Value doesn't exist in the Variable.
| ERR_NOWRITE    |    4 | Function can't write to the current environment.

### EnVar::SetHKCU / EnVar::SetHKLM

* SetHKCU sets the environment access to the current user. This is the default.
* SetHKLM sets the environment access to the local machine.

These functions do not return a value on the stack.

### EnVar::Check [VariableName] [Path]

Checks for a Path in the specified VariableName. Passing "null" as a Path makes
it check for the existance of VariableName. Passing "null" for both makes it
check for write access to the current environment.

### EnVar::AddValue [VariableName] [Path] / EnVar::AddValueEx [VariableName] [Path]

Adds a Path to the specified VariableName. Does does not modify existing
variables if they are not the right type. AddValueEx is for expandable paths,
ie %tempdir%. Both functions respect expandable variables, so if the variable
already exists, they try to leave it intact. AddValueEx converts the variable
to its expandable version. If the path already exists, it returns a success
error code.

### EnVar::DeleteValue [VariableName] [Path]

Deletes a path from a variable if it's the right type. The delete is also
recursive, so if it finds multiple paths, all of them are deleted as well.

### EnVar::Delete [VariableName]

Deletes a variable from the environment. Note that "path" cannot be deleted.
Also, variables that aren't the right type are not deleted.

### EnVar::Update [RegRoot] [VariableName]

Updates the installer environment by reading VariableName from the registry
and can use RegRoot to specify from which root it reads from: HKCU for the
current user, and HKLM for the local machine. Anything else (including an
empty string) for RegRoot means it reads from both roots, appends the paths
together, and updates the installer environment. This function is not affected
by EnVar::SetHKCU and EnVar::SetHKLM, and does not write to the registry.

## Examples

For a example NSIS script visit [Docs/example.nsi](https://github.com/GsNSIS/EnVar/blob/master/Docs/example.nsi).

## Development

### System Requirements

* Visual Studio 2015 or higher
* PowerShell 3.0 or higher (to run the build script)
* PowerShell Module [Invoke-MsBuild](https://github.com/deadlydog/Invoke-MsBuild#readme)
  (to run the build script)

#### Install Module Invoke-MsBuild

Run the following command in your PowerShell:

```pwsh
Install-Module -Name Invoke-MsBuild -Force
```

### Build and deploy

All build and deploy actions are implemented in a PowerShell
[build script](https://github.com/GsNSIS/EnVar/tree/master/Bin/Build.ps1).

Run `help Bin\Build` to get a detailed description of the available steps.

For a normal build and deploy run simply call the script without any parameters.

## License

This project is released under the terms of the [zlib License](LICENSE)
