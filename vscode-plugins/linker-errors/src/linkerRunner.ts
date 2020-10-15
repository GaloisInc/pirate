import * as vscode from "vscode";
import * as child_process from "child_process";

export class LinkerRunner {
  public async runLinker(diagnosticCollection: vscode.DiagnosticCollection) {
    let diagnostics = this.linkProject();
    for (let file of diagnostics.keys()) {
      const fileDiags = diagnostics.get(file);
      if (fileDiags) {
        vscode.workspace.findFiles(file, null, 1).then((uriArray) => {
          diagnosticCollection.set(uriArray[0], fileDiags);
        });
      }
    }
  }

  // This is a bit of a hack, but currently the linker conflict lines are of the
  // form "Symbol <name> needed, but has unmet requirement <capability>".  For
  // now, it seems sufficient to just look at the first word.
  private isLinkerConflict(text: string): boolean {
    console.log("Checking: " + text);
    return text.trimLeft().startsWith("Symbol");
  }

  private parseLinkerError(err: string, loc: string): [vscode.Diagnostic, string] {
    const errWords = err.split(" ");
    const symbolName = errWords[1];
    const capability = errWords[7];

    const locParts = loc.split(":");
    const fileName = locParts[0].split(" ")[2];
    const lineNumber = parseInt(locParts[1]);

    let start: vscode.Position = new vscode.Position(lineNumber, 0);
    let end: vscode.Position = new vscode.Position(lineNumber, 999);
    let range: vscode.Range = new vscode.Range(start, end);
    return [
      new vscode.Diagnostic(
        range,
        "Reference to symbol " + symbolName + " requires the function to have capability " + capability,
        vscode.DiagnosticSeverity.Error),
      fileName];
  }

  private linkProject(): Map<string, Array<vscode.Diagnostic>> {
    let diags: Map<string, Array<vscode.Diagnostic>> = new Map();
    const folders = vscode.workspace.workspaceFolders;
    if (!folders) {
      return diags;
    }
    const options: child_process.ExecOptions = {
      cwd: folders[0].uri.fsPath
    };
    child_process.exec(
      "/home/karl/pirate/pirate-llvm/build/bin/clang -fuse-ld=lld -fdiagnostics-print-source-range-info -fdiagnostics-parseable-fixits -ffunction-sections -fdata-sections -Wl,-enclave,low *.c",
      options,
      (error, stdout, stderr) => {
        const lines = stderr.split("\n");
        let currLine = 0;
        while (currLine < lines.length) {
          let parts = lines[currLine].split(":");
          if (this.isLinkerConflict(parts[2])) {
            const [diag, fileName] = this.parseLinkerError(parts[2], lines[++currLine]);
            let currentDiags = diags.get(fileName);
            if (currentDiags) {
              currentDiags.push(diag);
              diags.set(fileName, currentDiags);
            } else {
              diags.set(fileName, [diag]);
            }
          } else {
            console.log("ignored line: " + lines[currLine]);
          }
          currLine++;
        }
      });
    return diags;
  }
}
