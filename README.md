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
![Scintilla](https://img.shields.io/badge/dynamic/regex?url=https%3A%2F%2Fraw.githubusercontent.com%2FRadAd%2FRadNotepadMFC%2Frefs%2Fheads%2Fmaster%2FScintillaPre.vcxproj&search=%3CVersion%3E(.*)%3C%2FVersion%3E&replace=%241&label=Scintilla)
[![Scintilla latest](https://img.shields.io/badge/dynamic/regex?url=https%3A%2F%2Fwww.scintilla.org%2FScintillaDownload.html&search=Release%20(%5B1-9%5C.%5D%2B)&replace=%241&label=Latest)](https://www.scintilla.org/ScintillaDownload.html)

![Lexilla](https://img.shields.io/badge/dynamic/regex?url=https%3A%2F%2Fraw.githubusercontent.com%2FRadAd%2FRadNotepadMFC%2Frefs%2Fheads%2Fmaster%2FLexillaPre.vcxproj&search=%3CVersion%3E(.*)%3C%2FVersion%3E&replace=%241&label=Lexilla)
[![Lexilla latest](https://img.shields.io/badge/dynamic/regex?url=https%3A%2F%2Fwww.scintilla.org%2FLexillaDownload.html&search=Release%20(%5B1-9%5C.%5D%2B)&replace=%241&label=Latest)](https://www.scintilla.org/LexillaDownload.html)

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
