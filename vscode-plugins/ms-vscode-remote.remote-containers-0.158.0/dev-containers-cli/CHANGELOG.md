# Change Log

Notable changes.

## January 2021

### [0.0.6]
- Bugfixes.

### [0.0.5]
- Support GitHub PR URLs with `--git-clone-url`.
- Added `--forward-server-port` and `--install-web-ui` for local testing.

### [0.0.4]
- Shallow clone first, then unshallow.
  - Allow unshallow to finish in a background process. Foreground process yields to caller.
  - Allow shallow clone to run concurrently with creating the container depending on the devcontainer.json. Err on the safe side.
- Added `--git-clone-branch` and `--git-clone-recurse-submodules`.
- Added `--log-level`, `--terminal-columns` and `--terminal-rows`.

### [0.0.3]
- Added `--git-clone-url` and `--git-clone-token-env-var` to clone a repository concurrently (if possible) to downloading the Docker image.
- Added `--telemetry` and `--environment` for enabling sending of telemetry events.
- Added `--performance-log-file` for logging of performance markers in JSON format.
- Added `--dotfiles-install-command` and `--dotfiles-target-path` complementing `--dotfiles-repository`.
- Added `"remoteUser"` to the result JSON.
