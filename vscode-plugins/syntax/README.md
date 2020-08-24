# pirate-syntax README

This extension deals with theming and syntax highlighting for Pirate-annotated
code.

In order to not have to copy/re-implement existing C/C++ syntax highlighting
definitions, we make use of injection grammars to add additional highlighting
rules for Pirate annotations.

In order to support current VS Code themes without modification, we can't use
entirely new syntax scope names, and so the scopes used here are taken somewhat
loosely from the set of known C/C++ ones.