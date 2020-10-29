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
let diagnosticCollection;
function activate(context) {
    console.log('Congratulations, your extension "linker-errors" is now active!');
    diagnosticCollection = vscode.languages.createDiagnosticCollection('linker-errors');
    let disposable = vscode.commands.registerCommand('linker-errors.runLinker', () => __awaiter(this, void 0, void 0, function* () {
        let lr = new LR.LinkerRunner();
        lr.runLinker(vscode.workspace.getConfiguration('linker-errors'), diagnosticCollection);
    }));
    context.subscriptions.push(disposable);
    context.subscriptions.push(diagnosticCollection);
}
exports.activate = activate;
// this method is called when your extension is deactivated
function deactivate() { }
exports.deactivate = deactivate;
//# sourceMappingURL=extension.js.map