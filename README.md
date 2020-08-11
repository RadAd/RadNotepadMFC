<!-- ![Icon](res\RadNotepad.ico) RadNotepad MFC -->
<img src="res\RadNotepad.ico" width=32/> RadNotepad MFC
==========

Text Editor for Source Code

![Windows](https://img.shields.io/badge/platform-Windows-blue.svg)
[![Releases](https://img.shields.io/github/release/RadAd/RadNotepadMFC.svg)](https://github.com/RadAd/RadNotepadMFC/releases/latest)
[![Build](https://img.shields.io/appveyor/ci/RadAd/RadNotepadMFC.svg)](https://ci.appveyor.com/project/RadAd/RadNotepadMFC)

[Scintilla](https://www.scintilla.org/)
=======
Evrything is included in order to build RadNotepadMFC.
If you want to download the scintilla source:
```
msbuild ScintillaPre.vcxproj /t:Update
```

Build
=======
```
MSBuild.bat RadNotepad.vcxproj -p:Configuration=Release -p:Platform=x64
```
