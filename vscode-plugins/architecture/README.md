# PIRATE architecture plugin

The Pirate architecture modeling plugin aims to provide VSCode users
with visualization and editing tools for PIRATE architecture files.
These are intended to illustrate the system architectures and provide
an alternative means for navigating the code in an application.

## Building

The first step to building this extension is to install [npm](https://www.npmjs.com/)
and VSCode.  Once npm is installed, you can install the dependencies by running:

```
npm install
```

Once the dependencies are installed, you can debug the extension with VSCode by
start debugging (F5) or build it manually by running:

```
npm run compile
```

# Debugging

Debugging webviews is not yet officially supported, but there is an
experimental workaround. You need to set the following fields in your user
configuration (they will **not** work in the workspace configuration):

```
"debug.javascript.usePreview": true,
"webview.experimental.useIframes": true,
```

This works in conjunction with the line:

```
debugWebviews: true
```

in `launch.json`.
