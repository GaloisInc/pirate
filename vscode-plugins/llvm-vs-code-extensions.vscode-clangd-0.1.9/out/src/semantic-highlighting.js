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
const fs = require("fs");
const jsonc = require("jsonc-parser");
const path = require("path");
const vscode = require("vscode");
const vscodelc = require("vscode-languageclient/node");
function activate(context) {
    const feature = new SemanticHighlightingFeature(context);
    context.subscriptions.push(feature);
    context.client.registerFeature(feature);
}
exports.activate = activate;
// Language server push notification providing the semantic highlighting
// information for a text document.
const NotificationType = new vscodelc.NotificationType('textDocument/semanticHighlighting');
// The feature that should be registered in the vscode lsp for enabling
// experimental semantic highlighting.
class SemanticHighlightingFeature {
    constructor(context) {
        // Any disposables that should be cleaned up when clangd crashes.
        this.subscriptions = [];
        context.subscriptions.push(context.client.onDidChangeState(({ newState }) => {
            if (newState == vscodelc.State.Running) {
                // Register handler for semantic highlighting notification.
                context.client.onNotification(NotificationType, this.handleNotification.bind(this));
            }
            else if (newState == vscodelc.State.Stopped) {
                // Dispose resources when clangd crashes.
                this.dispose();
            }
        }));
    }
    fillClientCapabilities(capabilities) {
        // Extend the ClientCapabilities type and add semantic highlighting
        // capability to the object.
        const textDocumentCapabilities = capabilities.textDocument;
        textDocumentCapabilities.semanticHighlightingCapabilities = {
            semanticHighlighting: true,
        };
    }
    loadCurrentTheme() {
        return __awaiter(this, void 0, void 0, function* () {
            const themeRuleMatcher = new ThemeRuleMatcher(yield loadTheme(vscode.workspace.getConfiguration('workbench')
                .get('colorTheme')));
            this.highlighter.initialize(themeRuleMatcher);
        });
    }
    initialize(capabilities, documentSelector) {
        // The semantic highlighting capability information is in the capabilities
        // object but to access the data we must first extend the ServerCapabilities
        // type.
        const serverCapabilities = capabilities;
        if (!serverCapabilities.semanticHighlighting)
            return;
        this.scopeLookupTable = serverCapabilities.semanticHighlighting.scopes;
        // Important that highlighter is created before the theme is loading as
        // otherwise it could try to update the themeRuleMatcher without the
        // highlighter being created.
        this.highlighter = new Highlighter(this.scopeLookupTable);
        this.subscriptions.push(vscode.Disposable.from(this.highlighter));
        // Adds a listener to reload the theme when it changes.
        this.subscriptions.push(vscode.workspace.onDidChangeConfiguration((conf) => {
            if (!conf.affectsConfiguration('workbench.colorTheme'))
                return;
            this.loadCurrentTheme();
        }));
        this.loadCurrentTheme();
        // Event handling for handling with TextDocuments/Editors lifetimes.
        this.subscriptions.push(vscode.window.onDidChangeVisibleTextEditors((editors) => editors.forEach((e) => this.highlighter.applyHighlights(e.document.uri))));
        this.subscriptions.push(vscode.workspace.onDidCloseTextDocument((doc) => this.highlighter.removeFileHighlightings(doc.uri)));
    }
    handleNotification(params) {
        const lines = params.lines.map((line) => ({ line: line.line, tokens: decodeTokens(line.tokens) }));
        this.highlighter.highlight(vscode.Uri.parse(params.textDocument.uri), lines);
    }
    // Disposes of all disposable resources used by this object.
    dispose() {
        this.subscriptions.forEach((d) => d.dispose());
        this.subscriptions = [];
    }
}
exports.SemanticHighlightingFeature = SemanticHighlightingFeature;
// Converts a string of base64 encoded tokens into the corresponding array of
// HighlightingTokens.
function decodeTokens(tokens) {
    const scopeMask = 0xFFFF;
    const lenShift = 0x10;
    const uint32Size = 4;
    const buf = Buffer.from(tokens, 'base64');
    const retTokens = [];
    for (let i = 0, end = buf.length / uint32Size; i < end; i += 2) {
        const start = buf.readUInt32BE(i * uint32Size);
        const lenKind = buf.readUInt32BE((i + 1) * uint32Size);
        const scopeIndex = lenKind & scopeMask;
        const len = lenKind >>> lenShift;
        retTokens.push({ character: start, scopeIndex: scopeIndex, length: len });
    }
    return retTokens;
}
exports.decodeTokens = decodeTokens;
// The main class responsible for processing of highlightings that clangd
// sends.
class Highlighter {
    constructor(scopeLookupTable) {
        // Maps uris with currently open TextDocuments to the current highlightings.
        this.files = new Map();
        // DecorationTypes for the current theme that are used when highlighting. A
        // SemanticHighlightingToken with scopeIndex i should have the decoration at
        // index i in this list.
        this.decorationTypes = [];
        this.scopeLookupTable = scopeLookupTable;
    }
    dispose() {
        this.files.clear();
        this.decorationTypes.forEach((t) => t.dispose());
        // Dispose must not be not called multiple times if initialize is
        // called again.
        this.decorationTypes = [];
    }
    // This function must be called at least once or no highlightings will be
    // done. Sets the theme that is used when highlighting. Also triggers a
    // recolorization for all current highlighters. Should be called whenever the
    // theme changes and has been loaded. Should also be called when the first
    // theme is loaded.
    initialize(themeRuleMatcher) {
        this.decorationTypes.forEach((t) => t.dispose());
        this.decorationTypes = this.scopeLookupTable.map((scopes) => {
            const options = {
                // If there exists no rule for this scope the matcher returns an empty
                // color. That's ok because vscode does not do anything when applying
                // empty decorations.
                color: themeRuleMatcher.getBestThemeRule(scopes[0]).foreground,
                // If the rangeBehavior is set to Open in any direction the
                // highlighting becomes weird in certain cases.
                rangeBehavior: vscode.DecorationRangeBehavior.ClosedClosed,
            };
            return vscode.window.createTextEditorDecorationType(options);
        });
        this.getVisibleTextEditorUris().forEach((fileUri) => this.applyHighlights(fileUri));
    }
    // Adds incremental highlightings to the current highlightings for the file
    // with fileUri. Also applies the highlightings to any associated
    // TextEditor(s).
    highlight(fileUri, highlightingLines) {
        const fileUriStr = fileUri.toString();
        if (!this.files.has(fileUriStr)) {
            this.files.set(fileUriStr, new Map());
        }
        const fileHighlightings = this.files.get(fileUriStr);
        highlightingLines.forEach((line) => fileHighlightings.set(line.line, line));
        this.applyHighlights(fileUri);
    }
    // Applies all the highlightings currently stored for a file with fileUri.
    applyHighlights(fileUri) {
        const fileUriStr = fileUri.toString();
        if (!this.files.has(fileUriStr))
            // There are no highlightings for this file, must return early or will get
            // out of bounds when applying the decorations below.
            return;
        if (!this.decorationTypes.length)
            // Can't apply any decorations when there is no theme loaded.
            return;
        // This must always do a full re-highlighting due to the fact that
        // TextEditorDecorationType are very expensive to create (which makes
        // incremental updates infeasible). For this reason one
        // TextEditorDecorationType is used per scope.
        const ranges = this.getDecorationRanges(fileUri);
        vscode.window.visibleTextEditors.forEach((e) => {
            if (e.document.uri.toString() !== fileUriStr)
                return;
            this.decorationTypes.forEach((d, i) => e.setDecorations(d, ranges[i]));
        });
    }
    // Called when a text document is closed. Removes any highlighting entries for
    // the text document that was closed.
    removeFileHighlightings(fileUri) {
        // If there exists no entry the call to delete just returns false.
        this.files.delete(fileUri.toString());
    }
    // Gets the uris as strings for the currently visible text editors.
    getVisibleTextEditorUris() {
        return vscode.window.visibleTextEditors.map((e) => e.document.uri);
    }
    // Returns the ranges that should be used when decorating. Index i in the
    // range array has the decoration type at index i of this.decorationTypes.
    getDecorationRanges(fileUri) {
        const fileUriStr = fileUri.toString();
        if (!this.files.has(fileUriStr))
            // this.files should always have an entry for fileUri if we are here. But
            // if there isn't one we don't want to crash the extension. This is also
            // useful for tests.
            return [];
        const lines = Array.from(this.files.get(fileUriStr).values());
        const decorations = this.decorationTypes.map(() => []);
        lines.forEach((line) => {
            line.tokens.forEach((token) => {
                decorations[token.scopeIndex].push(new vscode.Range(new vscode.Position(line.line, token.character), new vscode.Position(line.line, token.character + token.length)));
            });
        });
        return decorations;
    }
}
exports.Highlighter = Highlighter;
class ThemeRuleMatcher {
    constructor(rules) {
        // A cache for the getBestThemeRule function.
        this.bestRuleCache = new Map();
        this.themeRules = rules;
    }
    // Returns the best rule for a scope.
    getBestThemeRule(scope) {
        if (this.bestRuleCache.has(scope))
            return this.bestRuleCache.get(scope);
        let bestRule = { scope: '', foreground: '' };
        this.themeRules.forEach((rule) => {
            // The best rule for a scope is the rule that is the longest prefix of the
            // scope (unless a perfect match exists in which case the perfect match is
            // the best). If a rule is not a prefix and we tried to match with longest
            // common prefix instead variables would be highlighted as `less`
            // variables when using Light+ (as variable.other would be matched against
            // variable.other.less in this case). Doing common prefix matching also
            // means we could match variable.cpp to variable.css if variable.css
            // occurs before variable in themeRules.
            // FIXME: This is not defined in the TextMate standard (it is explicitly
            // undefined, https://macromates.com/manual/en/scope_selectors). Might
            // want to rank some other way.
            if (scope.startsWith(rule.scope) &&
                rule.scope.length > bestRule.scope.length)
                // This rule matches and is more specific than the old rule.
                bestRule = rule;
        });
        this.bestRuleCache.set(scope, bestRule);
        return bestRule;
    }
}
exports.ThemeRuleMatcher = ThemeRuleMatcher;
// Get all token color rules provided by the theme.
function loadTheme(themeName) {
    const extension = vscode.extensions.all.find((extension) => {
        const contribs = extension.packageJSON.contributes;
        if (!contribs || !contribs.themes)
            return false;
        return contribs.themes.some((theme) => theme.id === themeName ||
            theme.label === themeName);
    });
    if (!extension) {
        return Promise.reject('Could not find a theme with name: ' + themeName);
    }
    const themeInfo = extension.packageJSON.contributes.themes.find((theme) => theme.id === themeName || theme.label === themeName);
    return parseThemeFile(path.join(extension.extensionPath, themeInfo.path));
}
/**
 * Parse the TextMate theme at fullPath. If there are multiple TextMate scopes
 * of the same name in the include chain only the earliest entry of the scope is
 * saved.
 * @param fullPath The absolute path to the theme.
 * @param seenScopes A set containing the name of the scopes that have already
 *     been set.
 */
