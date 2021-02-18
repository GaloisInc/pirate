import * as vscode from 'vscode';
import * as LR from './linkerRunner';
import { PirateLens } from './pirateLens';

let diagnosticCollection: vscode.DiagnosticCollection;
let linkerRunner: LR.LinkerRunner;

export function activate(context: vscode.ExtensionContext) {
  console.log('Congratulations, your extension "linker-errors" is now active!');

  diagnosticCollection = vscode.languages.createDiagnosticCollection(
    'linker-errors'
  );

  linkerRunner = new LR.LinkerRunner();
  let runLinkerDisposable = vscode.commands.registerCommand(
    'linker-errors.runLinker',
    async () => {
      linkerRunner.runLinker(
        vscode.workspace.getConfiguration('linker-errors'),
        diagnosticCollection
      );
    }
  );

  let folders = vscode.workspace.workspaceFolders;
  if (!folders) {
    console.log("Linker extension doesn't work without a workspace folder");
  } else {
    // TODO: should probably also look at cmake files, maybe be configurable in general?
    const glob = new vscode.RelativePattern(folders[0], '**/*.{cpp,hpp,c,h,}');
    let watcher = vscode.workspace.createFileSystemWatcher(
      glob,
      false,
      false,
      false
    );
    watcher.onDidChange((_uri) => {
      console.log('Did change, trying to run linker');
      vscode.commands.executeCommand('linker-errors.runLinker');
    });
    watcher.onDidCreate((_uri) => {
      console.log('File created, trying to run linker');
      vscode.commands.executeCommand('linker-errors.runLinker');
    });
    watcher.onDidDelete((_uri) => {
      console.log('File deleted, trying to run linker');
      vscode.commands.executeCommand('linker-errors.runLinker');
    });

    context.subscriptions.push(runLinkerDisposable);
    context.subscriptions.push(diagnosticCollection);
    context.subscriptions.push(watcher);

    const pirateLensProvider = new PirateLens(linkerRunner);
    vscode.languages.registerCodeLensProvider('*', pirateLensProvider);
    linkerRunner.runLinker(
      vscode.workspace.getConfiguration('linker-errors'),
      diagnosticCollection
    );
  }
}

// this method is called when your extension is deactivated
export function deactivate() {}
