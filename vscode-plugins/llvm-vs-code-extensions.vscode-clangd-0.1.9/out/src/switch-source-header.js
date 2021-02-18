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
const vscodelc = require("vscode-languageclient/node");
function activate(context) {
    context.subscriptions.push(vscode.commands.registerCommand('clangd.switchheadersource', () => switchSourceHeader(context.client)));
}
exports.activate = activate;
var SwitchSourceHeaderRequest;
(function (SwitchSourceHeaderRequest) {
    SwitchSourceHeaderRequest.type = new vscodelc
        .RequestType('textDocument/switchSourceHeader');
})(SwitchSourceHeaderRequest || (SwitchSourceHeaderRequest = {}));
function switchSourceHeader(client) {
    return __awaiter(this, void 0, void 0, function* () {
        const uri = vscode.Uri.file(vscode.window.activeTextEditor.document.fileName);
        if (!uri)
            return;
        const docIdentifier = vscodelc.TextDocumentIdentifier.create(uri.toString());
        const sourceUri = yield client.sendRequest(SwitchSourceHeaderRequest.type, docIdentifier);
        if (!sourceUri) {
            vscode.window.showInformationMessage('Didn\'t find a corresponding file.');
            return;
        }
        const doc = yield vscode.workspace.openTextDocument(vscode.Uri.parse(sourceUri));
        vscode.window.showTextDocument(doc);
    });
}
//# sourceMappingURL=switch-source-header.js.map