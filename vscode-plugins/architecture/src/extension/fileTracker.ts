import * as vscode from 'vscode'
import * as tracker from './docUpdater'
import { TextPosition, TextRange } from './position'

// Disposables controlled by sta
let activationCount: number=0
// Disposables that should be valid iff activationCount > 0
let onDidDeleteFiles: vscode.Disposable|null=null
let onDidRenameFiles: vscode.Disposable|null=null

function activate() {
    activationCount++
    if (activationCount > 1) return

    onDidDeleteFiles = vscode.workspace.onDidDeleteFiles((e) => {
        for (let uri of e.files) {
        }
    })

    onDidRenameFiles = vscode.workspace.onDidRenameFiles((e) => {
        for (let r of e.files) {
        }
    })
}

function deactivate() {
    --activationCount
    if (activationCount === 0) {
        onDidDeleteFiles?.dispose()
        onDidDeleteFiles = null
        onDidRenameFiles?.dispose()
        onDidRenameFiles = null
    }
}

/** Interface for handling file changes. */
export interface FileChangeHandler {
    readonly onDelete: () => void
    readonly onRename: (newUri:vscode.Uri) => void
}

export type FileIndex = number

export class FileSet {
    #fileMap:Map<FileIndex, vscode.Uri> = new Map()

    constructor(readonly doc:tracker.TrackedDoc) {
        activate()
    }

    /** Call when files in set are no longer of interest. */
    dispose() {
        deactivate()
    }

    /**
     * Mark the given range as something to track and provide the initial path.
     */
    track(r:TextRange, uri:vscode.Uri):FileIndex|undefined {
        let idx = this.doc.track(r)
        if (idx === undefined) return undefined
        this.#fileMap.set(idx, uri)
        return idx
    }

    /**
     * Return the current URI of the file.
     */
    currentURI(idx:FileIndex):vscode.Uri|undefined {
        return this.#fileMap.get(idx)
    }
}

/**
 * This tracks file locations and allows
 */
export class FileTracker {
}