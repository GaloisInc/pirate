import * as child_process from 'child_process';
import * as fs from 'fs';
import * as lookpath from 'lookpath';
import * as path from 'path';
import * as tmp from 'tmp';
import * as vscode from 'vscode';

export class LinkerRunner {
  public isRunning: boolean = false;
  public latestJSON: any = [];

  // The public entry point for the linker, which just needs a way to register diagnostics.
  public async runLinker(
    config: vscode.WorkspaceConfiguration,
    diagnosticCollection: vscode.DiagnosticCollection
  ) {
    if (this.isRunning) {
      console.log('Tried to run, but already running');
      return;
    }
    console.log('Running linker');
    this.isRunning = true;
    diagnosticCollection.clear();

    const buildDir = tmp.dirSync();
    const opts: child_process.SpawnSyncOptions = {
      cwd: buildDir.name,
      encoding: 'utf8',
      shell: true,
    };
    let buildCommands = await this.runCmake(
      opts,
      config.get('make.makePath'),
      config.get('cmake.cmakePath'),
      config.get('cmake.cmakeFlags')
    );
    if (!buildCommands) {
      console.log(
        'Fatal error while running CMake prevented linker extension from running'
      );
      return;
    }
    let diagnostics = await this.linkProject(buildCommands, opts);
    for (let file of diagnostics.keys()) {
      const fileDiags = diagnostics.get(file);
      if (fileDiags) {
        const uniqueDiags = this.makeUnique(fileDiags);
        const lastSlash = file.lastIndexOf('/') + 1;
        const fileToFind = file.substr(lastSlash, file.length - lastSlash);
        vscode.workspace
          .findFiles(fileToFind, null, 1)
          .then(async (uriArray) => {
            const newDiags = await this.adjustRangeStart(
              uriArray[0],
              uniqueDiags
            );
            diagnosticCollection.set(uriArray[0], newDiags);
          });
      }
    }
    this.isRunning = false;
    this.updateJSON(buildDir.name);
  }

  private updateJSON(tmpPath: string) {
    const jsonFile = path.join(tmpPath, 'enclaves.json');
    if (fs.existsSync(jsonFile)) {
      const jsonBuf = fs.readFileSync(jsonFile);
      let jsonWithDuplicates = JSON.parse(jsonBuf.toString('utf8'));
      let json: any[] = [];
      for (let i = 0, len = jsonWithDuplicates.length; i < len; i++) {
        let unique = true;
        for (let j = 0, newLen = json.length; j < newLen; j++) {
          if (jsonWithDuplicates[i].location === json[j].location) {
            unique = false;
          }
        }
        if (unique) {
          json.push(jsonWithDuplicates[i]);
        }
      }
      this.latestJSON = json;
    } else {
      console.log("Couldn't find JSON dump");
    }
  }

  // Eliminate duplicate diagnostics, which requires looking just at the error
  // message and range since the whole object will generally compare as not being
  // equal no matter what.
  private makeUnique(
    items: Array<vscode.Diagnostic>
  ): Array<vscode.Diagnostic> {
    items.sort((lhs, rhs): number => {
      if (lhs.message < rhs.message) {
        return -1;
      } else {
        return 1;
      }
    });
    let uniqueItems = new Array<vscode.Diagnostic>();
    for (let i = 0; i < items.length; i++) {
      uniqueItems.push(items[i]);
      while (
        i < items.length - 1 &&
        items[i].message === items[i + 1].message &&
        items[i].range.start.line === items[i + 1].range.start.line
      ) {
        i++;
      }
    }
    return uniqueItems;
  }

  // Determines if a line of output text is one we want to parse
  private isLinkerConflict(text: string): boolean {
    return text.trimLeft().startsWith('ld.lld: error: Symbol');
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
  // diagnostic to skip over whitespace characters at the start of the line.
  // This is a purely aesthetic and probably somewhat slow operation, but
  // highlighting potential many whitespace characters looks quite bad.
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

  private buildDiagMap(
    diags: Array<[vscode.Diagnostic, string]>
  ): Map<string, Array<vscode.Diagnostic>> {
    let map: Map<string, Array<vscode.Diagnostic>> = new Map();
    for (const [d, filename] of diags) {
      const currentFileDiags = map.get(filename);
      if (currentFileDiags) {
        currentFileDiags.push(d);
        map.set(filename, currentFileDiags);
      } else {
        map.set(filename, [d]);
      }
    }
    return map;
  }

  // The main entry point for calling out to clang, providing a mapping from
  // filenames to arrays of errors
  private async linkProject(
    buildCommands: Array<string>,
    opts: child_process.SpawnSyncOptions
  ): Promise<Map<string, Array<vscode.Diagnostic>>> {
    let cmdDiags: Array<[vscode.Diagnostic, string]> = new Array();
    for (const cmd of buildCommands) {
      if (cmd !== '') {
        const newDiags = await this.runBuildCommand(cmd, opts);
        cmdDiags = cmdDiags.concat(newDiags);
      }
    }
    return this.buildDiagMap(cmdDiags);
  }

  // Run a single command that has been extracted from the makefile, looking for
  // errors in the output
  private async runBuildCommand(
    cmd: string,
    opts: child_process.SpawnSyncOptions
  ): Promise<Array<[vscode.Diagnostic, string]>> {
    let diagnostics = new Array<[vscode.Diagnostic, string]>();
    let cmdResult = child_process.spawnSync(cmd, [], opts);

    const errText = cmdResult.stderr.toString('utf8');
    const lines = errText.split('\n');
    let currLine = 0;
    while (currLine < lines.length - 1) {
      if (this.isLinkerConflict(lines[currLine])) {
        const parts = lines[currLine].split(':');
        diagnostics.push(this.parseLinkerError(parts[2], lines[currLine + 1]));
        currLine++;
      }
      currLine++;
    }
    return diagnostics;
  }

  // A helper function that checks whether an executable exists on the PATH, so
  // we can give better errors in error cases.
  private async executableExists(exeName: string): Promise<boolean> {
    let path = await lookpath.lookpath(exeName);
    return path !== undefined;
  }

  // Try to get the compiler/linker commands that cmake would have used to build
  // a project.
  private async runCmake(
    opts: child_process.SpawnSyncOptions,
    makePath: string | undefined,
    cmakePath: string | undefined,
    cmakeFlags: Array<string> | undefined
  ): Promise<Array<string> | undefined> {
    const folders = vscode.workspace.workspaceFolders;

    if (!folders) {
      console.log(
        'The Pirate Linker extension requires you to open a workspace'
      );
      return undefined;
    } else if (!this.executableExists('cmake')) {
      console.log("Can't find cmake on the PATH");
      return undefined;
    }
    if (!cmakePath) {
      cmakePath = 'cmake';
    }
    if (!cmakeFlags) {
      cmakeFlags = [
        '-G',
        '"Unix Makefiles"',
        '-DCMAKE_BUILD_TYPE=Debug',
        '-DCMAKE_C_FLAGS=-Wl,-pirate,enclaves.json',
        // '-DCMAKE_CXX_FLAGS=-Wl,-pirate,enclaves_cxx.json',
      ];
    }
    child_process.spawnSync(
      cmakePath,
      cmakeFlags.concat([folders[0].uri.fsPath]),
      opts
    );
    if (!makePath) {
      makePath = 'make';
    }
    let makeResult = child_process.spawnSync(makePath, ['--dry-run'], opts);
    return makeResult.stdout.toString('utf8').split('\n');
  }
}
