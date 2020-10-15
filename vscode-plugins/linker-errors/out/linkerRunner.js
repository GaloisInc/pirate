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
class LinkerRunner {
    runLinker(diagnosticCollection) {
        return __awaiter(this, void 0, void 0, function* () {
            let diagnostics = this.linkProject();
            for (let file of diagnostics.keys()) {
                const fileDiags = diagnostics.get(file);
                if (fileDiags) {
                    vscode.workspace.findFiles(file, null, 1).then((uriArray) => {
                        diagnosticCollection.set(uriArray[0], fileDiags);
                    });
                }
            }
        });
    }
    // This is a bit of a hack, but currently the linker conflict lines are of the
    // form "Symbol <name> needed, but has unmet requirement <capability>".  For
    // now, it seems sufficient to just look at the first word.
    isLinkerConflict(text) {
        console.log("Checking: " + text);
        return text.trimLeft().startsWith("Symbol");
    }
    parseLinkerError(err, loc) {
        const errWords = err.split(" ");
        const symbolName = errWords[1];
        const capability = errWords[7];
        const locParts = loc.split(":");
        const fileName = locParts[0].split(" ")[2];
        const lineNumber = parseInt(locParts[1]);
        let start = new vscode.Position(lineNumber, 0);
        let end = new vscode.Position(lineNumber, 999);
        let range = new vscode.Range(start, end);
        return [
            new vscode.Diagnostic(range, "Reference to symbol " + symbolName + " requires the function to have capability " + capability, vscode.DiagnosticSeverity.Error),
            fileName
        ];
    }
    linkProject() {
        let diags = new Map();
        const folders = vscode.workspace.workspaceFolders;
        if (!folders) {
            return diags;
        }
        const options = {
            cwd: folders[0].uri.fsPath
        };
        child_process.exec("/home/karl/pirate/pirate-llvm/build/bin/clang -fuse-ld=lld -fdiagnostics-print-source-range-info -fdiagnostics-parseable-fixits -ffunction-sections -fdata-sections -Wl,-enclave,low *.c", options, (error, stdout, stderr) => {
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
                    }
                    else {
                        diags.set(fileName, [diag]);
                    }
                }
                else {
                    console.log("ignored line: " + lines[currLine]);
                }
                currLine++;
            }
        });
        return diags;
    }
}
exports.LinkerRunner = LinkerRunner;
//# sourceMappingURL=linkerRunner.js.map