function parseThemeFile(fullPath, seenScopes) {
    return __awaiter(this, void 0, void 0, function* () {
        if (!seenScopes)
            seenScopes = new Set();
        // FIXME: Add support for themes written as .tmTheme.
        if (path.extname(fullPath) === '.tmTheme')
            return [];
        try {
            const contents = yield readFileText(fullPath);
            const parsed = jsonc.parse(contents);
            const rules = [];
            // To make sure it does not crash if tokenColors is undefined.
            if (!parsed.tokenColors)
                parsed.tokenColors = [];
            parsed.tokenColors.forEach((rule) => {
                if (!rule.scope || !rule.settings || !rule.settings.foreground)
                    return;
                const textColor = rule.settings.foreground;
                // Scopes that were found further up the TextMate chain should not be
                // overwritten.
                const addColor = (scope) => {
                    if (seenScopes.has(scope))
                        return;
                    rules.push({ scope, foreground: textColor });
                    seenScopes.add(scope);
                };
                if (rule.scope instanceof Array) {
                    return rule.scope.forEach((s) => addColor(s));
                }
                addColor(rule.scope);
            });
            if (parsed.include)
                // Get all includes and merge into a flat list of parsed json.
                return [
                    ...(yield parseThemeFile(path.join(path.dirname(fullPath), parsed.include), seenScopes)),
                    ...rules
                ];
            return rules;
        }
        catch (err) {
            // If there is an error opening a file, the TextMate files that were
            // correctly found and parsed further up the chain should be returned.
            // Otherwise there will be no highlightings at all.
            console.warn('Could not open file: ' + fullPath + ', error: ', err);
        }
        return [];
    });
}
exports.parseThemeFile = parseThemeFile;
function readFileText(path) {
    return new Promise((resolve, reject) => {
        fs.readFile(path, 'utf8', (err, data) => {
            if (err) {
                return reject(err);
            }
            return resolve(data);
        });
    });
}
//# sourceMappingURL=semantic-highlighting.js.map