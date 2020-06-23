import * as path from 'path';
import * as vscode from 'vscode';
import { getNonce } from './util';
import * as fs from "fs";
import * as Model from "./model";
import * as R from "../shared/viewRequests";

/**
 * Bring up a window to show the text document beside this window.
 */
function showDocument(loc: Model.FileLocation) {
	const uri = vscode.Uri.parse(loc.uri, false);
	if (uri) {
		const pos = new vscode.Position(loc.line, loc.column);
		const range = new vscode.Range(pos, pos);
		vscode.window.showTextDocument(uri, {viewColumn: vscode.ViewColumn.Beside, selection: range});
	}
}

/**
 * Provider for PIRATE project files.
 */
export class ProjectEditorProvider implements vscode.CustomTextEditorProvider {

	public static register(context: vscode.ExtensionContext): vscode.Disposable {
		const provider = new ProjectEditorProvider(context);
        const providerRegistration = vscode.window.registerCustomEditorProvider('pirate.graph', provider);
		return providerRegistration;
	}

	constructor(private readonly context: vscode.ExtensionContext) {

	}

	/*



	public newChannel(webview: vscode.Webview, s: ModelChannel):ModelChannel {
		let cmd:NewChannel = {
			tag: WebviewCommandTag.NewChannel,
			source: { serviceName: s., portName: "" },
			target: { serviceName: "", portName: "" },
		};
		webview.postMessage(cmd);
		return s;
	}
	*/

	/**
	 * Called when our custom editor is opened.
	 */
	public async resolveCustomTextEditor(
		document: vscode.TextDocument,
		webviewPanel: vscode.WebviewPanel,
		_token: vscode.CancellationToken
	): Promise<void> {
		// Setup initial content for the webview
		webviewPanel.webview.options = {
			// Enable javascript
			enableScripts: true,
			// And restrict the webview to only loading content.
			localResourceRoots: [
				vscode.Uri.file(path.join(this.context.extensionPath, 'webview-static')),
				vscode.Uri.file(path.join(this.context.extensionPath, 'out', 'webview', 'webview'))
			]
		};

		webviewPanel.webview.html = this.getHtmlForWebview(webviewPanel.webview);

		const system = new Model.Configuration(e => webviewPanel.webview.postMessage(e));

		const gps = new Model.Service(
			system,
			'GPS',
			{
				uri: 'file:///home/jhendrix/repos/gaps/vscode-plugin/pirate/examples/pnt/sensors.h',
			  	line: 30,
			  	column: 6
			},
			{
				left: 50,
				top: 50,
				width: 200,
				height: 100,
			});

		const rfSensor = new Model.Service(
			system,
			'RFSensor',
			{
				uri: 'file:///home/jhendrix/repos/gaps/vscode-plugin/pirate/examples/pnt/sensors.h',
				line: 64,
				column: 6
			},
			{
				left:   300,
				top:     50,
				width:  200,
				height: 100
			});

//		this.newChannel(webviewPanel.webview, {source:gps, target: rfSensor});

		// Receive message from the webview.
		webviewPanel.webview.onDidReceiveMessage((e:R.ViewRequest) => {
			switch (e.tag) {
			case R.Tag.VisitServiceClass:
				const cmd = e as R.VisitServiceClass;
				// Get service associated with name, note this theoretically may
				// be deleted by the time we receive the request, we ignore
				// requests to show undefined services silently.
				const s = system.findService(cmd.name);
				if (s) {
					showDocument(s.classDefinition);
				}
				break;
			default:
				console.log('Unknown request from view: ' + e.tag);
				break;
			}
		});

		const changeDocumentSubscription = vscode.workspace.onDidChangeTextDocument(e => {
			if (e.document.uri.toString() === document.uri.toString()) {
				//updateWebview();
			}
		});

		// Make sure we get rid of the listener when our editor is closed.
		webviewPanel.onDidDispose(() => {
			changeDocumentSubscription.dispose();
		});
	}

	/**
	 * Get the static html used for the editor webviews.
	 */
	private getHtmlForWebview(webview: vscode.Webview): string {
		const mediaPath = path.join(this.context.extensionPath, 'media');
		// Local path to script and css for the webview
		const cssContents    = fs.readFileSync(path.join(this.context.extensionPath, 'webview-static', 'pirateYMLEditor.css'), 'utf8');
		const scriptDiskPath = path.join(this.context.extensionPath, 'out', 'webview', 'webview',      'pirateYMLEditor.js');
//		const scriptResource = "vscode-resource:" + scriptDiskPath;
		const scriptResource = webview.asWebviewUri(vscode.Uri.file(scriptDiskPath));
		// Use a nonce to whitelist which scripts can be run
		//const nonce = getNonce();

		return /* html */`
			<!DOCTYPE html>
			<html lang="en">
			<head>
				<meta charset="UTF-8">
				<meta name="viewport" content="width=device-width, initial-scale=1.0">
				<style>${cssContents}</style>
				<title>PIRATE Project Viewer</title>
			</head>
			<body>
			<div id="lasterrormsg">No error message</div>
			<svg width=600 height=600 id="panel">
				<!-- arrowhead marker definition -->
				<marker id="arrowhead" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
		   			<path d="M 0 0 L 10 5 L 0 10 z" />
				</marker>
			</svg>
			<script src="${scriptResource}" />
			</body>
			</html>`;
/*
			<script>
			${scriptContents}
			</script>
*/

	}
}
