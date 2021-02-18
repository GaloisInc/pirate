"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const vscode = require("vscode");
const vscodelc = require("vscode-languageclient/node");
function activate(context) {
    const status = new FileStatus();
    context.subscriptions.push(vscode.Disposable.from(status));
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(() => { status.updateStatus(); }));
    context.subscriptions.push(context.client.onDidChangeState(({ newState }) => {
        if (newState == vscodelc.State.Running) {
            // clangd starts or restarts after crash.
            context.client.onNotification('textDocument/clangd.fileStatus', (fileStatus) => { status.onFileUpdated(fileStatus); });
        }
        else if (newState == vscodelc.State.Stopped) {
            // Clear all cached statuses when clangd crashes.
            status.clear();
        }
    }));
}
exports.activate = activate;
class FileStatus {
    constructor() {
        this.statuses = new Map();
        this.statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 10);
    }
    onFileUpdated(fileStatus) {
        const filePath = vscode.Uri.parse(fileStatus.uri);
        this.statuses.set(filePath.fsPath, fileStatus);
        this.updateStatus();
    }
    updateStatus() {
        const activeDoc = vscode.window.activeTextEditor.document;
        // Work around https://github.com/microsoft/vscode/issues/58869
        // Don't hide the status when activeTextEditor is output panel.
        // This aligns with the behavior of other panels, e.g. problems.
        if (activeDoc.uri.scheme == 'output')
            return;
        const status = this.statuses.get(activeDoc.fileName);
        if (!status) {
            this.statusBarItem.hide();
            return;
        }
        this.statusBarItem.text = `clangd: ` + status.state;
        this.statusBarItem.show();
    }
    clear() {
        this.statuses.clear();
        this.statusBarItem.hide();
    }
    dispose() { this.statusBarItem.dispose(); }
}
//# sourceMappingURL=file-status.js.map