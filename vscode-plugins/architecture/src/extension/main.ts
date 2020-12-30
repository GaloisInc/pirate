import * as vscode from 'vscode'

import { ModelResources } from './modelResources'

// Language ID in vscode to identify pirate model files.
const piratemodelLangID:string = "piratemap"

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

		// Enable and register custom editor
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
			let mdlRes = new ModelResources(this.#dc, (msg:string) => this.#c.appendLine(msg), doc)
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

		mdl.resolvePirateGraphViewer(this.context, webviewPanel)
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
