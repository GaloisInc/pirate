"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.LinkerRunner = void 0;
const vscode = require("vscode");
const child_process = require("child_process");
const tmp = require("tmp");
const lookpath = require("lookpath");
class LinkerRunner {
    // The public entry point for the linker, which just needs a way to register diagnostics.
    runLinker(diagnosticCollection) {
        return __awaiter(this, void 0, void 0, function* () {
            const buildDir = tmp.dirSync();
            const opts = {
                cwd: buildDir.name,
                encoding: 'utf8',
                shell: true,
            };
            let buildCommands = yield this.runCmake(opts);
            if (!buildCommands) {
                console.log('Fatal error while running CMake prevented linker extension from running');
                return;
            }
            let diagnostics = yield this.linkProject(buildCommands, opts);
            for (let file of diagnostics.keys()) {
                const fileDiags = diagnostics.get(file);
                if (fileDiags) {
                    const uniqueDiags = this.makeUnique(fileDiags);
                    const lastSlash = file.lastIndexOf('/') + 1;
                    const fileToFind = file.substr(lastSlash, file.length - lastSlash);
                    vscode.workspace
                        .findFiles(fileToFind, null, 1)
                        .then((uriArray) => __awaiter(this, void 0, void 0, function* () {
                        const newDiags = yield this.adjustRangeStart(uriArray[0], uniqueDiags);
                        diagnosticCollection.set(uriArray[0], newDiags);
                    }));
                }
            }
        });
    }
    // Eliminate duplicate diagnostics, which requires looking just at the error
    // message and range since the whole object will generally compare as not being
    // equal no matter what.
    makeUnique(items) {
        items.sort((lhs, rhs) => {
            if (lhs.message < rhs.message) {
                return -1;
            }
            else {
                return 1;
            }
        });
        let uniqueItems = new Array();
        for (let i = 0; i < items.length; i++) {
            uniqueItems.push(items[i]);
            while (i < items.length - 1 &&
                items[i].message === items[i + 1].message &&
                items[i].range.start.line === items[i + 1].range.start.line) {
                i++;
            }
        }
        return uniqueItems;
    }
    // Determines if a line of output text is one we want to parse
    isLinkerConflict(text) {
        return text.trimLeft().startsWith('ld.lld: error: Symbol');
    }
    // Parses linker errors of the following specific form (assumes the ld.lld:
    // error bit has been trimmed already)
    //
    // Symbol foo needed, but has unmet requirement high
    // used at main.c:7:(.text.main+0x15)
    //
    // The two parameters are these two lines of text
    parseLinkerError(err, loc) {
        const errWords = err.trimLeft().split(' ');
        const symbolName = errWords[1];
        const capability = errWords[7];
        const locParts = loc.trimLeft().split(':');
        const fileName = locParts[0].split(' ')[2];
        const lineNumber = parseInt(locParts[1]);
        // We don't know the real start character so we set it to zero and adjust it later.
        let range = new vscode.Range(lineNumber - 1, 0, lineNumber - 1, 999);
        return [
            new vscode.Diagnostic(range, 'Reference to symbol ' +
                symbolName +
                ' requires the function to have capability ' +
                capability, vscode.DiagnosticSeverity.Error),
            fileName,
        ];
    }
    // For a diagnostic and the corresponding file Uri, we adjust the range of the
    // diagnostic to skip over whitespace characters at the start of the line.
    // This is a purely aesthetic and probably somewhat slow operation, but
    // highlighting potential many whitespace characters looks quite bad.
    adjustRangeStart(uri, diags) {
        return __awaiter(this, void 0, void 0, function* () {
            const bytes = yield vscode.workspace.fs.readFile(uri);
            const lines = bytes.toString().split('\n');
            diags.forEach((diag, _idx, _arr) => {
                const line = lines[diag.range.start.line];
                diag.range = new vscode.Range(diag.range.start.line, line.search(/\S|$/), diag.range.end.line, 999);
            });
            return diags;
        });
    }
    buildDiagMap(diags) {
        let map = new Map();
        for (const [d, filename] of diags) {
            const currentFileDiags = map.get(filename);
            if (currentFileDiags) {
                currentFileDiags.push(d);
                map.set(filename, currentFileDiags);
            }
            else {
                map.set(filename, [d]);
            }
        }
        return map;
    }
    // The main entry point for calling out to clang, providing a mapping from
    // filenames to arrays of errors
    linkProject(buildCommands, opts) {
        return __awaiter(this, void 0, void 0, function* () {
            let cmdDiags = new Array();
            for (const cmd of buildCommands) {
                if (cmd !== '') {
                    const newDiags = yield this.runBuildCommand(cmd, opts);
                    cmdDiags = cmdDiags.concat(newDiags);
                }
            }
            return this.buildDiagMap(cmdDiags);
        });
    }
    // Run a single command that has been extracted from the makefile, looking for
    // errors in the output
    runBuildCommand(cmd, opts) {
        return __awaiter(this, void 0, void 0, function* () {
            let diagnostics = new Array();
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
        });
    }
    // A helper function that checks whether an executable exists on the PATH, so
    // we can give better errors in error cases.
    executableExists(exeName) {
        return __awaiter(this, void 0, void 0, function* () {
            let path = yield lookpath.lookpath(exeName);
            return path !== undefined;
        });
    }
    // Try to get the compiler/linker commands that cmake would have used to build
    // a project.
    runCmake(opts) {
        return __awaiter(this, void 0, void 0, function* () {
            const folders = vscode.workspace.workspaceFolders;
            if (!folders) {
                console.log('The Pirate Linker extension requires you to open a workspace');
                return undefined;
            }
            else if (!this.executableExists('cmake')) {
                console.log("Can't find cmake on the PATH");
                return undefined;
            }
            let cmakeResult = child_process.spawnSync('cmake', [
                '-G',
                '"Unix Makefiles"',
                '-DCMAKE_BUILD_TYPE=Debug',
                folders[0].uri.fsPath,
            ], opts);
            let makeResult = child_process.spawnSync('make', ['--dry-run'], opts);
            return makeResult.stdout.toString('utf8').split('\n');
        });
    }
}
exports.LinkerRunner = LinkerRunner;
//# sourceMappingURL=linkerRunner.js.map