// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import * as child_process from 'child_process';
import * as fs from 'fs';
import * as path from 'path';

export class PirateCppConfigurationProvider
  implements vscode.DebugConfigurationProvider {

  async provideDebugConfigurations(
    folder?: vscode.WorkspaceFolder,
    _token?: vscode.CancellationToken
  ): Promise<vscode.DebugConfiguration[]> {
    const configs: vscode.DebugConfiguration[] = [];

		if (!folder) {
      vscode.window.showErrorMessage(
        "Debugging only works with a workspace folder, not standalone files"
      );
      return configs;
    }
    const dir = path.join(folder.uri.fsPath, ".pirate");

    // if(!fs.existsSync("directory")) {
    //   fs.mkdirSync("directory");
    // }

    if (!this.runPDL(dir, folder)) {
      return configs;
    }

    // Create one configuration per YAML file that PDL has generated
    const pdlOutFiles = fs.readdirSync(dir);
    const yamlFiles = pdlOutFiles.filter((file, _idx, _arr) => {
      return file.endsWith(".yaml");
    });
    yamlFiles.forEach((file, _idx, _arr) => {
      const palConfig: vscode.DebugConfiguration = {
        type: "cppdbg",
        name: file.split(".")[0],
        request: "launch",
        program: "/usr/local/bin/pal",
        args: [file],
        cwd: dir,
        setupCommands: [
          { text: "-gdb-set follow-fork-mode child" }
        ]
      };
      configs.push(palConfig);
    });
    return configs;
  }

  private runPDL(workdir: string, folder: vscode.WorkspaceFolder): boolean {
    const procOpts: child_process.SpawnSyncOptions = {
      cwd: folder.uri.fsPath,
      encoding: "utf8",
      shell: true,
    };
    const cmd: string = "pdl debug " + workdir + " pdl.yaml";
    const pdlResult: child_process.SpawnSyncReturns<Buffer> = child_process.spawnSync(
      cmd,
      procOpts
    );

    if (pdlResult.error) {
      const errText: string = pdlResult.stderr.toString("utf8");
      vscode.window.showErrorMessage("Error from PDL: " + errText);
      return false;
    }
    return true;
  }
}

// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	console.log('Congratulations, your extension "piratedebug" is now active!');

	const provider: PirateCppConfigurationProvider = new PirateCppConfigurationProvider();
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('cppdbg', provider))

	// The command has been defined in the package.json file
	// Now provide the implementation of the command with registerCommand
	// The commandId parameter must match the command field in package.json
	let command = vscode.commands.registerCommand('extension.helloWorld', () => {
		// The code you place here will be executed every time your command is executed

		// Display a message box to the user
		vscode.window.showInformationMessage('Hello World!');
	});

	context.subscriptions.push(command);
}

// this method is called when your extension is deactivated
export function deactivate() {}
