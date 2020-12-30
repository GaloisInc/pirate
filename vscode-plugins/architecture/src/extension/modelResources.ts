import * as path from 'path'
import * as vscode from 'vscode'

import * as A     from '../shared/architecture'
import { common } from '../shared/webviewProtocol'

import * as tracker from './docUpdater'
import { ModelWebview, ModelWebviewServices } from './modelWebview'
import * as parser from './parser'

/**
 * This is a cell that holds what edit to apply next (if any)
 */
type NextEditCell = { next : null|(() => void)}


/**
 * Create vscode diagnostic from parser error message.
 */
function diagnosticFromParserError(e : parser.Error):vscode.Diagnostic {
	const start = new vscode.Position(e.start.line, e.start.character)
	const end = new vscode.Position(e.end.line, e.end.character)
	const r : vscode.Range = new vscode.Range(start, end)
	return new vscode.Diagnostic(r, e.message)
}

/**
 * Manages state corresponding to a particular URI
 */
export class ModelResources {
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

	#onNext:null|NextEditCell = null

	/** System layout associated with document */
	constructor(dc: vscode.DiagnosticCollection, logger: (msg:string) => void, doc:vscode.TextDocument) {
		this.#dc = dc
		this.#logger = logger
		this.#uri = doc.uri
		this.updateModel(doc)
	}

	/**
	 * Parse the document and update the system model.
	 *
	 * Note. This should be invoked any time a new file is opened or changed
	 * in a modification not driven by out plugin.
	 */
	private updateModel(doc:vscode.TextDocument):void {
		try {
			const uri = doc.uri
			const t = new tracker.TrackedDoc(uri)
			const res = parser.parseArchitectureFile(doc.getText(), t)
			this.#dc.set(uri, res.errors.map(diagnosticFromParserError))
			const model = res.value
			if (model !== undefined) {
				this.system = model
				this.trackedDoc = t
				this.#onNext = null
				for (const view of this.#activeWebviews)
					view.setSystemModel(model)
			}
		} catch (e) {
			this.#logger("Exception parsing model.")
		}
	}

	#expectedEdits: null|tracker.NormalizedEdits=null

	/**
	 * Called whenever vscode notifies us that a text document changed.
	 */
	public onDidChangeTextDocument(e: vscode.TextDocumentChangeEvent) {
		if (this.trackedDoc
			&& this.#expectedEdits
			&& this.trackedDoc.allExpected(this.#expectedEdits, e.contentChanges)) {

			this.#expectedEdits = null
			return
		}
		this.system = null
		this.trackedDoc = null
		this.#onNext = null
		this.updateModel(e.document)
	}

	/**
	 * Called when pirate graph viewer is opened
	 */
	public resolvePirateGraphViewer(context: vscode.ExtensionContext, webviewPanel: vscode.WebviewPanel): void {

		const thisDirectoryPath = path.dirname(this.#uri.fsPath)

		let view:ModelWebview

		// Append new system model webview to active webviews.
		let svc:ModelWebviewServices = {
			showDocument: (idx) => {
				if (this.trackedDoc === null) return
				let loc = this.trackedDoc.locations[idx].loc
				const uri = vscode.Uri.file(path.join(thisDirectoryPath, loc.filename))
				const pos = new vscode.Position(loc.line, loc.column)
				const range = new vscode.Range(pos, pos)
				vscode.window.showTextDocument(uri, {selection: range})
			},
			synchronizeEdits: (edits: readonly common.TrackUpdate[]) => {
				const doc = this.trackedDoc
				if (!doc)  return
				const edit = tracker.NormalizedEdits.fromArray(edits)

				const lastOnNext = this.#onNext
				const thisOnNext: NextEditCell = { next: null }
				this.#onNext = thisOnNext
				let doEdit = () => {
					this.#expectedEdits = edit
					for (let v of this.#activeWebviews) {
						if (v !== view)
							v.notifyDocumentEdited(edit.array)
					}
					const wsEdit = doc.mkWorkspaceEdit(edit)
					vscode.workspace.applyEdit(wsEdit).then((success) => {
						if (success) doc.updateRanges(edit)
						if (thisOnNext.next)
							thisOnNext.next()
						else
							this.#onNext = null

					}, (rsn) => {
						this.#logger("Edit failed" + rsn)
						if (thisOnNext.next)
							thisOnNext.next()
						else
							this.#onNext = null
					})
				}
				if (lastOnNext === null)
					doEdit()
				else
					lastOnNext.next = doEdit
			}
		}
        view = new ModelWebview(context, svc, webviewPanel.webview)
		this.#activeWebviews.add(view)

		// Make sure we get rid of the listener when our editor is closed.
		webviewPanel.onDidDispose(() => this.#activeWebviews.delete(view))

		const mdl = this.system
		if (mdl) view.setSystemModel(mdl)
	}
}
