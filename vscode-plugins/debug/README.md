# Pirate Debug Configuration Provider

This extension attempts to provide debug configurations for Pirate applications based on `pdl.yaml` configuration files.  It is currently relatively limited and works only when no `launch.json` already exists.  To use it:

1. Select the 'Run' in the Activity Bar (the 'play' or triangle icon all the way on the left)
2. Click the link reading `create a launch.json file`
3. Select the `Pirate` configuration

Note that this extension requires the Pirate Deployment tool (PDL) to exist on the PATH to function correctly.  If you need to set an explicit path to it, you can do so in the extension settings.

Note also that due to a limitation in VSCode's extension API, it is currently not possible to generate a 'compound' debug target.  So if you wish to debug an entire multi-enclave Pirate application at once, it is useful to manually add a compound target that launches all of the generated ones.  See https://code.visualstudio.com/docs/editor/debugging#_compound-launch-configurations

## Coming soon

More ways of generating and updating existing configurations.