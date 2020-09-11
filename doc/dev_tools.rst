Pirate Developer Tools
^^^

Overview
===

The pirate developer tools are built on top of `Visual Studio Code
<https://code.visualstudio.com>`_ and make use of a modified version of `clangd
<https://clangd.llvm.org/>`_.

Installation
===

You can install VS Code in any way you choose—directly from the website, through
your preferred package manager, etc.  All of the Pirate functionality is
implemented as extensions which, in principle, should be compatible with any
recent version of the editor including open source versions such as `VSCodium
<https://vscodium.com>`_.

**TODO**: Determine the best way to install all the extensions that we rely on
that aren't our own.  Possibly a meta-extension that just depends on them.  Then
document that here before merging docs to master.

Extension Overview
===

This documentation will provide an overview of each of the extensions intended
to be used when developing applications with the Pirate tooling.  However, some
of these extensions (such as the one for interacting with debuggers) are large
and complex.  Each description will contain a link to where you can find
additional information.

The current set of extensions is as follows:
* `Pirate Syntax Highlighting`_
* `VSCode-clangd`_
* `Architecture Visualization`_
* `Native Debug`_
* `*Optional:* CMake`_
* `*Optional:* CMake Tools`_
* `*Optional:* Remote/Docker Development`_


Pirate Syntax Highlighting
---

You can find additional information and the source code in the main Pirate
repository:
`<https://github.com/GaloisInc/pirate/tree/master/vscode-plugins/syntax>`_

This plugin defines several grammar injections that provide additional syntax
highlighting for Pirate pragmas and attributes.  Eventually, it may also
contains additional aesthetic customizations in order to highlight code
belonging to specific enclaves, and so on.

VSCode-clangd
---

This extension is responsible for establishing the communication
between VSCode and the clangd language server.  While Pirate
officially recommends using CMake to build your projects, this
extension should work regardless of your build system so long as you
can generate an LLVM compilation database, i.e., a
``compile_commands.json`` file.  This extension expects to find such a
file in the root of your project, so you may want to copy or link it
there from wherever you generate it (and put it in ``.gitignore``
since it will only apply to the paths in your environment.)

Configuration
~~~

Depending on your development environment, you may need to configure
this extension to correctly locate the ``clangd`` executable.  The
clangd implementation is tied very closely to LLVM and clang, and so
you must use the version that is shipped with the Pirate LLVM fork.

If you use the official Docker image, the correct executable should already be
on your path.  If you are building Pirate LLVM yourself, you'll want to do one
of the following:
* Link or copy the ``clangd`` executable to some place on your ``$PATH``
* Configure this extension to point to wherever you built ``clangd``
* Install the Pirate LLVM toolchain to the system

Note also that if you have C/C++ plugins besides those that are installed as
part of Pirate, it's possible that conflicts could arise in terms of what
extensions handle your code.  Microsoft offers several plugins, including one
installed by default, that may offer to "configure intellisense" for you, or
other things.  You should always refuse, or else this extension may not function
correctly.

Language Server Protocol
~~~

The Language Server Protocol is the protocol used for communication between
editors (such as VS Code) and language servers (such as clangd).  The idea is
that it allows a single language server implementation to work for any editor.

The clangd implementation includes several extensions to the official protocol,
such as for switching between source and header files (part of the official
clangd release) and for observing particular kinds of changes for the sake of
performing additional analysis (Pirate-specific extension).

In principle you could use any of a number of C/C++ language server
implementations, but doing so will likely cause a number of other Pirate
extensions not to function correctly.

You can find the upstream clangd documentation, which is almost entirely still
applicable, here: `https://clangd.llvm.org/`._

Architecture Visualization
---

**TODO**

Native Debug
---

This plugin serves as the interface between VS Code and a debugger.
At the moment, GDB is preferred to LLDB for stability concerns but in
principle both should work.  Changes to the plugin have, so far, been
upstreamed and so documentation for the plugin is extremely useful:
`https://github.com/WebFreak001/code-debug/`._

Configuration
~~~~

It's difficult to summarize the configuration of the debugger plugin,
since it will likely be unique for each project depending on it's
structure, where you want to execute it, etc.

Every project will need a ``launch.json`` file.  This file configures
the actions that any VS Code debugger can execute.  In general, you
will have one configuration for each executable you want to run and
then one more that serves as a compound launcher for all the others.
That means, if you have 2 enclaves in your project that compile to 2
executables, you will likely want one launch action for each of the 2
enclaves, and a third that triggers them both at once.

If you don't already have a ``launch.json`` file, you can navigate to
the "Run" pane in VS Code by clicking the icon on the left, and then
click the link for "create a launch.json file".  When you do this, you
will be asked what type of debugging environment you want to set up.
This is a bit subtle, because Microsoft ships a simplified debugger
for C/C++ code that you probably have installed.  You **DO NOT** want
to select ``C++ (GDB/LLDB)``.  Instead, you want to select the entry
labeled simply ``GDB``.  If you don't see this entry, make sure you've
installed the plugin and restart your editor.  This will automatically
fill in some of the JSON boilerplate for you.

Now you will need to configure the details for yourself.  As
mentioned, you will want one configuration per enclave/executable.
The configuration may look something like this::

  {
    "version": "0.2.0",
    "configurations": [
      {
        "name": "Green",
        "type": "gdb",
        "request": "launch",
        "target": "./path/to/executable",
        "arguments": "--gps-to-target ...",
        "cwd": "${workspaceRoot}",
        "valuesFormatting": "parseText",
      },
      {
        "name": "Orange",
        ...
      }
    ],
    "compounds": [
      {
        "name": "Orange and Green",
        "configurations": ["Green", "Orange"]
      }
    ]
  }

