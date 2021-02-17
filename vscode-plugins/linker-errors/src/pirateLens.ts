// pirateLens
import * as vscode from 'vscode';
import * as path from 'path';
import { LinkerRunner } from './linkerRunner';

export class PirateLens implements vscode.CodeLensProvider {
  private codeLenses: vscode.CodeLens[] = [];
  private linkerRunner: LinkerRunner;

  constructor(lr: LinkerRunner) {
    this.linkerRunner = lr;
  }

  private parseLocation(loc: string): [string, number] {
    const parts = loc.split(':');
    return [parts[0], Number.parseInt(parts[1])];
  }

  provideCodeLenses(
    document: vscode.TextDocument,
    _token: vscode.CancellationToken
  ): vscode.ProviderResult<vscode.CodeLens[]> {
    console.log('providing code lenses');
    let lenses = new Array<vscode.CodeLens>();
    const json = this.linkerRunner.latestJSON;
    json.forEach((item: any) => {
      let [file, line] = this.parseLocation(item.location);
      if (path.basename(file) === path.basename(document.uri.fsPath)) {
        const range = new vscode.Range(line - 1, 0, line - 1, 999);
        let cmd = {
          title: "Function requires capability '" + item.caps + "'",
          tooltip: "Function requires capability '" + item.caps + "'",
          command: '',
        };
        lenses.push(new vscode.CodeLens(range, cmd));
      }
    });
    return lenses;
  }
}
