import * as vscode from 'vscode';
import * as LR from './linkerRunner';

let diagnosticCollection: vscode.DiagnosticCollection;

export function activate(context: vscode.ExtensionContext) {
  console.log('Congratulations, your extension "linker-errors" is now active!');

  diagnosticCollection = vscode.languages.createDiagnosticCollection(
    'pirate-linker'
  );

  let disposable = vscode.commands.registerCommand(
    'linker-errors.helloWorld',
    async () => {
      let lr: LR.LinkerRunner = new LR.LinkerRunner();
      lr.runLinker(diagnosticCollection);
    }
  );

  context.subscriptions.push(disposable);
  context.subscriptions.push(diagnosticCollection);
}

// this method is called when your extension is deactivated
export function deactivate() {}
