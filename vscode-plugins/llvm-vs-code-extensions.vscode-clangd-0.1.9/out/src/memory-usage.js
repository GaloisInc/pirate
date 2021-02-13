"use strict";
// Implements the "memory usage" feature.
// When the server advertises `memoryUsageProvider`, a command
// (clangd.memoryUsage) is available (context variable:
// clangd.memoryUsage.supported). It sends the $/memoryUsage request and
// displays the result in a tree view (clangd.memoryUsage) which becomes visible
// (context: clangd.memoryUsage.hasData)
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
    const feature = new MemoryUsageFeature(context);
    context.client.registerFeature(feature);
}
exports.activate = activate;
exports.MemoryUsageRequest = new vscodelc.RequestType('$/memoryUsage');
function convert(m, title) {
    const slash = Math.max(title.lastIndexOf('/'), title.lastIndexOf('\\'));
    return {
        title: title.substr(slash + 1),
        isFile: slash >= 0,
        total: m._total,
        self: m._self,
        children: Object.keys(m)
            .sort()
            .filter(x => !x.startsWith('_'))
            .map(e => convert(m[e], e))
            .sort((x, y) => y.total - x.total),
    };
}
class MemoryUsageFeature {
    constructor(context) {
        this.context = context;
        const adapter = new TreeAdapter();
        adapter.onDidChangeTreeData((e) => vscode.commands.executeCommand('setContext', 'clangd.memoryUsage.hasData', adapter.root != null));
        this.context.subscriptions.push(vscode.window.registerTreeDataProvider('clangd.memoryUsage', adapter));
        this.context.subscriptions.push(vscode.commands.registerCommand('clangd.memoryUsage', () => __awaiter(this, void 0, void 0, function* () {
            const usage = yield this.context.client.sendRequest(exports.MemoryUsageRequest, {});
            adapter.root = convert(usage, '<root>');
        })));
        this.context.subscriptions.push(vscode.commands.registerCommand('clangd.memoryUsage.close', () => adapter.root = null));
    }
    fillClientCapabilities(capabilities) { }
    fillInitializeParams(_params) { }
    initialize(capabilities, _documentSelector) {
        vscode.commands.executeCommand('setContext', 'clangd.memoryUsage.supported', 'memoryUsageProvider' in capabilities);
    }
    dispose() { }
}
class TreeAdapter {
    constructor() {
        this._onDidChangeTreeData = new vscode.EventEmitter();
        this.onDidChangeTreeData = this._onDidChangeTreeData.event;
    }
    get root() { return this.root_; }
    set root(n) {
        this.root_ = n;
        this._onDidChangeTreeData.fire(/*root changed*/ null);
    }
    getTreeItem(node) {
        const item = new vscode.TreeItem(node.title);
        item.description = (node.total / 1024 / 1024).toFixed(2) + ' MB';
        item.tooltip = `self=${node.self} total=${node.total}`;
        if (node.isFile)
            item.iconPath = new vscode.ThemeIcon('symbol-file');
        else if (!node.children.length)
            item.iconPath = new vscode.ThemeIcon('circle-filled');
        if (node.children.length) {
            if (node.children.length >= 6 || node.isFile)
                item.collapsibleState = vscode.TreeItemCollapsibleState.Collapsed;
            else
                item.collapsibleState = vscode.TreeItemCollapsibleState.Expanded;
        }
        return item;
    }
    getChildren(t) {
        return __awaiter(this, void 0, void 0, function* () {
            if (!t)
                return this.root ? [this.root] : [];
            return t.children;
        });
    }
}
//# sourceMappingURL=memory-usage.js.map