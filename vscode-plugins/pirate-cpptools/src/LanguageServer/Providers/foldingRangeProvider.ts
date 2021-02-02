/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All Rights Reserved.
 * See 'LICENSE' in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
import * as vscode from 'vscode';
import { DefaultClient, GetFoldingRangesParams, GetFoldingRangesRequest, FoldingRangeKind } from '../client';

export class FoldingRangeProvider implements vscode.FoldingRangeProvider {
    private client: DefaultClient;
    public onDidChangeFoldingRangesEvent = new vscode.EventEmitter<void>();
    public onDidChangeFoldingRanges?: vscode.Event<void>;
    constructor(client: DefaultClient) {
        this.client = client;
        this.onDidChangeFoldingRanges = this.onDidChangeFoldingRangesEvent.event;
    }
    provideFoldingRanges(document: vscode.TextDocument, context: vscode.FoldingContext,
        token: vscode.CancellationToken): Promise<vscode.FoldingRange[]> {
        const id: number = ++DefaultClient.abortRequestId;
        const params: GetFoldingRangesParams = {
            id: id,
            uri: document.uri.toString()
        };
        return new Promise<vscode.FoldingRange[]>((resolve, reject) => {
            this.client.notifyWhenReady(() => {
                this.client.languageClient.sendRequest(GetFoldingRangesRequest, params)
                    .then((ranges) => {
                        if (ranges.canceled) {
                            reject();
                        } else {
                            const result: vscode.FoldingRange[] = [];
                            ranges.ranges.forEach((r) => {
                                const foldingRange: vscode.FoldingRange = {
                                    start: r.range.start.line,
                                    end: r.range.end.line
                                };
                                switch (r.kind) {
                                    case FoldingRangeKind.Comment:
                                        foldingRange.kind = vscode.FoldingRangeKind.Comment;
                                        break;
                                    case FoldingRangeKind.Imports:
                                        foldingRange.kind = vscode.FoldingRangeKind.Imports;
                                        break;
                                    case FoldingRangeKind.Region:
                                        foldingRange.kind = vscode.FoldingRangeKind.Region;
                                        break;
                                    default:
                                        break;
                                }
                                result.push(foldingRange);
                            });
                            resolve(result);
                        }
                    });
                token.onCancellationRequested(e => this.client.abortRequest(id));
            });
        });
    }

    public refresh(): void {
        this.onDidChangeFoldingRangesEvent.fire();
    }
}
