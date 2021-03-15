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
exports.deactivate = exports.activate = void 0;
const vscode = require("vscode");
const LR = require("./linkerRunner");
const pirateLens_1 = require("./pirateLens");
let diagnosticCollection;
let linkerRunner;
function activate(context) {
    console.log('Congratulations, your extension "linker-errors" is now active!');
    diagnosticCollection = vscode.languages.createDiagnosticCollection('linker-errors');
    linkerRunner = new LR.LinkerRunner();
    let runLinkerDisposable = vscode.commands.registerCommand('linker-errors.runLinker', () => __awaiter(this, void 0, void 0, function* () {
        linkerRunner.runLinker(vscode.workspace.getConfiguration('linker-errors'), diagnosticCollection);
    }));
    let folders = vscode.workspace.workspaceFolders;
    if (!folders) {
        console.log("Linker extension doesn't work without a workspace folder");
    }
    else {
        // TODO: should probably also look at cmake files, maybe be configurable in general?
        const glob = new vscode.RelativePattern(folders[0], '**/*.{cpp,hpp,c,h,}');
        let watcher = vscode.workspace.createFileSystemWatcher(glob, false, false, false);
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
        const pirateLensProvider = new pirateLens_1.PirateLens(linkerRunner);
        vscode.languages.registerCodeLensProvider('*', pirateLensProvider);
        linkerRunner.runLinker(vscode.workspace.getConfiguration('linker-errors'), diagnosticCollection);
    }
}
exports.activate = activate;
// this method is called when your extension is deactivated
function deactivate() { }
exports.deactivate = deactivate;
//# sourceMappingURL=extension.js.map