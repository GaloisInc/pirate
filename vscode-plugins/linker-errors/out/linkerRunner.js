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
        });
    }
    parseOutputLine(line) {
        let start = new vscode.Position(3, 1);
        let end = new vscode.Position(3, 120);
        let range = new vscode.Range(start, end);
        return [
            new vscode.Diagnostic(range, "Karl's diagnostic", vscode.DiagnosticSeverity.Error),
            "foo.c"
        ];
    }
    linkProject() {
        let diags = new Array();
        const folders = vscode.workspace.workspaceFolders;
        if (!folders) {
            return diags;
        }
        const options = {
            cwd: folders[0].uri.fsPath
        };
        child_process.exec("/home/karl/galois/pirate/pirate-llvm/build/bin/clang -fuse-ld=lld -fdiagnostics-print-source-range-info -fdiagnostics-parseable-fixits -ffunction-sections -fdata-sections -Wl,-enclave,green *.c", options, (error, stdout, stderr) => {
            let lines = stderr.split("\n");
            lines.forEach((line, idx, arr) => {
                let parts = line.split(":");
                if (parts[0] === "ld.lld") {
                    diags.push(this.parseOutputLine(parts[2]));
                }
                else {
                    console.log("other: " + line);
                }
            });
        });
        return diags;
    }
}
exports.LinkerRunner = LinkerRunner;
//# sourceMappingURL=linkerRunner.js.map