"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const path = require("path");
const vscode = require("vscode");
// Gets the config value `clangd.<key>`. Applies ${variable} substitutions.
function get(key) {
    return substitute(vscode.workspace.getConfiguration('clangd').get(key));
}
exports.get = get;
// Sets the config value `clangd.<key>`. Does not apply substitutions.
function update(key, value, target) {
    return vscode.workspace.getConfiguration('clangd').update(key, value, target);
}
exports.update = update;
// Traverse a JSON value, replacing placeholders in all strings.
function substitute(val) {
    if (typeof val == 'string') {
        val = val.replace(/\$\{(.*?)\}/g, (match, name) => {
            const rep = replacement(name);
            // If there's no replacement available, keep the placeholder.
            return (rep === null) ? match : rep;
        });
    }
    else if (Array.isArray(val))
        val = val.map((x) => substitute(x));
    else if (typeof val == 'object') {
        // Substitute values but not keys, so we don't deal with collisions.
        const result = {};
        for (let [k, v] of Object.entries(val))
            result[k] = substitute(v);
        val = result;
    }
    return val;
}
// Subset of substitution variables that are most likely to be useful.
// https://code.visualstudio.com/docs/editor/variables-reference
function replacement(name) {
    if (name == 'workspaceRoot' || name == 'workspaceFolder' || name == 'cwd') {
        if (vscode.workspace.rootPath !== undefined)
            return vscode.workspace.rootPath;
        if (vscode.window.activeTextEditor !== undefined)
            return path.dirname(vscode.window.activeTextEditor.document.uri.fsPath);
        return process.cwd();
    }
    const envPrefix = 'env:';
    if (name.startsWith(envPrefix))
        return process.env[name.substr(envPrefix.length)] || '';
    const configPrefix = 'config:';
    if (name.startsWith(configPrefix)) {
        const config = vscode.workspace.getConfiguration().get(name.substr(configPrefix.length));
        return (typeof config == 'string') ? config : null;
    }
    return null;
}
//# sourceMappingURL=config.js.map