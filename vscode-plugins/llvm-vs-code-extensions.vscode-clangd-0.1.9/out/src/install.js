"use strict";
// Automatically install clangd binary releases from GitHub.
// This wraps `@clangd/install` in the VSCode UI. See that package for more.
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
const common = require("@clangd/install");
const path = require("path");
const vscode = require("vscode");
const config = require("./config");
// Returns the clangd path to be used, or null if clangd is not installed.
function activate(context, globalStoragePath) {
    return __awaiter(this, void 0, void 0, function* () {
        const ui = new UI(context, globalStoragePath);
        context.subscriptions.push(vscode.commands.registerCommand('clangd.install', () => __awaiter(this, void 0, void 0, function* () { return common.installLatest(ui); })));
        context.subscriptions.push(vscode.commands.registerCommand('clangd.update', () => __awaiter(this, void 0, void 0, function* () { return common.checkUpdates(true, ui); })));
        const status = yield common.prepare(ui, config.get('checkUpdates'));
        return status.clangdPath;
    });
}
exports.activate = activate;
class UI {
    constructor(context, globalStoragePath) {
        this.context = context;
        this.globalStoragePath = globalStoragePath;
    }
    get storagePath() { return this.globalStoragePath; }
    choose(prompt, options) {
        return __awaiter(this, void 0, void 0, function* () {
            return yield vscode.window.showInformationMessage(prompt, ...options);
        });
    }
    slow(title, result) {
        const opts = {
            location: vscode.ProgressLocation.Notification,
            title: title,
            cancellable: false,
        };
        return Promise.resolve(vscode.window.withProgress(opts, () => result));
    }
    progress(title, cancel, body) {
        const opts = {
            location: vscode.ProgressLocation.Notification,
            title: title,
            cancellable: cancel != null,
        };
        const result = vscode.window.withProgress(opts, (progress, canc) => __awaiter(this, void 0, void 0, function* () {
            if (cancel)
                canc.onCancellationRequested((_) => cancel.abort());
            let lastFraction = 0;
            return body(fraction => {
                if (fraction > lastFraction) {
                    progress.report({ increment: 100 * (fraction - lastFraction) });
                    lastFraction = fraction;
                }
            });
        }));
        return Promise.resolve(result); // Thenable to real promise.
    }
    error(s) { vscode.window.showErrorMessage(s); }
    info(s) { vscode.window.showInformationMessage(s); }
    command(name, body) {
        this.context.subscriptions.push(vscode.commands.registerCommand(name, body));
    }
    shouldReuse(release) {
        return __awaiter(this, void 0, void 0, function* () {
            const message = `clangd ${release} is already installed!`;
            const use = 'Use the installed version';
            const reinstall = 'Delete it and reinstall';
            const response = yield vscode.window.showInformationMessage(message, use, reinstall);
            if (response == use) {
                // Find clangd within the existing directory.
                return true;
            }
            else if (response == reinstall) {
                // Remove the existing installation.
                return false;
            }
            else {
                // User dismissed prompt, bail out.
                return undefined;
            }
        });
    }
    promptReload(message) {
        return __awaiter(this, void 0, void 0, function* () {
            if (yield vscode.window.showInformationMessage(message, 'Reload window'))
                vscode.commands.executeCommand('workbench.action.reloadWindow');
        });
    }
    showHelp(message, url) {
        return __awaiter(this, void 0, void 0, function* () {
            if (yield vscode.window.showInformationMessage(message, 'Open website'))
                vscode.env.openExternal(vscode.Uri.parse(url));
        });
    }
    promptUpdate(oldVersion, newVersion) {
        return __awaiter(this, void 0, void 0, function* () {
            const message = 'An updated clangd language server is available.\n ' +
                `Would you like to upgrade to clangd ${newVersion}? ` +
                `(from ${oldVersion})`;
            const update = `Install clangd ${newVersion}`;
            const dontCheck = 'Don\'t ask again';
            const response = yield vscode.window.showInformationMessage(message, update, dontCheck);
            if (response == update) {
                common.installLatest(this);
            }
            else if (response == dontCheck) {
                config.update('checkUpdates', false, vscode.ConfigurationTarget.Global);
            }
        });
    }
    promptInstall(version) {
        return __awaiter(this, void 0, void 0, function* () {
            const p = this.clangdPath;
            let message = '';
            if (p.indexOf(path.sep) < 0) {
                message += `The '${p}' language server was not found on your PATH.\n`;
            }
            else {
                message += `The clangd binary '${p}' was not found.\n`;
            }
            message += `Would you like to download and install clangd ${version}?`;
            if (yield vscode.window.showInformationMessage(message, 'Install'))
                common.installLatest(this);
        });
    }
    get clangdPath() {
        let p = config.get('path');
        // Backwards compatibility: if it's a relative path with a slash, interpret
        // relative to project root.
        if (!path.isAbsolute(p) && p.indexOf(path.sep) != -1 &&
            vscode.workspace.rootPath !== undefined)
            p = path.join(vscode.workspace.rootPath, p);
        return p;
    }
    set clangdPath(p) {
        config.update('path', p, vscode.ConfigurationTarget.Global);
    }
}
//# sourceMappingURL=install.js.map