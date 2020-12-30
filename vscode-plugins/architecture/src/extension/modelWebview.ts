import * as fs from 'fs'
import * as path from 'path'
import * as vscode from 'vscode'

import * as A from '../shared/architecture'
import { webview, extension, common } from '../shared/webviewProtocol'

import { getNonce } from './nonce'

/**
 * Provides services the model webview needs to process messages from
 * the webview.
 */
export interface ModelWebviewServices {
	/**
	 * Bring up a window to show the text document beside this window.
	 */
    showDocument(idx: A.LocationIndex):void

    /**
     * Synchronize edits received from this webview with rest of document.
     */
    synchronizeEdits(edits:readonly common.TrackUpdate[]):void
}

/**
 * Get the static html used for the editor webviews.
 */
function configureWebview(webview: vscode.Webview, extensionPath:string, htmlBodyContents: string): void {
	// Local path to script and css for the webview
	const cssDiskPath    = path.join(extensionPath, 'webview-static', 'webview.css')
	const cssResource    = webview.asWebviewUri(vscode.Uri.file(cssDiskPath))
	const scriptDiskPath = path.join(extensionPath, 'out', 'webview', 'webview', 'webview.js')
	const scriptResource = webview.asWebviewUri(vscode.Uri.file(scriptDiskPath))
	// Use a nonce to whitelist which scripts can be run
	const nonce = getNonce()

	webview.html = `
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
export class ModelWebview {
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

	constructor(context: vscode.ExtensionContext, svc:ModelWebviewServices, view: vscode.Webview) {
        // Setup initial content for the webview
        view.options = {
            // Enable javascript
            enableScripts: true,
            // And restrict the webview to only loading content.
            localResourceRoots: [
                vscode.Uri.file(context.asAbsolutePath('webview-static')),
                vscode.Uri.file(context.asAbsolutePath(path.join('out', 'webview')))
            ]
        }
        const htmlBodyDiskPath = path.join(context.extensionPath, 'webview-static', 'contents.html')
        const htmlBodyContents = fs.readFileSync(htmlBodyDiskPath, 'utf8')
        configureWebview(view, context.extensionPath, htmlBodyContents)

		this.#webview = view

		// Receive message from the webview.
		view.onDidReceiveMessage((e:webview.Event) => {
			switch (e.tag) {
			case webview.Tag.VisitURI:
				svc.showDocument(e.locationIdx)
				break
			case webview.Tag.UpdateDocument:
				// Only apply update doc requests when we reject update doc requests if the system is waiting
				// for a system layout wait count request.
				if (this.#setSystemModelWaitCount === 0)
					svc.synchronizeEdits(e.edits)
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

    /**
     * Tell the webview about edits made by other components.
     */
	public notifyDocumentEdited(edits: readonly common.TrackUpdate[]) {
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
