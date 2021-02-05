# Dev Containers CLI

CLI for creating a Dev Container.

To install it, first login with a GitHub PAT with permission to read packages:

```
npm login --registry=https://npm.pkg.github.com
npm install --registry=https://npm.pkg.github.com/microsoft @microsoft/vscode-dev-containers-cli
```

Run it with the npx wrapper and no parameters to see the available options:
```
npx @microsoft/vscode-dev-containers-cli
```
