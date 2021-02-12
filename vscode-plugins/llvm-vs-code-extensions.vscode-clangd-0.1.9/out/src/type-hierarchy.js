"use strict";
// This file implements the client side of the proposed type hierarchy
// extension to LSP. The proposal can be found at
// https://github.com/microsoft/vscode-languageserver-node/pull/426.
// Clangd supports the server side of this protocol.
// The feature allows querying the base and derived classes of the
// symbol under the cursor, which are visualized in a tree view.
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
    const feature = new TypeHierarchyFeature(context);
    context.client.registerFeature(feature);
}
exports.activate = activate;
var TypeHierarchyDirection;
(function (TypeHierarchyDirection) {
    TypeHierarchyDirection.Children = 0;
    TypeHierarchyDirection.Parents = 1;
    TypeHierarchyDirection.Both = 2;
})(TypeHierarchyDirection = exports.TypeHierarchyDirection || (exports.TypeHierarchyDirection = {}));
var TypeHierarchyRequest;
(function (TypeHierarchyRequest) {
    TypeHierarchyRequest.type = new vscodelc.RequestType('textDocument/typeHierarchy');
})(TypeHierarchyRequest || (TypeHierarchyRequest = {}));
var ResolveTypeHierarchyRequest;
(function (ResolveTypeHierarchyRequest) {
    ResolveTypeHierarchyRequest.type = new vscodelc.RequestType('typeHierarchy/resolve');
})(ResolveTypeHierarchyRequest = exports.ResolveTypeHierarchyRequest || (exports.ResolveTypeHierarchyRequest = {}));
// A dummy node used to indicate that a node has multiple parents
// when we are in Children mode.
const dummyNode = {
    name: '[multiple parents]',
    kind: vscodelc.SymbolKind.Null,
    uri: '',
    range: { start: { line: 0, character: 0 }, end: { line: 0, character: 0 } },
    selectionRange: { start: { line: 0, character: 0 }, end: { line: 0, character: 0 } },
};
class TypeHierarchyTreeItem extends vscode.TreeItem {
    constructor(item) {
        super(item.name);
        if (item.children) {
            if (item.children.length == 0) {
                this.collapsibleState = vscode.TreeItemCollapsibleState.None;
            }
            else {
                this.collapsibleState = vscode.TreeItemCollapsibleState.Expanded;
            }
        }
        else {
            this.collapsibleState = vscode.TreeItemCollapsibleState.Collapsed;
        }
        // Do not register actions for the dummy node.
        if (item == dummyNode) {
            return;
        }
        // Make the item respond to a single-click by navigating to the
        // definition of the class.
        this.command = {
            arguments: [item],
            command: 'clangd.typeHierarchy.gotoItem',
            title: 'Go to'
        };
    }
}
class TypeHierarchyFeature {
    constructor(context) {
        this.serverSupportsTypeHierarchy = false;
        new TypeHierarchyProvider(context);
        context.subscriptions.push(context.client.onDidChangeState(stateChange => {
            this.state = stateChange.newState;
            this.recomputeEnableTypeHierarchy();
        }));
    }
    fillClientCapabilities(capabilities) { }
    fillInitializeParams(_params) { }
    initialize(capabilities, documentSelector) {
        const serverCapabilities = capabilities;
        if (serverCapabilities.typeHierarchyProvider) {
            this.serverSupportsTypeHierarchy = true;
            this.recomputeEnableTypeHierarchy();
        }
    }
    dispose() { }
    recomputeEnableTypeHierarchy() {
        if (this.state == vscodelc.State.Running) {
            vscode.commands.executeCommand('setContext', 'clangd.enableTypeHierarchy', this.serverSupportsTypeHierarchy);
        }
        else if (this.state == vscodelc.State.Stopped) {
            vscode.commands.executeCommand('setContext', 'clangd.enableTypeHierarchy', false);
        }
    }
}
class TypeHierarchyProvider {
    constructor(context) {
        this._onDidChangeTreeData = new vscode.EventEmitter();
        this.onDidChangeTreeData = this._onDidChangeTreeData.event;
        this.client = context.client;
        context.subscriptions.push(vscode.window.registerTreeDataProvider('clangd.typeHierarchyView', this));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand('clangd.typeHierarchy', this.reveal, this));
        context.subscriptions.push(vscode.commands.registerCommand('clangd.typeHierarchy.close', this.close, this));
        context.subscriptions.push(vscode.commands.registerCommand('clangd.typeHierarchy.gotoItem', this.gotoItem, this));
        context.subscriptions.push(vscode.commands.registerCommand('clangd.typeHierarchy.viewParents', () => this.setDirection(TypeHierarchyDirection.Parents)));
        context.subscriptions.push(vscode.commands.registerCommand('clangd.typeHierarchy.viewChildren', () => this.setDirection(TypeHierarchyDirection.Children)));
        this.treeView = vscode.window.createTreeView('clangd.typeHierarchyView', { treeDataProvider: this });
        context.subscriptions.push(this.treeView);
        // Show children by default.
        this.direction = TypeHierarchyDirection.Children;
    }
    gotoItem(item) {
        return __awaiter(this, void 0, void 0, function* () {
            const uri = vscode.Uri.parse(item.uri);
            const range = this.client.protocol2CodeConverter.asRange(item.selectionRange);
            const doc = yield vscode.workspace.openTextDocument(uri);
            let editor;
            if (doc) {
                editor = yield vscode.window.showTextDocument(doc, undefined);
            }
            else {
                editor = vscode.window.activeTextEditor;
            }
            if (!editor) {
                return;
            }
            editor.revealRange(range, vscode.TextEditorRevealType.InCenter);
            editor.selection = new vscode.Selection(range.start, range.end);
        });
    }
    setDirection(direction) {
        return __awaiter(this, void 0, void 0, function* () {
            this.direction = direction;
            // Recompute the root based on the starting item.
            this.root = this.computeRoot();
            this._onDidChangeTreeData.fire(null);
            // Re-focus the starting item, which may not be the root.
            this.treeView.reveal(this.startingItem, { focus: true })
                .then(() => { }, (reason) => {
                // Sometimes TreeView.reveal() fails. It's unclear why, and it does
                // not appear to have any visible effects, but vscode complains if you
                // don't handle the rejection promise, so we do so and log a warning.
                console.log('Warning: TreeView.reveal() failed for reason: ' +
                    reason);
            });
        });
    }
    getTreeItem(element) {
        return new TypeHierarchyTreeItem(element);
    }
    getParent(element) {
        // This function is implemented so that VSCode lets us call
        // this.treeView.reveal().
        if (element.parents) {
            if (element.parents.length == 1) {
                return element.parents[0];
            }
            else if (element.parents.length > 1) {
                return dummyNode;
            }
        }
        return null;
    }
    getChildren(element) {
        return __awaiter(this, void 0, void 0, function* () {
            if (!this.root)
                return [];
            if (!element)
                return [this.root];
            if (this.direction == TypeHierarchyDirection.Parents) {
                // Clangd always resolves parents eagerly, so just return them.
                return element.parents;
            }
            // Otherwise, this.direction == Children.
            if (!element.children) {
                // Children are not resolved yet, resolve them now.
                const resolved = yield this.client.sendRequest(ResolveTypeHierarchyRequest.type, {
                    item: element,
                    direction: TypeHierarchyDirection.Children,
                    resolve: 1
                });
                element.children = resolved.children;
            }
            return element.children;
        });
    }
    computeRoot() {
        // In Parents mode, the root is always the starting item.
        if (this.direction == TypeHierarchyDirection.Parents) {
            return this.startingItem;
        }
        // In Children mode, we also include base classes of
        // the starting item as parents. If we encounter a class
        // with multiple bases, we show a dummy node with the label
        // "[multiple parents]" instead.
        let root = this.startingItem;
        while (root.parents && root.parents.length == 1) {
            root = root.parents[0];
        }
        if (root.parents && root.parents.length > 1) {
            dummyNode.children = [root];
            // Do not set "root.parents = [dummyNode]".
            // This would discard the real parents and we'd have to re-query
            // them if entering Parents mode.
            // Instead, we teach getParent() to return the dummy node if
            // there are multiple parents.
            root = dummyNode;
        }
        return root;
    }
    reveal(editor) {
        return __awaiter(this, void 0, void 0, function* () {
            // This makes the type hierarchy view visible by causing the condition
            // "when": "extension.vscode-clangd.typeHierarchyVisible" from
            // package.json to evaluate to true.
            vscode.commands.executeCommand('setContext', 'clangd.typeHierarchyVisible', true);
            const item = yield this.client.sendRequest(TypeHierarchyRequest.type, Object.assign(Object.assign({}, this.client.code2ProtocolConverter.asTextDocumentPositionParams(editor.document, editor.selection.active)), { 
                // Resolve up to 5 initial levels. Any additional levels will be resolved
                // on the fly if the user expands the tree item.
                resolve: 5, 
                // Resolve both directions initially. That way, if the user switches
                // to the Parents view, we have the data already. Note that clangd
                // does not support resolving parents via typeHierarchy/resolve,
                // so otherwise we'd have to remember the TextDocumentPositionParams
                // to make another textDocument/typeHierarchy request when switching
                // to Parents view.
                direction: TypeHierarchyDirection.Both }));
            if (item) {
                this.startingItem = item;
                this.root = this.computeRoot();
                this._onDidChangeTreeData.fire(null);
                // This focuses the "explorer" view container which contains the
                // type hierarchy view.
                vscode.commands.executeCommand('workbench.view.explorer');
                // This expands and focuses the type hierarchy view.
                // Focus the item on which the operation was invoked, not the
                // root (which could be its ancestor or the dummy node).
                this.treeView.reveal(this.startingItem, { focus: true })
                    .then(() => { }, (reason) => {
                    // Sometimes TreeView.reveal() fails. It's unclear why, and it does
                    // not appear to have any visible effects, but vscode complains if
                    // you don't handle the rejection promise, so we do so and log a
                    // warning.
                    console.log('Warning: TreeView.reveal() failed for reason: ' +
                        reason);
                });
            }
            else {
                vscode.window.showInformationMessage('No type hierarchy available for selection');
            }
        });
    }
    close() {
        // Hide the type hierarchy view.
        vscode.commands.executeCommand('setContext', 'clangd.typeHierarchyVisible', false);
        this.root = undefined;
        this._onDidChangeTreeData.fire(null);
    }
}
//# sourceMappingURL=type-hierarchy.js.map