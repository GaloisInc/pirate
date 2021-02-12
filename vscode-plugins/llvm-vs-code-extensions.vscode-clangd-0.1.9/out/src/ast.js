"use strict";
// Implements the "ast dump" feature: textDocument/ast.
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
    const feature = new ASTFeature(context);
    context.client.registerFeature(feature);
}
exports.activate = activate;
const ASTRequestType = new vscodelc.RequestType('textDocument/ast');
class ASTFeature {
    constructor(context) {
        this.context = context;
        // The adapter holds the currently inspected node.
        const adapter = new TreeAdapter();
        // Create the AST view, showing data from the adapter.
        const tree = vscode.window.createTreeView('clangd.ast', { treeDataProvider: adapter });
        context.subscriptions.push(tree, 
        // Ensure the AST view is visible exactly when the adapter has a node.
        // clangd.ast.hasData controls the view visibility (package.json).
        adapter.onDidChangeTreeData((_) => {
            vscode.commands.executeCommand('setContext', 'clangd.ast.hasData', adapter.hasRoot());
            // Show the AST tree even if it's beet collapsed or closed.
            // reveal(root) fails here: "Data tree node not found".
            if (adapter.hasRoot())
                tree.reveal(null);
        }), vscode.window.registerTreeDataProvider('clangd.ast', adapter), 
        // Create the "Show AST" command for the context menu.
        // It's only shown if the feature is dynamicaly available (package.json)
        vscode.commands.registerTextEditorCommand('clangd.ast', (editor, _edit) => __awaiter(this, void 0, void 0, function* () {
            const converter = this.context.client.code2ProtocolConverter;
            const item = yield this.context.client.sendRequest(ASTRequestType, {
                textDocument: converter.asTextDocumentIdentifier(editor.document),
                range: converter.asRange(editor.selection),
            });
            if (!item)
                vscode.window.showInformationMessage('No AST node at selection');
            adapter.setRoot(item, editor.document.uri);
        })), 
        // Clicking "close" will empty the adapter, which in turn hides the
        // view.
        vscode.commands.registerCommand('clangd.ast.close', () => adapter.setRoot(null, null)));
    }
    fillClientCapabilities(capabilities) { }
    // The "Show AST" command is enabled if the server advertises the capability.
    initialize(capabilities, _documentSelector) {
        vscode.commands.executeCommand('setContext', 'clangd.ast.supported', 'astProvider' in capabilities);
    }
    dispose() { }
}
// Icons used for nodes of particular roles and kinds. (Kind takes precedence).
// IDs from https://code.visualstudio.com/api/references/icons-in-labels
// We're uncomfortably coupled to the concrete roles and kinds from clangd:
// https://github.com/llvm/llvm-project/blob/main/clang-tools-extra/clangd/DumpAST.cpp
// There are only a few roles, corresponding to base AST node types.
const RoleIcons = {
    'type': 'symbol-misc',
    'declaration': 'symbol-function',
    'expression': 'primitive-dot',
    'specifier': 'list-tree',
    'statement': 'symbol-event',
    'template argument': 'symbol-type-parameter',
};
// Kinds match Stmt::StmtClass etc, corresponding to AST node subtypes.
// In principle these could overlap, but in practice they don't.
const KindIcons = {
    'Compound': 'json',
    'Recovery': 'error',
    'TranslationUnit': 'file-code',
    'PackExpansion': 'ellipsis',
    'TemplateTypeParm': 'symbol-type-parameter',
    'TemplateTemplateParm': 'symbol-type-parameter',
    'TemplateParamObject': 'symbol-type-parameter',
};
// Primary text shown for this node.
function describe(role, kind) {
    // For common roles where the kind is fairly self-explanatory, we don't
    // include it. e.g. "Call" rather than "Call expression".
    if (role == 'expression' || role == 'statement' || role == 'declaration' ||
        role == 'template name')
        return kind;
    return kind + ' ' + role;
}
// Map a root ASTNode onto a VSCode tree.
class TreeAdapter {
    constructor() {
        this._onDidChangeTreeData = new vscode.EventEmitter();
        this.onDidChangeTreeData = this._onDidChangeTreeData.event;
    }
    hasRoot() { return this.root != null; }
    setRoot(newRoot, newDoc) {
        this.root = newRoot;
        this.doc = newDoc;
        this._onDidChangeTreeData.fire(/*root changed*/ null);
    }
    getTreeItem(node) {
        const item = new vscode.TreeItem(describe(node.role, node.kind));
        if (node.children && node.children.length > 0)
            item.collapsibleState = vscode.TreeItemCollapsibleState.Expanded;
        item.description = node.detail;
        item.tooltip = node.arcana;
        const icon = KindIcons[node.kind] || RoleIcons[node.role];
        if (icon)
            item.iconPath = new vscode.ThemeIcon(icon);
        // Clicking on the node should highlight it in the source.
        if (node.range && this.doc) {
            item.command = {
                title: 'Jump to',
                command: 'vscode.open',
                arguments: [
                    this.doc, {
                        preserveFocus: true,
                        selection: node.range,
                    }
                ],
            };
        }
        return item;
    }
    getChildren(element) {
        if (!element)
            return this.root ? [this.root] : [];
        return element.children || [];
    }
    getParent(node) {
        if (node == this.root)
            return null;
        function findUnder(parent) {
            for (const child of parent.children || []) {
                const result = (node == child) ? parent : findUnder(child);
                if (result)
                    return result;
            }
            return null;
        }
        return findUnder(this.root);
    }
}
//# sourceMappingURL=ast.js.map