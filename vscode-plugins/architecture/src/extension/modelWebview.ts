/**
 * Defines the ModelWebview class capturing the state of a webview instance for
 * a given URI, as well as the interface for their communication with the
 * extension.
 */

import * as fs from 'fs'
import * as path from 'path'

import * as vscode from 'vscode'

import * as A from '../shared/architecture'
import { webview, extension, common } from '../shared/webviewProtocol'

/**
 * Provides services the model webview needs to process messages from
 * the webview.
 */
export interface ModelWebviewServices {
    /**
     * Bring up a window to show the text document beside this window.
     */
    showDocument(idx: A.LocationIndex): void

    /**
     * Synchronize edits received from this webview with rest of document.
     */
    synchronizeEdits(view: ModelWebview, edits: readonly common.TrackUpdate[]): void

    /**
     * Log a message for debugging purposes.
     */
    debugLog(message: string): void
}

/**
 * Get the static html used for the editor webviews.
 */
function getHtmlContents(staticPath: string): string {
    const htmlBodyDiskPath = path.join(staticPath, 'contents.html')
    return fs.readFileSync(htmlBodyDiskPath, 'utf8')
}

/**
 * Manages state in the extension for a webview corresponding to a particular uri.
 */
export class ModelWebview {
    readonly #webview: vscode.Webview

    /**
     * Indicates the ready command has been received from a newly created
     * webview.  Prior to this we do not know if messages will be received.
     */
    #ready = false

    /**
     * Number of `SetSystemLayout` requests outstanding.
     *
     * The message queue between extension and webview is assumed to be
     * in order and reliable, but we do not make latency assumptions.
     * Potentially the VSCode extension can update the model multiple
     * times while
     */
    #setSystemModelWaitCount = 0

    private model: A.SystemModel | null = null

    readonly #disposables: { dispose: () => void }[] = []

    constructor(context: vscode.ExtensionContext, svc: ModelWebviewServices, webviewPanel: vscode.WebviewPanel, mdl: A.SystemModel | null) {
        const view = webviewPanel.webview
        this.#webview = view

        const staticPath = context.asAbsolutePath('webview-static')
        const scriptPath = context.asAbsolutePath('out')

        // Setup initial content for the webview
        view.options = {
            // Enable javascript
            enableScripts: true,
            // And restrict the webview to only loading content.
            localResourceRoots: [
                vscode.Uri.file(staticPath),
                vscode.Uri.file(scriptPath),
            ],
        }
        // Local path to script and css for the webview
        const cssWebviewUri = view.asWebviewUri(vscode.Uri.file(path.join(staticPath, 'webview.css')))
        // const scriptWebviewUri = view.asWebviewUri(vscode.Uri.file(path.join(scriptPath, 'webview', 'webview.js')))
        const scriptWebviewUri = view.asWebviewUri(vscode.Uri.file(path.join(scriptPath, 'webview.bundle.js')))
        const webviewProto = view.cspSource
        view.html = `
            <!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8">
                <meta http-equiv="Content-Security-Policy" content="default-src ${webviewProto}; style-src ${webviewProto} 'unsafe-inline'"/>
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>PIRATE Project Viewer</title>
                <link href="${cssWebviewUri}" rel="stylesheet">
            </head>
            <body>
            ${getHtmlContents(staticPath)}
            <script type='module' src="${scriptWebviewUri}" />
            </body>
            </html>`

        let visible = webviewPanel.visible

        this.#disposables.push(webviewPanel.onDidChangeViewState((e) => {
            if (!visible && e.webviewPanel.visible && this.model) { this.setModel(this.model) }
            visible = e.webviewPanel.visible
        }))

        this.model = mdl
        // Receive message from the webview.
        this.#disposables.push(view.onDidReceiveMessage((e: webview.Event) => {
            switch (e.tag) {
                case webview.Tag.Ready:
                    this.#ready = true
                    if (this.model) { this.setModel(this.model) }
                    break
                case webview.Tag.VisitURI:
                    if (this.#setSystemModelWaitCount === 0) {
                        svc.showDocument(e.locationIdx)
                    }
                    break
                case webview.Tag.UpdateDocument:
                    // Only apply update doc requests when we reject update doc
                    // requests if the system is waiting for a system layout wait
                    // count request.
                    if (this.#setSystemModelWaitCount === 0) {
                        svc.synchronizeEdits(this, e.edits)
                    }
                    break
                case webview.Tag.SetSystemModelDone:
                    this.#setSystemModelWaitCount--
                    break
            }
        }))
    }

    dispose(): void {
        this.#disposables.forEach((d) => d.dispose)
    }

    private postEvent(m: extension.Event) {
        this.#webview.postMessage(m)
    }

    /**
     * Update the document associated with the given webview.
     */
    public setModel(s: A.SystemModel): void {
        this.model = s
        if (this.#ready) {
            this.#setSystemModelWaitCount++
            const msg: extension.SetSystemModel = { tag: extension.Tag.SetSystemModel, system: s }
            this.postEvent(msg)
        }
    }

    /**
     * Tell the webview about edits made by other components.
     */
    public notifyDocumentEdited(edits: readonly common.TrackUpdate[]): void {
        // If not yet ready then we can ignore edits.
        if (this.#ready) {
            const msg: extension.DocumentEdited = { tag: extension.Tag.DocumentEdited, edits: edits }
            this.postEvent(msg)
        }
    }

}
