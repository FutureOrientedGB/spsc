@echo off

@REM -G <generator-name>          = Specify a build system generator.
@REM -S <path-to-source>          = Explicitly specify a source directory.
@REM -C <initial-cache>           = Pre-load a script to populate the cache.
@REM -B <path-to-build>           = Explicitly specify a build directory.
cmake                                                                                                        ^
    -S .                                                                                                   ^
    -B build/x64-windows-static                                                                              ^
    -A x64                                                                                                                                                                                                ^
