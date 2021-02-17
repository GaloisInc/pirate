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
const vscode = require("vscode");
const clangd_context_1 = require("./clangd-context");
/**
 *  This method is called when the extension is activated. The extension is
 *  activated the very first time a command is executed.
 */
function activate(context) {
    return __awaiter(this, void 0, void 0, function* () {
        const outputChannel = vscode.window.createOutputChannel('clangd');
        context.subscriptions.push(outputChannel);
        const clangdContext = new clangd_context_1.ClangdContext;
        context.subscriptions.push(clangdContext);
        // An empty place holder for the activate command, otherwise we'll get an
        // "command is not registered" error.
        context.subscriptions.push(vscode.commands.registerCommand('clangd.activate', () => __awaiter(this, void 0, void 0, function* () { })));
        context.subscriptions.push(vscode.commands.registerCommand('clangd.restart', () => __awaiter(this, void 0, void 0, function* () {
            yield clangdContext.dispose();
            yield clangdContext.activate(context.globalStoragePath, outputChannel);
        })));
        yield clangdContext.activate(context.globalStoragePath, outputChannel);
    });
}
exports.activate = activate;
//# sourceMappingURL=extension.js.map