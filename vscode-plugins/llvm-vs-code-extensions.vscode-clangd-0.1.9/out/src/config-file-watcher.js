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
const config = require("./config");
function activate(context) {
    if (config.get('onConfigChanged') != 'ignore') {
        context.client.registerFeature(new ConfigFileWatcherFeature(context));
    }
}
exports.activate = activate;
class ConfigFileWatcherFeature {
    constructor(context) {
        this.context = context;
    }
    fillClientCapabilities(capabilities) { }
    initialize(capabilities, _documentSelector) {
        var _a;
        if ((_a = capabilities
            .compilationDatabase) === null || _a === void 0 ? void 0 : _a.automaticReload)
            return;
        this.context.subscriptions.push(new ConfigFileWatcher(this.context));
    }
    dispose() { }
}
class ConfigFileWatcher {
    constructor(context) {
        this.context = context;
        this.databaseWatcher = undefined;
        this.debounceTimer = undefined;
        this.createFileSystemWatcher();
        context.subscriptions.push(vscode.workspace.onDidChangeWorkspaceFolders(() => { this.createFileSystemWatcher(); }));
    }
    dispose() {
        if (this.databaseWatcher)
            this.databaseWatcher.dispose();
    }
    createFileSystemWatcher() {
        if (this.databaseWatcher)
            this.databaseWatcher.dispose();
        if (vscode.workspace.workspaceFolders) {
            this.databaseWatcher = vscode.workspace.createFileSystemWatcher('{' +
                vscode.workspace.workspaceFolders.map(f => f.uri.fsPath).join(',') +
                '}/{build/compile_commands.json,compile_commands.json,compile_flags.txt}');
            this.context.subscriptions.push(this.databaseWatcher.onDidChange(this.debouncedHandleConfigFilesChanged.bind(this)));
            this.context.subscriptions.push(this.databaseWatcher.onDidCreate(this.debouncedHandleConfigFilesChanged.bind(this)));
            this.context.subscriptions.push(this.databaseWatcher);
        }
    }
    debouncedHandleConfigFilesChanged(uri) {
        return __awaiter(this, void 0, void 0, function* () {
            if (this.debounceTimer) {
                clearTimeout(this.debounceTimer);
            }
            this.debounceTimer = setTimeout(() => __awaiter(this, void 0, void 0, function* () {
                yield this.handleConfigFilesChanged(uri);
                this.debounceTimer = undefined;
            }), 2000);
        });
    }
    handleConfigFilesChanged(uri) {
        return __awaiter(this, void 0, void 0, function* () {
            // Sometimes the tools that generate the compilation database, before
            // writing to it, they create a new empty file or they clear the existing
            // one, and after the compilation they write the new content. In this cases
            // the server is not supposed to restart
            if ((yield vscode.workspace.fs.stat(uri)).size <= 0)
                return;
            switch (config.get('onConfigChanged')) {
                case 'restart':
                    vscode.commands.executeCommand('clangd.restart');
                    break;
                case 'ignore':
                    break;
                case 'prompt':
                default:
                    switch (yield vscode.window.showInformationMessage(`Clangd configuration file at '${uri.fsPath}' has been changed. Do you want to restart it?`, 'Yes', 'Yes, always', 'No, never')) {
                        case 'Yes':
                            vscode.commands.executeCommand('clangd.restart');
                            break;
                        case 'Yes, always':
                            vscode.commands.executeCommand('clangd.restart');
                            config.update('onConfigChanged', 'restart', vscode.ConfigurationTarget.Global);
                            break;
                        case 'No, never':
                            config.update('onConfigChanged', 'ignore', vscode.ConfigurationTarget.Global);
                            break;
                        default:
                            break;
                    }
                    break;
            }
        });
    }
}
//# sourceMappingURL=config-file-watcher.js.map