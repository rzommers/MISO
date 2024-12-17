# Editor's Guide

This page is for editors of this guide, and serves as a reference of information for adding 

Immediate things that need to be done:
1. OpenCascade - Currently have a folder to connect with GitHub, would be nice to find version on their website that is compatible.
2. MotorModel Tests - Section needs to be updated when problems are resolved

The Readthedocs portion of this repository includes the following files/folders:
1. .readthedocs.yaml - Main file; same basic formula, doesn't need to be touched beyond what's already been done
2. rtd (folder)
     1. Makefile
     2. make.bat
     3. requirements.txt - installed packages, like markdown; extensions can be added here if desired
     4. conf.py - Configuration file; used to set project title, author(s), copyright, extensions, and themes; can be modified as project changes
     5. index.md - Markdown file, main content page
     6. editors.md - This file
  
To make a new page, simply make a new GitHub file with the ending ".md". The readthedocs page should update, but this is not confirmed.

Editing in GitHub will automatically reflect changes in Readthedocs. There may be a time delay, but no further work needs to be done. Changes to the conf.py file will take longer to reflect over.

### Markdown Basics

- To make titles/subtitles, use the # key before the title name. Each additional # in a row diminishes the size of the title.
- To make a code block, use ``` before and after the lines of code, each on their own line. Separate code blocks from other text by at least one empty line.
- Inspect this file's markdown code to see how bulleted and numbered lists are made.
- For additional information, check out the [Full Markdown Guide](https://www.markdownguide.org/extended-syntax/)