You can see there are two configurations, and a "compound"
configuration which simply names the other two.  Now, when you debug
the compound task, it will start both executable and allow you to
switch between the two at will.

More Complex Setups
~~~

If you want to do remote debugging or use multiple machines, see the
`*Optional:* Remote Development`_ section below.

CMake
-----------------

The only supported build tool for Pirate at the moment is CMake.  A
brief intro to some Pirate-specific considerations is reproduced
below, as well as notes on the CMake extensions for VS Code.

CMake the Tool
~~~

A complete introduction to CMake itself is out of scope here, but
there are a few useful things to know that are Pirate-specific.

Most importantly, you'll need to specify some additional compilation
and linking flags.  Some of these enable features, but you'll also
need to ensure that the Pirate linker is being used.

You will want to enable the following compilation flags:

* ``-ffunction-sections``
* ``-fdata-sections``

And you will also need the following linker flags, which will likely
occur once per enclave/target.  Note that you can replace the
executable name ``lld`` in order to point to the Pirate version of
``lld``.

* ``-Wl,-enclave,enclave_name``
* ``-fuse-ld=lld``

As mentioned above, CMake is also able to generate a
``compile_commands.json`` file for you automatically, which is a
mandatory part of getting ``clangd`` working correctly.  You can use
the flag ``-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`` when configuring a
project to do this, and then link it into the root project directory.

CMake Extension
~~~

The `CMake extension <https://github.com/twxs/vs.language.cmake>`_ is
quite simple, and adds editor support for working on CMake
configuration files.


CMake Tools Extension
~~~

This is a `Microsoft extension
<https://github.com/microsoft/vscode-cmake-tools>`_ that allows you to
integrate with CMake in richer ways.  It is completely optional, and
somewhat difficult to set up.  This is because it overlaps in
functionality somewhat with the Clangd extension.  There does not
appear to be a good way to tell VS Code to prefer one extension over
another when both want to, e.g., set up autocompletion for a project.
For this reason, it may be simpler to just add a build task manually
that calls out to CMake and use only the Clangd extension.

*Optional:* Remote/Docker Development
------------------------------

VS Code has excellent support for working remotely, which includes
actual remote machines, virtual machines, Docker containers, and more.
To start, you will need to install `the plugin
<https://code.visualstudio.com/docs/remote/remote-overview>`_ which
bundles together support for all the different platforms.

You can use this plugin to develop code that exists on your local
machine inside the Pirate Docker image, which automatically puts the
most recent version of the toolchain in the environment.

To do this, install the extension, open your project in VS Code, and
add a trivial Dockerfile to the root directory that contains the
following contents::

  FROM pirateteam/ubuntu:latest

Now click on the ``><`` icon in the bottom left of the editor, and
select "Reopen in container".  If you have added the Dockerfile, you
should be able to select "From 'Dockerfile'", and VS Code will
automatically build this image, install your project code into it, and
open it transparently.

Note that your installed extensions are separate for the local project
and the "remote" project.  If you browse to your installed extension
while you have the remote project open, each will have a "Install in
Dev Container" button, which you should use.  You can also edit the
development container files to automatically install extensions
whenever the Dockerfile causes the container to be re-built.

Remote Debugging
~~~

If you want to debug remotely, for instance to test enclaves running
on separate machines, you will need to do a few things.  First, both
the compiled executable and the corresponding source code needs to
exist on the remote machine.  One good setup is to create VS Code task
that does your compilation, and then another that depends on the
compilation task that copies the files to the remote machine.  This
might look something like this (note this assumes you have key-based
authentication and SSH aliases configured already)::

  {
    "version": "2.0.0",
    "tasks": [
      {
        "label": "Copy Foo",
        "type": "shell",
        "command": "scp -r ${workspaceFolder} machine1:~/myproject"
      },
      {
        "label": "Copy Bar",
        "type": "shell",
        "command": "scp -r ${workspaceFolder} machine2:~/myproject"
      },
      {
        "label": "Copy All",
        "type":"shell",
        "dependsOn":[ "Copy Foo", "Copy Bar" ]
      }
    ]
  }

You will also need to update the ``launch.json`` file.  This will vary
based on the desired setup, of course, but here is an example::

  {
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
      {
        "name": "Green",
        "type": "gdb",
        "request": "launch",
        "target": "./path/to/executable",
        "arguments": "--args",
        "cwd": "${workspaceRoot}",
        "valuesFormatting": "parseText",
        "pathSubstitutions": {
          "/root/myproject": "/home/username/myproject"
        },
        "ssh": {
          "host": "address.of.remote.machine",
          "port": 22,
          "cwd": "/home/username/myproject",
          "user": "karl",
          "keyfile": "/home/username/.ssh/id_rsa",
          "forwardX11": false
        }
      },
  ...

There are a few things to note here.  The first is the ``ssh`` block,
which should be mostly self-explanatory.  You can also hardcode a
password if you do not have key-based authentication, but this will be
stored in plain text and is not recommended.  VS Code will
autocomplete the different settings if you need something not shown
here.

The less intuitive section is the ``pathSubstitutions`` block.  This
sets up name substitution for the debuginfo in the binary.  For
instance, if you build the executables in Docker, and then scp them to
the remote debugging machines, the debugger will try to look for
corresponding source in the directories where they were located in the
Docker container.  In many cases this won't match, such as when
compiling as the root user in Docker and having a different username
on your debugging machine.  So, if source files were located at
``/root/myproject`` in Docker but you've placed them at
``/home/username/myproject`` on your debugging machine, you can
specify that substitution as in the above example.  You can specify
more than one substitution if necessary.
