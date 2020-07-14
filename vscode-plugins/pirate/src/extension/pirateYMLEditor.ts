import * as path from 'path';
import * as vscode from 'vscode';
import { getNonce } from './util';
import * as fs from "fs";
import * as Model from "./model";
import * as R from "../shared/viewRequests";
import * as U from "../shared/modelUpdates";
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
			},
			'green');
		const gpsPort = new Model.OutPort(
			gps,
			"sensorData",
			{
				uri: 'file:///home/jhendrix/repos/gaps/vscode-plugin/pirate/examples/pnt/sensors.h',
			  	line: 30,
			  	column: 6
			},
			U.Border.Bottom,
			90
		);
		const target = new Model.Service(
			system,
			'Target',
			{
				uri: 'file:///home/jhendrix/repos/gaps/vscode-plugin/pirate/examples/pnt/sensors.h',
				line: 30,
				column: 6
			},
			{
				left: 50,
				top: 200,
				width: 200,
				height: 100,
			},
			'green');
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
			},
			'orange');
		const rfPort = new Model.OutPort(
			rfSensor,
			"sensor",
			{
				uri: 'file:///home/jhendrix/repos/gaps/vscode-plugin/pirate/examples/pnt/sensors.h',
			  	line: 30,
			  	column: 6
			},
			U.Border.Bottom,
			90
		);
		const uav = new Model.Service(
				system,
				'UAV',
				{
					uri: 'file:///home/jhendrix/repos/gaps/vscode-plugin/pirate/examples/pnt/sensors.h',
					line: 30,
					column: 6
				},
				{
					left: 300,
					top: 200,
					width: 200,
					height: 100,
				},
				'orange');

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
		// Local path to script and css for the webview
		const cssDiskPath    = path.join(this.context.extensionPath, 'webview-static', 'pirateYMLEditor.css');
		const cssResource    = webview.asWebviewUri(vscode.Uri.file(cssDiskPath));
		const scriptDiskPath = path.join(this.context.extensionPath, 'out', 'webview', 'webview', 'pirateYMLEditor.js');
		const scriptResource = webview.asWebviewUri(vscode.Uri.file(scriptDiskPath));
		const htmlBodyDiskPath    = path.join(this.context.extensionPath, 'webview-static', 'pirateYMLEditor.html');
		const htmlBodyContents    = fs.readFileSync(htmlBodyDiskPath, 'utf8');
		// Use a nonce to whitelist which scripts can be run
		const nonce = getNonce();

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
			<div id="lasterrormsg">No error message</div>
			${htmlBodyContents}
			<script type="module" nonce="${nonce}" src="${scriptResource}" />
			</body>
			</html>`;
	}
}
