import * as path from 'path'
import * as vscode from 'vscode'
import { getNonce } from './util'
import * as fs from "fs"
import { webview, extension } from "../shared/webviewProtocol"
import { parseArchitectureFile } from './parser'
import { SystemLayout } from '../shared/architecture'
import * as tracker from './changeTracker'

/**
 * Bring up a window to show the text document beside this window.
 */
function showDocument(loc: webview.VisitURI) {
	let include: vscode.GlobPattern = loc.filename
	vscode.workspace.findFiles(include).then((uriList) => {
		if (uriList.length === 0) {
			vscode.window.showErrorMessage('Location not found')
		} else if (uriList.length > 1) {
			vscode.window.showErrorMessage('Ambiguous location')
		} else {
			const uri = uriList[0]
			const pos = new vscode.Position(loc.line, loc.column)
			const range = new vscode.Range(pos, pos)
			vscode.window.showTextDocument(uri, {selection: range})		
		}
	})
}

type TrackedSystem = { system?: SystemLayout, doc: tracker.TrackedDocument }

/**
 * Manages state in the extension for a webview corrwesponding to a particular uri.
 */
class WebviewInstance {
	#webview : vscode.Webview
	/**
	 * Number of `SetSystemLayout` requests outstanding.
	 * 
	 * The message queue between extension and webview is assumed to be
	 * in order and reliable, but we do not make latency assumptions.
	 * Potentially the VSCode extension can update the model multiple
	 * times while 
	 */
	#setSystemLayoutWaitCount: number = 0
	#doc : tracker.TrackedDocument

	constructor(view: vscode.Webview, doc: tracker.TrackedDocument, log:(msg:string)=>void) {
		this.#webview = view
		this.#doc = doc

		// Receive message from the webview.
		view.onDidReceiveMessage((e:webview.Event) => {
			switch (e.tag) {
			case webview.Tag.VisitURI:
				showDocument(e)
				break
			case webview.Tag.UpdateDoc:
				// Only apply update doc requests when we reject update doc requests if the system is waiting
				// for a system layout wait count request.
				if (this.#setSystemLayoutWaitCount === 0) {
					const changes = new tracker.ChangeSet()
					for (const c of e.changes) {
						let loc = new tracker.TrackedLocation(c.locationId)
						changes.replace(loc, c.newText)
					}
					this.#doc.apply(changes)
				}
				break
			case webview.Tag.SetSystemLayoutDone:
				this.#setSystemLayoutWaitCount--
				break
			}
		})

	}

	get docUpdating():boolean { return this.#doc?.hasPending ?? false }

	private postEvent(m : extension.Event) {
		this.#webview.postMessage(m)
	}

	/**
	 * Update the document associated with the given webview.
	 */
	public setSystemLayout(s: TrackedSystem): void {
		this.#setSystemLayoutWaitCount++
		this.#doc = s.doc
		if (s.system)
			this.postEvent({ tag: extension.Tag.SetSystemLayout, system: s.system })
	 }

}

/**
 * Provider for PIRATE project files.
 */
export class ProjectEditorProvider implements vscode.CustomTextEditorProvider {

	public static register(context: vscode.ExtensionContext): vscode.Disposable {
		const c = vscode.window.createOutputChannel('pirate')
		const provider = new ProjectEditorProvider(context, c)	
        const providerRegistration = vscode.window.registerCustomEditorProvider('pirate.graph', provider)
		return vscode.Disposable.from(providerRegistration, new vscode.Disposable(c.dispose))
	}

	#dc = vscode.languages.createDiagnosticCollection('PIRATE')

	constructor(private readonly context: vscode.ExtensionContext, 
		        private readonly c: vscode.OutputChannel) {

		vscode.workspace.onDidChangeTextDocument((e) => {
			const doc = e.document
			const uri = doc.uri
			const view = this.#activeWebviews.get(uri.toString())
			if (view && view.docUpdating) {
				c.appendLine('Ignoring pending change')
				return
			}
			let newLayout			
			if (view || doc.languageId === "piratemap")		
				newLayout = this.resolveSystemLayout(doc)
			if (view && newLayout) 
				view.setSystemLayout(newLayout)
		})
	}

	#activeWebviews: Map <string, WebviewInstance> = new Map()

	/**
	 * Resolve a system layout by parsing the document.
	 */
	private resolveSystemLayout(doc:vscode.TextDocument):TrackedSystem{
		const uri = doc.uri
		const bytes = doc.getText()
		try {
			const res = parseArchitectureFile(uri, bytes, {tabstopWidth: 8}, this.c.appendLine)
			let diagnostics : vscode.Diagnostic[] = []
			for (const e of res.errors) {
				const start = new vscode.Position(e.start.line, e.start.column)
				const end = new vscode.Position(e.end.line, e.end.column)
				const r : vscode.Range = new vscode.Range(start, end)
				const d = new vscode.Diagnostic(r, e.message)
				diagnostics.push(d)
			}
			this.#dc.set(uri, diagnostics)
			return { system: res.value, doc: res.doc }
		} catch (e) {			
			this.c.appendLine('Exception thrown: ' + e)
			return { system: undefined, doc: new tracker.TrackedDocument(uri, this.c.appendLine) }
		}
	}

	/**
	 * Called when our custom editor is opened.
	 */
	public async resolveCustomTextEditor(
		initialDocument: vscode.TextDocument,
		webviewPanel: vscode.WebviewPanel,
		_token: vscode.CancellationToken
	): Promise<void> {
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

		webviewPanel.webview.html = this.getHtmlForWebview(webviewPanel.webview)

		const uri = initialDocument.uri

		const l = this.resolveSystemLayout(initialDocument)

		const view = new WebviewInstance(webviewPanel.webview, l.doc, this.c.appendLine)

		// Make sure we get rid of the listener when our editor is closed.
		webviewPanel.onDidDispose(() => {
			this.#activeWebviews.delete(uri.toString())
		})

		this.#activeWebviews.set(uri.toString(), view)
		if (l) view.setSystemLayout(l)
	}

	/**
	 * Get the static html used for the editor webviews.
	 */
	private getHtmlForWebview(webview: vscode.Webview): string {
		// Local path to script and css for the webview
		const cssDiskPath    = path.join(this.context.extensionPath, 'webview-static', 'webview.css')
		const cssResource    = webview.asWebviewUri(vscode.Uri.file(cssDiskPath))
		const scriptDiskPath = path.join(this.context.extensionPath, 'out', 'webview', 'webview', 'webview.js')
		const scriptResource = webview.asWebviewUri(vscode.Uri.file(scriptDiskPath))
		const htmlBodyDiskPath    = path.join(this.context.extensionPath, 'webview-static', 'contents.html')
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
}
