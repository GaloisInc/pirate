import * as vscode from "vscode";
import * as child_process from "child_process";

export class LinkerRunner {
  public async runLinker(diagnosticCollection: vscode.DiagnosticCollection) {
    let diagnostics = this.linkProject();
  }

  private parseOutputLine(line: string): [vscode.Diagnostic, string] {


    let start: vscode.Position = new vscode.Position(3, 1);
    let end: vscode.Position = new vscode.Position(3, 120);
    let range: vscode.Range = new vscode.Range(start, end);
    return [
      new vscode.Diagnostic(
        range,
        "Karl's diagnostic",
        vscode.DiagnosticSeverity.Error),
      "foo.c"];
  }

  private linkProject(): Array<[vscode.Diagnostic, string]> {
    let diags: Array<[vscode.Diagnostic, string]> = new Array();
    const folders = vscode.workspace.workspaceFolders;
    if (!folders) {
      return diags;
    }
    const options: child_process.ExecOptions = {
      cwd: folders[0].uri.fsPath
    };
    child_process.exec(
      "/home/karl/galois/pirate/pirate-llvm/build/bin/clang -fuse-ld=lld -fdiagnostics-print-source-range-info -fdiagnostics-parseable-fixits -ffunction-sections -fdata-sections -Wl,-enclave,green *.c",
      options,
      (error, stdout, stderr) => {
        let lines = stderr.split("\n");
        lines.forEach((line, idx, arr) => {
          let parts = line.split(":");
          if (parts[0] === "ld.lld") {
            diags.push(this.parseOutputLine(parts[2]));
          } else {
            console.log("other: " + line);
          }
        });
      });
    return diags;
  }
}