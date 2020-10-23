import * as vscode from 'vscode';
import * as child_process from 'child_process';

export class LinkerRunner {
  // The public entry point for the linker, which just needs a way to register diagnostics.
  public async runLinker(diagnosticCollection: vscode.DiagnosticCollection) {
    let diagnostics = this.linkProject();
    for (let file of diagnostics.keys()) {
      const fileDiags = diagnostics.get(file);
      if (fileDiags) {
        this.makeUnique(fileDiags);
        vscode.workspace.findFiles(file, null, 1).then(async (uriArray) => {
          const newDiags = await this.adjustRangeStart(uriArray[0], fileDiags);
          diagnosticCollection.set(uriArray[0], newDiags);
        });
      }
    }
  }

  // Eliminate duplicate diagnostics, which requires looking just at the error
  // message and range since the whole object will generally compare as not being
  // equal no matter what.
  private makeUnique(
    items: Array<vscode.Diagnostic>
  ): Array<vscode.Diagnostic> {
    return items;
  }

  // Determines if a line of output text is one we want to parse
  private isLinkerConflict(text: string): boolean {
    return text.trimLeft().startsWith('ld.lld');
  }

  // Parses linker errors of the following specific form (assumes the ld.lld:
  // error bit has been trimmed already)
  //
  // Symbol foo needed, but has unmet requirement high
  // used at main.c:7:(.text.main+0x15)
  //
  // The two parameters are these two lines of text
  private parseLinkerError(
    err: string,
    loc: string
  ): [vscode.Diagnostic, string] {
    const errWords = err.trimLeft().split(' ');
    const symbolName = errWords[1];
    const capability = errWords[7];

    const locParts = loc.trimLeft().split(':');
    const fileName = locParts[0].split(' ')[2];
    const lineNumber = parseInt(locParts[1]);

    // We don't know the real start character so we set it to zero and adjust it later.
    let range: vscode.Range = new vscode.Range(
      lineNumber - 1,
      0,
      lineNumber - 1,
      999
    );
    return [
      new vscode.Diagnostic(
        range,
        'Reference to symbol ' +
          symbolName +
          ' requires the function to have capability ' +
          capability,
        vscode.DiagnosticSeverity.Error
      ),
      fileName,
    ];
  }

  // For a diagnostic and the corresponding file Uri, we adjust the range of the
  // diagnostic to skip over whitespace characters at the start of the line. This
  // is a purely aesthetic and probably somewhat slow operation, but highlighting
  // potential many whitespace characters looks quite bad.
  private async adjustRangeStart(
    uri: vscode.Uri,
    diags: Array<vscode.Diagnostic>
  ): Promise<Array<vscode.Diagnostic>> {
    const bytes = await vscode.workspace.fs.readFile(uri);
    const lines = bytes.toString().split('\n');
    diags.forEach((diag, _idx, _arr) => {
      const line = lines[diag.range.start.line];
      diag.range = new vscode.Range(
        diag.range.start.line,
        line.search(/\S|$/),
        diag.range.end.line,
        999
      );
    });
    return diags;
  }

  // The main entry point for calling out to clang, providing a mapping from
  // filenames to arrays of errors
  private linkProject(): Map<string, Array<vscode.Diagnostic>> {
    let diags: Map<string, Array<vscode.Diagnostic>> = new Map();
    const folders = vscode.workspace.workspaceFolders;
    if (!folders) {
      return diags;
    }
    const options: child_process.SpawnSyncOptions = {
      cwd: folders[0].uri.fsPath,
      encoding: 'utf8',
      shell: true,
    };
    let procResult = child_process.spawnSync(
      '/home/karl/galois/pirate/pirate-llvm/build/bin/clang',
      [
        '-g',
        '-fuse-ld=lld',
        '-ffunction-sections',
        '-fdata-sections',
        '-Wl,-enclave,low',
        '*.c',
      ],
      options
    );

    const errText = procResult.stderr.toString('utf8');
    const lines = errText.split('\n');
    let currLine = 0;
    while (currLine < lines.length - 1) {
      if (this.isLinkerConflict(lines[currLine])) {
        const parts = lines[currLine].split(':');
        const [diag, fileName] = this.parseLinkerError(
          parts[2],
          lines[currLine + 1]
        );
        currLine++;
        const currentDiags = diags.get(fileName);
        if (currentDiags) {
          currentDiags.push(diag);
          diags.set(fileName, currentDiags);
        } else {
          diags.set(fileName, [diag]);
        }
      }
      currLine++;
    }
    return diags;
  }
}
