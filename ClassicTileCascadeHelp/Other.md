## Getting Help
If you have questions, concerns, bug reports, etc, please file an issue in [this repository](https://github.com/thf06037/ClassicTileCascade)'s Issue Tracker.

## Contributing
I'm currently mainly looking for people to test the application on a variety of PCs and let me know (via the Issue Tracker) whether 
they encounter any bugs or issues with the app.

I am open to pull requests for adding new features, etc. Please note that I may not be able to review these quickly.

## About The Code
Classic Tile & Cascade's source code is available in [this GitHub repository](https://github.com/thf06037/ClassicTileCascade).

Classic Tile & Cascade is written in C++ using native Windows libraries. 
I have not used any UI frameworks such as MFC, WTL, or WinRT.

The main directories are:
- ClassicTileCascade: Top level directory with solution (.SLN) file, License and Readme files
- ClassicTileCascade\ClassicTileCascade: The C++ source code for the ClassicTileCascade.exe that implements the
notification icon
- ClassicTileCascade\ClassicTileCascadeHelp: The source files and compiled help (CHM) file
for the project's HTML Help 
- ClassicTileCascade\ClassicTileCascadeSetup: The project file (*.vdproj) for the Visual Studio install project
that creates the MSI and Setup.exe file for the install 


It was written and compiled/linked using Microsoft Visual Studio Community 2022 (64-bit) - Current
Version 17.6.3.

To rebuild the project fully from the command line (including the main C++ executable project, installation project, 
and the HTML Help CHM file - requires Visual Studio), 
CD to the top level code directory and run the following:

`devenv ClassicTileCascade.sln /rebuild "Release|x86" /project "ClassicTileCascadeSetup\ClassicTileCascadeSetup.vdproj"`


The HTML Help for the project was developed and built (CHM file) 
using HTML Help Workshop. 
You can download HTML Help Workshop for free [here](https://web.archive.org/web/20210126113408/https://www.microsoft.com/en-us/download/details.aspx?id=21138).
You only need to download & install htmlhelp.exe from that page.

The HTML Help CHM will be built automatically by a regular build/rebuild of the solution. 
Should you wish to compile the CHM seperately from the project via the command line, 
CD to the ClassicTileCascadeHelp directory and run the following:

`hhc ClassicTileCascadeHelp.hhp`

A post build step for the project requires pandoc. You can download pandoc [here](https://pandoc.org/installing.html).

## About Me And Why I Created This Project
**About me**

I created all of the code and help, etc. for this project as an "individual coder."

I have been in software development my entire career and played roles as a software developer, 
business analyst, IT project manager, and IT middle and executive manager. 

**Why I created this project**

A few main reasons:

1. I recently moved my main home PC to Windows 11 and wanted the classic tile and cascade functionality from 
Windows 10 back. The "Aero Snap" approach (which is the only windows arrangement approach on Windows 11
out of the box) has several shortcomings. 

2. I also wanted to fix the tile and cascade functionality that worked
fine in Windows 7, but was broken by Windows 10 (see above).

3. I have little or no time in my current IT role for hands on coding. But I miss coding so badly! In my 
coding heyday (ca. 1996-2006), I developed mostly in Win32 C++ (MFC, COM/DCOM, ATL), Visual Basic 6, and 
Java. In this project I wanted to:

    i. Learn more about how C++ has developed and change in the last 15 years. Particularly wanted to 
get exposed to the language changes that C++ 11 brought about. **My observation**: I was really blown away
by the scope of change to C++ in the last 10+ years!! So much so that I found it really hard to 
leverage all of the new functionality in a "hand's on" project. This project inspired me
to order Scott Meyer's "Effective Modern C++" book to get a deeper perspective.

    ii. See whether Win32 programming in C/C++ had changed much in the last 15 years. **My observation**: Very 
little change. I was able to pick things up basically where I left them those many years ago. Based on what is 
available online, most developers have understandably moved to C#/WinForms/WPF for this type of project.

    iii. Learn about modern source code control using Git and Github. **My observation**: Git is easy to get 
started with (especially with Visual Studio integration), but an absolute beast to master. I probably spent
more time figuring out how to get certain things done in Git 
than anything else on the project. I have barely scratched the surface.

    iv. Learn about the non-technical aspects of Open Source projects, including licensing, collaboration,
code signing, etc. **My observation**: There are a tremendous amount of good resources online for this, but many of them
are geared towards organizations rather than indivdual coders. 
