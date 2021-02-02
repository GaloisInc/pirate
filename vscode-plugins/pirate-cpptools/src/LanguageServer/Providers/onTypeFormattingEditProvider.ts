/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All Rights Reserved.
 * See 'LICENSE' in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
import * as vscode from 'vscode';
import {DefaultClient,  FormatParams, FormatOnTypeRequest, cachedEditorConfigSettings} from '../client';
import { CppSettings } from '../settings';
import * as editorConfig from 'editorconfig';

export class OnTypeFormattingEditProvider implements vscode.OnTypeFormattingEditProvider {
    private client: DefaultClient;
    constructor(client: DefaultClient) {
        this.client = client;
    }

    public provideOnTypeFormattingEdits(document: vscode.TextDocument, position: vscode.Position, ch: string, options: vscode.FormattingOptions, token: vscode.CancellationToken): Promise<vscode.TextEdit[]> {
        return new Promise<vscode.TextEdit[]>((resolve, reject) => {
            this.client.notifyWhenReady(() => {
                const filePath: string = document.uri.fsPath;
                const configCallBack = (editorConfigSettings: any | undefined) => {
                    const params: FormatParams = {
                        settings: { ...editorConfigSettings },
                        uri: document.uri.toString(),
                        insertSpaces: options.insertSpaces,
                        tabSize: options.tabSize,
                        character: ch,
                        range: {
                            start: {
                                character: position.character,
                                line: position.line
                            },
                            end: {
                                character: 0,
                                line: 0
                            }
                        }
                    };
                    return this.client.languageClient.sendRequest(FormatOnTypeRequest, params)
                        .then((textEdits) => {
                            const result: vscode.TextEdit[] = [];
                            textEdits.forEach((textEdit) => {
                                result.push({
                                    range: new vscode.Range(textEdit.range.start.line, textEdit.range.start.character, textEdit.range.end.line, textEdit.range.end.character),
                                    newText: textEdit.newText
                                });
                            });
                            resolve(result);
                        });
                };
                const settings: CppSettings = new CppSettings();
                if (settings.formattingEngine !== "vcFormat") {
                    // If not using vcFormat, only process on-type requests for ';'
                    if (ch !== ';') {
                        const result: vscode.TextEdit[] = [];
                        resolve(result);
                    } else {
                        configCallBack(undefined);
                    }
                } else {
                    const editorConfigSettings: any = cachedEditorConfigSettings.get(filePath);
                    if (!editorConfigSettings) {
                        editorConfig.parse(filePath).then(configCallBack);
                    } else {
                        cachedEditorConfigSettings.set(filePath, editorConfigSettings);
                        configCallBack(editorConfigSettings);
                    }
                }
            });
        });
    }
}
