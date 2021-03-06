# CMPL pre-built binaries

OverlayPal depends on CMPL for optimising background / sprite palettes. CMPL is Free Software licensed under the GPLv3.
The source code for CMPL can be found from official CMPL webpage https://projects.coin-or.org/Cmpl

To aid a quicker integration with less build effort, OverlayPal currently includes pre-built binaries and calls the executable as a sub-process, on both Windows and GNU/Linux.

The specific binaries in this directory have been downloaded from the official website:

    CMPL 1.11.0 for Windows: http://coliop.org/_download/Cmpl-1-11-0-win.zip

    CMPL 1.12.0 for GNU/Linux: http://www.coliop.org/_download/Cmpl-1-12-0-Linux64.tar.gz

The inconsistency between versions used for Windows and GNU/Linux is a result of two particular quirks:

1. The official 1.12.0 binary for Windows was causing yet-to-be-investigated crashes for non-trivial optimisation problems.
2. An official download link for the outdated 1.11.0 distribution for GNU/Linux has not been found.

If you wish to use any other build of CMPL with OverlayPal, just make sure that the "bin" directory of CMPL / CBC executable is available as a subfolder "Cmpl/bin" below OverlayPal's application directory.

A direct integration of the CMPL codebase into OverlayPal is planned to happen at some point.
