import * as path from 'path'

import * as vscode from 'vscode'

import * as A from '../shared/architecture'
import { TextRange } from '../shared/position'
import { common } from '../shared/webviewProtocol'

import * as tracker from './docUpdater'
import { ModelWebview, ModelWebviewServices } from './modelWebview'
import * as parser from './parser'

/**
 * This is a cell that holds what edit to apply next (if any)
 */
type NextEditCell = { next : null|(() => void)}

function mkvscodeRange(r:TextRange):vscode.Range {
    const start = new vscode.Position(r.start.line, r.start.character)
    const end = new vscode.Position(r.end.line, r.end.character)
    return new vscode.Range(start, end)
}

/**
 * Create vscode diagnostic from parser error message.
 */
function diagnosticFromParserError(e : parser.Error):vscode.Diagnostic {
    return new vscode.Diagnostic(mkvscodeRange(e), e.message)
}

function addPortSymbol(syms:vscode.DocumentSymbol[], p:A.Port) {
    const kind = vscode.SymbolKind.Property
    const range = mkvscodeRange(p.definition)
    const selRange = mkvscodeRange(p.name)
    const psym = new vscode.DocumentSymbol(p.name.value, 'port', kind, range, selRange)
    syms.push(psym)

}

function mkSymbols(mdl:A.SystemModel):vscode.DocumentSymbol[] {
    const symbols:vscode.DocumentSymbol[]=[]
    for (const a of mdl.actors) {
        const kind = vscode.SymbolKind.Interface
        const range = mkvscodeRange(a.definition)
        const selRange = mkvscodeRange(a.name)
        const asym = new vscode.DocumentSymbol(a.name.value, 'actor', kind, range, selRange)
        symbols.push(asym)
        for (const p of a.inPorts) { addPortSymbol(asym.children, p) }
        for (const p of a.outPorts) { addPortSymbol(asym.children, p) }
    }
    return symbols
}

/**
 * Manages state corresponding to a particular URI
 */
export class ModelResources implements ModelWebviewServices {
    /**
     * Disagnostic collection to add diagnostics to.
     */
    readonly #dc: vscode.DiagnosticCollection

    readonly #logger: (msg:string) => void

    readonly #uri: vscode.Uri

    // Active webviews open for this URI
    readonly #activeWebviews: Set<ModelWebview> = new Set()

    private system: A.SystemModel|null=null

    private trackedDoc:tracker.TrackedDoc|null=null

    getDocumentSymbols(): vscode.DocumentSymbol[] {
        return this.system ? mkSymbols(this.system) : []
    }

    #onNext:null|NextEditCell = null

    /** System layout associated with document */
    constructor(dc: vscode.DiagnosticCollection, logger: (msg:string) => void, doc:vscode.TextDocument) {
        this.#dc = dc
        this.#logger = logger
        this.#uri = doc.uri
        this.updateModel(doc)
    }

    debugLog(msg:string): void { this.#logger(msg) }
    /**
     * Parse the document and update the system model.
     *
     * Note. This should be invoked any time a new file is opened or changed
     * in a modification not driven by out plugin.
     */
    private updateModel(doc:vscode.TextDocument):void {
        this.#onNext = null
        try {
            const uri = doc.uri
            const t = new tracker.TrackedDoc(uri)
            const res = parser.parseArchitectureFile(doc.getText(), t)
            this.#dc.set(uri, res.errors.map(diagnosticFromParserError))
            const model = res.value
            if (model !== undefined) {
                this.system = model
                this.trackedDoc = t
                for (const view of this.#activeWebviews) { view.setModel(model) }
            } else {
                this.system = null
                this.trackedDoc = null
            }
        } catch (e) {
            this.system = null
            this.trackedDoc = null
            this.#logger('Exception parsing model.')
        }
    }


    #expectedEdits: null|tracker.NormalizedEdits=null

    /**
     * Called whenever vscode notifies us that a text document changed.
     */
    public onDidChangeTextDocument(e: vscode.TextDocumentChangeEvent): void {
        if (this.trackedDoc
            && this.#expectedEdits
            && this.trackedDoc.allExpected(this.#expectedEdits, e.contentChanges)) {

            this.#expectedEdits = null
            return
        }
        this.updateModel(e.document)
    }

    showDocument(idx:A.LocationIndex):void {
        if (this.trackedDoc === null) return
        if (idx >= this.trackedDoc.locations.length) return
        const thisDirectoryPath = path.dirname(this.#uri.fsPath)
        const loc = this.trackedDoc.locations[idx].loc
        const uri = vscode.Uri.file(path.join(thisDirectoryPath, loc.filename))
        const pos = new vscode.Position(loc.line, loc.column)
        const range = new vscode.Range(pos, pos)
        vscode.window.showTextDocument(uri, {selection: range})
    }

    /**
     * Synchronize edits from a webview.
     */
    synchronizeEdits(source:ModelWebview, edits: readonly common.TrackUpdate[]):void {
        const doc = this.trackedDoc
        if (!doc)  return
        const edit = tracker.NormalizedEdits.fromArray(edits)

        const lastOnNext = this.#onNext
        const thisOnNext: NextEditCell = { next: null }
        this.#onNext = thisOnNext
        const doEdit = () => {
            this.#expectedEdits = edit
            for (const v of this.#activeWebviews) {
                if (v !== source) { v.notifyDocumentEdited(edit.array) }
            }
            const wsEdit = doc.mkWorkspaceEdit(edit)
            vscode.workspace.applyEdit(wsEdit).then((success) => {
                if (success) doc.updateRanges(edit)
                if (thisOnNext.next) { thisOnNext.next() } else { this.#onNext = null }

            }, (rsn) => {
                this.#logger(`Edit failed${rsn}`)
                if (thisOnNext.next) { thisOnNext.next() } else { this.#onNext = null }
            })
        }
        if (lastOnNext === null) { doEdit() } else { lastOnNext.next = doEdit }
    }

    /**
     * Called when pirate graph viewer is opened
     */
    public resolvePirateGraphViewer(context: vscode.ExtensionContext, webviewPanel: vscode.WebviewPanel): void {
        // Create webview
        const view:ModelWebview = new ModelWebview(context, this, webviewPanel, this.system)
        // Append new system model webview to active webviews.
        this.#activeWebviews.add(view)
        // Make sure we get rid of the listener when our editor is closed.
        webviewPanel.onDidDispose(() => {
            this.#logger('dispose Webview')
            this.#activeWebviews.delete(view)
            view.dispose()
        })
    }
}
