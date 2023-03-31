<!-- ![Icon](res/RadNotepad.ico) RadNotepad MFC -->
# <img src="res/RadNotepad.ico" width=32/> RadNotepad MFC

Text Editor for Source Code. Supports many languages and customizable themes.

![Windows](https://img.shields.io/badge/platform-Windows-blue.svg)
[![Downloads](https://img.shields.io/github/downloads/RadAd/RadNotepadMFC/total.svg)](https://github.com/RadAd/RadNotepadMFC/releases/latest)
[![Releases](https://img.shields.io/github/release/RadAd/RadNotepadMFC.svg)](https://github.com/RadAd/RadNotepadMFC/releases/latest)
[![commits-since](https://img.shields.io/github/commits-since/RadAd/RadNotepadMFC/latest.svg)](https://github.com/RadAd/RadNotepadMFC/commits/master)
[![Build](https://img.shields.io/appveyor/ci/RadAd/RadNotepadMFC.svg)](https://ci.appveyor.com/project/RadAd/RadNotepadMFC)

![Screenshot](docs/RadNotepad.png)

[Scintilla](https://www.scintilla.org/)
-----------
The main text control is a scintilla control.
Everything is included in order to build RadNotepadMFC.
If you want to download the scintilla source:
```bat
msbuild ScintillaPre.vcxproj /t:Update
msbuild LexillaPre.vcxproj /t:Update
```

Build
-----
```bat
msbuild RadNotepad.vcxproj -p:Configuration=Release -p:Platform=x64
```
