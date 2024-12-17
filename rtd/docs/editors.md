# Editor's Guide

This page is for editors of this guide, and serves as a reference of information for adding new content/pages, as well as keeping track of improvements that need to be made.

Remaining issues that need to be resolved:

1. OpenCascade - Current solution is to download a file linked online. Better solution would be to download and install directly from the [OpenCascade Website](https://dev.opencascade.org/release).
2. MotorModel Tests - Section needs to be updated when problems are resolved
3. ESP - It is currently built in such a way that PUMI is not properly visualized, and fails the MFEM tests. A correction needs to be made and subsequent steps must be tested to confirm that the process still works properly.

The Readthedocs portion of this repository includes the following files/folders:

1. .readthedocs.yaml - Main file; points to configuration files, selects versions of Ubuntu, Python; don't change
- rtd (folder)
  2. mkdocs.yml - Markdown configuration file; used to change project title, theme
  3. requirements.txt - Generated requirements file for RTD; don't change
  4. requirements.in - Used for individual builds; don't change
  5. index.md - Main content page; uses Markdown
  6. editors.md - This page
  
To make a new page, simply make a new GitHub file with the ending ".md" within.

Every GitHub commit automatically makes Readthedocs build a new document. There may be a time delay, but no further work needs to be done.

The other website to maintain is [MotorModel Installation](https://motormodel.readthedocs.io/en/latest/), which does not have an Editor's Guide page.

### Markdown Basics

- To make titles/subtitles, use the # key before the title name. Each subsequent # in a row diminishes the size of the title.
- To make a code block, use ``` before and after the lines of code, each on their own line. Separate code blocks from other text by at least one empty line.
- To insert a link, write the display text in brackets, and immediately follow it (without spaces) with the embedded link in parentheses.
- Inspect this file's markdown code to see how bulleted lists are made.
- For additional information, check out the [Full Markdown Guide](https://www.markdownguide.org/extended-syntax/)
