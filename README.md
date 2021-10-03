Dict2 is a dictionary viewing application for Linux. It was written
with the intention to be used with the German-English dictionary files
from [www.dict.cc](www.dict.cc).

**Note**: You must download the dictionary files directly from
[here](https://www1.dict.cc/translation_file_request.php?l=e). They
are not included in the distribution.

**Important**: You should choose the old format when downloading
  (Elcombri / old format, cp1252). Other formats are not supported.

Features
--------
* Easy-to-use graphical interface.
* Regular expression (regex) search.
* Searching words with a common stem.
* Searching inflections of a word.
* Searching alternate forms of a word.
* Tabbed browsing.
* Caching to improve performance.

Requirements
------------
* Linux
* GCC
* GTK development files (libglade-2.0, libgnomeui-2.0)
* libiconv

Installation
------------
```
./configure
make
make install
```

If you're missing the necessary libraries (`configure` gives an
error), try searching for `libgnomeui`, `libglade`, `libiconv` or
similar with your package manager.

Usage
-----

A short manual is available at:
[https://lukaszcz.github.io/dict2](https://lukaszcz.github.io/dict2).

Dictionaries that come with Dict2
---------------------------------

There are several small dictionaries from Project Gutenberg that are
distributed together with Dict2. They are installed to
`prefix/share/dict` by default, where `prefix` is the directory you
passed as the `--prefix` parameter to `configure` (typically
`/usr/local` if you didn't pass anything).

The dictionary files are copyright by Winfried Honig. View them in an
ordinary text editor to see the exact license requirements.

Getting additional dictionary files
-----------------------------------

Dict2 was written with the main purpose to be used with the
English-German/German-English dictionary files from
[www.dict.cc](https://www1.dict.cc/translation_file_request.php?l=e). These
files are free to use, but the licence requires that they be not
distributed together with any software for reading them, so you just
have to fetch them form there.

You only need to unzip the downloaded file and load the text file by
clicking the 'Load' button.

**Important**: You should choose the old format when downloading
  (Elcombri / old format, cp1252). Other formats are not supported.

Configuration
-------------

The configuration may be changed from the GUI by clicking on the
'Options' button. It is saved in the file `.dict.cfg` in your home
directory, which itself should be intuitive enough to edit.

Regular expressions
-------------------

The program has a nice feature of finding dictionary entries that
match a particular Perl regular expression (regex). For information
about the exact syntax refer to the Perl manual (`man perl`; `grep`
regexes have similar syntax, so `man grep` may also be of use).

Copyright and license
---------------------

Copyright (C) 2007-2021 by Lukasz Czajka.

Distributed under GPL 2. See [LICENSE](LICENSE).
