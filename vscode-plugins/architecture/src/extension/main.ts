import * as path from 'path'
import * as vscode from 'vscode'
import { getNonce } from './util'
import * as fs from "fs"
import { webview, extension, common } from "../shared/webviewProtocol"
import * as parser from './parser'
import * as A from '../shared/architecture'
import * as tracker from './docUpdater'
import { disconnect } from 'process'

/**
 * Bring up a window to show the text document beside this window.
 */
function showDocument(parent: vscode.Uri, loc: webview.VisitURI) {
	const dir = path.dirname(parent.fsPath)
	const uri = vscode.Uri.file(path.join(dir, loc.filename))
	const pos = new vscode.Position(loc.line, loc.column)
	const range = new vscode.Range(pos, pos)
	vscode.window.showTextDocument(uri, {selection: range})
}

/**
 * This is a cell that holds what edit to apply next (if any)
 */
type NextEditCell = { next : null|(() => void)}

/**
 * Manages state corresponding to a particular URI
 */
class ModelResources {
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
		const uri = doc.uri
		const t = new tracker.TrackedDoc(uri)
		try {
			const res = parser.parseArchitectureFile(t, doc.getText())
			this.#dc.set(uri, res.errors.map(diagnosticFromError))
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

	expectedEdits: null|tracker.NormalizedEdits=null

	/**
	 * Called whenever vscode notifies us that a text document changed.
	 */
	public onDidChangeTextDocument(e: vscode.TextDocumentChangeEvent) {
		if (this.trackedDoc
			&& this.expectedEdits
			&& this.trackedDoc.allExpected(this.expectedEdits, e.contentChanges)) {

			this.expectedEdits = null
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
	public resolvePirateGraphViewer(webviewPanel: vscode.WebviewPanel): void {
		const uri = this.#uri

		let view:ModelWebview
		// Append new system model webview to active webviews.
		let onEdits = (edits: readonly common.ModifyString[]) => {
			const doc = this.trackedDoc
			if (!doc)  return
			const edit = tracker.NormalizedEdits.fromArray(edits)

			const lastOnNext = this.#onNext
			const thisOnNext: NextEditCell = { next: null }
			this.#onNext = thisOnNext
			let doEdit = () => {
				this.expectedEdits = edit
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
		view = new ModelWebview(webviewPanel.webview, uri, onEdits)
		this.#activeWebviews.add(view)

		// Make sure we get rid of the listener when our editor is closed.
		webviewPanel.onDidDispose(() => this.#activeWebviews.delete(view))

		const mdl = this.system
		if (mdl) view.setSystemModel(mdl)
	}
}

/**
 * Get the static html used for the editor webviews.
 */
function getHtmlForWebview(extensionPath:string, webview: vscode.Webview): string {
	// Local path to script and css for the webview
	const cssDiskPath    = path.join(extensionPath, 'webview-static', 'webview.css')
	const cssResource    = webview.asWebviewUri(vscode.Uri.file(cssDiskPath))
	const scriptDiskPath = path.join(extensionPath, 'out', 'webview', 'webview', 'webview.js')
	const scriptResource = webview.asWebviewUri(vscode.Uri.file(scriptDiskPath))
	const htmlBodyDiskPath    = path.join(extensionPath, 'webview-static', 'contents.html')
	const htmlBodyContents    = fs.readFileSync(htmlBodyDiskPath, 'utf8')
	// Use a nonce to whitelist which scripts can be run
	const nonce = getNonce()

	return /* html */`
		<!DOCTYPE html>
		<html lang="en">
		<head>
			<meta charset="UTF-8">
			<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource} 'unsafe-inline'; script-src 'nonce-${nonce}'; "/>
			<meta name="viewport" content="width=device-width, initial-scale=1.0">
			<title>PIRATE Project Viewer</title>
			<link nonce="${nonce}" href="${cssResource}" rel="stylesheet">
		</head>
		<body nonce="${nonce}">
		${htmlBodyContents}
		<script type="module" nonce="${nonce}" src="${scriptResource}" />
		</body>
		</html>`
}

/**
 * Manages state in the extension for a webview corresponding to a particular uri.
 */
class ModelWebview {
	/** URI for this model. */
	readonly #uri: vscode.Uri

	readonly #onEdits: (edits:readonly common.ModifyString[]) => void

	readonly #webview : vscode.Webview
	/**
	 * Number of `SetSystemLayout` requests outstanding.
	 *
	 * The message queue between extension and webview is assumed to be
	 * in order and reliable, but we do not make latency assumptions.
	 * Potentially the VSCode extension can update the model multiple
	 * times while
	 */
	#setSystemModelWaitCount: number = 0

	constructor(view: vscode.Webview, uri: vscode.Uri, onEdits: (edits:readonly common.ModifyString[]) => void) {
		this.#uri = uri
		this.#onEdits = onEdits
		this.#webview = view

		// Receive message from the webview.
		view.onDidReceiveMessage((e:webview.Event) => {
			switch (e.tag) {
			case webview.Tag.VisitURI:
				showDocument(this.#uri, e)
				break
			case webview.Tag.UpdateDocument:
				// Only apply update doc requests when we reject update doc requests if the system is waiting
				// for a system layout wait count request.
				if (this.#setSystemModelWaitCount === 0)
					this.#onEdits(e.changes)
				break
			case webview.Tag.SetSystemModelDone:
				this.#setSystemModelWaitCount--
				break
			}
		})
	}

	private postEvent(m : extension.Event) {
		this.#webview.postMessage(m)
	}

	public notifyDocumentEdited(edits: readonly common.ModifyString[]) {
		let msg:extension.DocumentEdited = { tag: extension.Tag.DocumentEdited, edits: edits }
		this.postEvent(msg)
	}

	/**
	 * Update the document associated with the given webview.
	 */
	public setSystemModel(s: A.SystemModel): void {
		this.#setSystemModelWaitCount++
		let msg:extension.SetSystemModel = { tag: extension.Tag.SetSystemModel, system: s }
		this.postEvent(msg)
	 }

}

// Language ID in vscode to identify pirate model files.
const piratemodelLangID:string = "piratemap"

/**
 * Create vscode diagnostic from parser error message.
 */
function diagnosticFromError(e : parser.Error):vscode.Diagnostic {
	const start = new vscode.Position(e.start.line, e.start.character)
	const end = new vscode.Position(e.end.line, e.end.character)
	const r : vscode.Range = new vscode.Range(start, end)
	return new vscode.Diagnostic(r, e.message)
}

/**
 * Manages global state for the Pirate plugin Provider for PIRATE project files.
 */
class PiratePlugin  {

	readonly #dc = vscode.languages.createDiagnosticCollection('PIRATE')

	// Map from URIs of open pirate system model text documents to tracker content.
	readonly #openModels: Map<string, ModelResources> = new Map()
	readonly #c: vscode.OutputChannel

	/** Things to dispose when plugin is deactivated */
	readonly #subscriptions: { dispose(): any }[] = []

	/**
	 * Initialize Pirate architecture extension
	 */
	constructor(private readonly context: vscode.ExtensionContext) {
		let subscribe = (d:{ dispose():any}) => this.#subscriptions.push(d)

		// Register our custom editor providers
		this.#c = vscode.window.createOutputChannel('pirate')
		subscribe(this.#c)

		// Listeners for text document open, close, change
		subscribe(vscode.workspace.onDidOpenTextDocument(this.onDidOpenTextDocument, this))
		subscribe(vscode.workspace.onDidCloseTextDocument(this.onDidCloseTextDocument, this))
		subscribe(vscode.workspace.onDidChangeTextDocument(this.onDidChangeTextDocument, this))

		// Tell vscode about out custom editor
		subscribe(vscode.window.registerCustomEditorProvider('pirate.graph', {
			resolveCustomTextEditor: (d,p,t) => this.resolvePirateGraphViewer(d,p,t)
		}))
	}

	/** Deactivate plugin */
	public dispose() {
		for(const d of this.#subscriptions)
			d.dispose()
	}

	private onDidOpenTextDocument(doc: vscode.TextDocument) {
		if (doc.languageId === piratemodelLangID) {
			let uri = doc.uri.toString()
			let mdlRes = new ModelResources(this.#dc, (m) => this.#c.appendLine(m), doc)
			this.#openModels.set(uri, mdlRes)
		}
	}

	private onDidCloseTextDocument(doc: vscode.TextDocument) {
		if (doc.languageId === piratemodelLangID)
			this.#openModels.delete(doc.uri.toString())
	}

	/**
	 * Called whenever vscode notifies us that a text document changed.
	 */
	private onDidChangeTextDocument(e: vscode.TextDocumentChangeEvent) {
		const doc = e.document
		if (doc.languageId !== piratemodelLangID) return
		if (e.contentChanges.length === 0) return

		const uri = doc.uri.toString()
		// Stop if existing model is defined and all changes are expected
		const model = this.#openModels.get(uri)
		if (model)
			model.onDidChangeTextDocument(e)
	}

	/**
	 * Called when pirate graph viewer is opened
	 */
	private resolvePirateGraphViewer(
		initialDocument: vscode.TextDocument,
		webviewPanel: vscode.WebviewPanel,
		_token: vscode.CancellationToken
	): void {
		const uri = initialDocument.uri
		const mdl = this.#openModels.get(uri.toString())
		if (mdl === undefined) {
			this.#c.appendLine("Could not find open model.")
			return
		}

		// Setup initial content for the webview
		webviewPanel.webview.options = {
			// Enable javascript
			enableScripts: true,
			// And restrict the webview to only loading content.
			localResourceRoots: [
				vscode.Uri.file(this.context.asAbsolutePath('webview-static')),
				vscode.Uri.file(this.context.asAbsolutePath(path.join('out', 'webview')))
			]
		}
		webviewPanel.webview.html = getHtmlForWebview(this.context.extensionPath, webviewPanel.webview)

		mdl.resolvePirateGraphViewer(webviewPanel)
	}
}

// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {
	context.subscriptions.push(new PiratePlugin(context))
}


// this method is called when your extension is deactivated
export function deactivate() {

}
