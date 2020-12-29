import { Agent } from 'http'
import * as vscode from 'vscode'

/** A location that is tracked. */
export class TrackedLocation {
    constructor(readonly index: number) {}
}

/** A set of changes to a text document. */
export class ChangeSet {
    /**
     * This is a mapping from source location indices to new
     * values.
     */
    #changes: Map<number, string> = new Map()

    /** Replace text at the given location with newValue */
    replace(location: TrackedLocation, newValue:string):void {
        this.#changes.set(location.index, newValue)
    }

    /*** Return true if change set is empty */
    get empty() { return this.#changes.size === 0 }

    /** Iterate through changes in file with changes at start first. */
    foreach(p: (index: number, newValue:string) => void) : void { 
        this.#changes.forEach((value, key, m) => p(key, value))
    }

    /** Use the newer change set to augment and replace changes in this one.  */
    merge(newer : ChangeSet):void {
        newer.#changes.forEach((value, key, _) => this.#changes.set(key, value))
    }
}

type Edit = {idx: number, newText: string}

/** Flag to control if we are adding new lines or inserting into a line */
const enum AdjustMode { InsertInLine, AddLines }

interface AddLines {
    mode: AdjustMode.AddLines
    lineDelta: number
    nextColumnOffset: number
}

interface InsertInLine {
    mode: AdjustMode.InsertInLine
    columnDelta: number
}

type ActiveAdjustment = null | AddLines | InsertInLine

/**
 * Return number of lines in string.
 * 
 * This treats carriage return and new lines as line terminators,
 * and treats DOS newlines (\r\n) as a single newline.
 */
function lineSourceDelta(x:string):{line: number, column: number } {
    let lines = x.match(/^.*(\r\n)?/gm)
    // The match should not ever fail, but we just return a reasonable default.
    if (!lines) return {line: 1, column: x.length }

    let n = lines.length
    return {line: n, column: lines[n-1].length }
}

function modifyPosition(p: vscode.Position, lineDelta: number, columnDelta?: number) {
    return new vscode.Position(p.line + lineDelta, p.character + (columnDelta ? columnDelta : 0))
}

function modifyRange(r: vscode.Range, lineDelta: number, columnDelta?: number):vscode.Range {
    return new vscode.Range(modifyPosition(r.start, lineDelta, columnDelta),
                            modifyPosition(r.end, lineDelta, columnDelta))
}

/** A set of locations that can be updated */
export class TrackedDocument {
    // URI of document
    readonly #uri : vscode.Uri
    // List of locations
    readonly #m : vscode.Range[] = []
    // Position of last added location (or undefined if no location added)
    #lastEnd : vscode.Position|undefined = undefined
    // Edits we are waiting to hear back form
    #waitingEdits: Edit[]|null = null
    // Set of changes we need to run after waiting edits (or null for none)
    #suspendedChanges : ChangeSet | null = null

    constructor(uri: vscode.Uri, readonly log:(msg:string) => void) { this.#uri = uri }

    /** Record a location to be marked.  */
    markLocation(startLine: number, startColumn: number, endLine: number, endColumn: number):TrackedLocation|undefined {
        const start = new vscode.Position(startLine, startColumn)
        const end = new vscode.Position(endLine, endColumn)
        // Fail if end posiition is not at or before the start
        if (start.isAfter(end)) return undefined

        // Fail if last end position was not at or before the start of this one.
        if (this.#lastEnd && this.#lastEnd.isAfter(start))
            return undefined

        const idx = this.#m.length
        this.#m.push(new vscode.Range(start, end))
        this.#lastEnd = end
        return new TrackedLocation(idx)
    }

    get hasPending():boolean { return this.#waitingEdits !== null }

    /**
     * Asynchronously apply changes to document.
     * 
     * Note.  This may fail, and no notice is given.
     * 
     * We currently assume that if apply fails, this indicates
     * that a new document will be available.
     */
    apply(changes: ChangeSet):void {
        if (changes.empty) return

        // If we are waiting for changes to execute now, then
        // throw these changes in pending.
        if (this.#suspendedChanges) {
            this.#suspendedChanges.merge(changes)
            return
        } else if (this.#waitingEdits) {
            this.#suspendedChanges = new ChangeSet()
            this.#suspendedChanges.merge(changes)
            return
        }
        const uri = this.#uri
        const edit = new vscode.WorkspaceEdit()
        let edits : Edit[] = []
        changes.foreach((idx, newText) => {
            let loc = this.#m[idx]
            let startColumn = loc.start.character
            let endColumn = loc.end.character
            let msg = "Start " + startColumn.toString() + ", End column " + endColumn.toString() + ", value =" + newText
            edit.replace(uri, this.#m[idx], newText)
            edits.push({idx: idx, newText: newText})
        })
        this.#waitingEdits = edits
        vscode.workspace.applyEdit(edit).then((s) => this.applyFulfilled(s), (m) => this.applyRejected(m))
    }

    private applySucceeded(edits:Edit[]) {
        // Delta to modify lines by
        let lineDelta:number = 0
        // Line that previous edit moved columns for that may still need to b
        // considered
        let adjustedLine: number = 0
        // Number of characters to adjust column in adjusted line by
        let columnDelta: number = 0
        for (let editIndex = 0; editIndex < edits.length; ) {            
            let e = edits[editIndex++]
            let locIdx = e.idx
            let newTextDelta = lineSourceDelta(e.newText)

            let editLoc = this.#m[locIdx]
            let editStart = editLoc.start
            let editEnd = editLoc.end

            // Compute new start position
            const newStartCol
                = editStart.line === adjustedLine
                ? editStart.character + columnDelta
                : editStart.character
            const newStart = new vscode.Position(editStart.line + lineDelta, newStartCol)
            // Compute new end base on whether we have newlines or not
            const newEnd
                = newTextDelta.line === 1
                ? new vscode.Position(newStart.line, newStart.character + newTextDelta.column)
                : new vscode.Position(newStart.line + newTextDelta.line, newTextDelta.column)
            this.#m[locIdx] = new vscode.Range(newStart, newEnd)
            // Adjust line delta by number of lines deleted in this edit.
            lineDelta = newEnd.line - editEnd.line
            // Line to adjust columns for
            adjustedLine = editEnd.line
            // Number of characters to adjust column by
            columnDelta = newEnd.character - editEnd.character
            // Handle case where neither old text or  new text have new lines
            // Get loc index of next edit (or length of locs if this is the last endit.)
            const nextEditLocIndex = editIndex < edits.length ? edits[editIndex].idx : this.#m.length
            // While we aren't updating a modified location and the location is on the line of 
            // this edit.
            let idx = locIdx+1
            while (idx < nextEditLocIndex && this.#m[idx].start.line === adjustedLine) {
                this.#m[idx] = modifyRange(this.#m[idx], lineDelta, columnDelta)
                ++idx
            }
            if (lineDelta !== 0) {
                while (idx < nextEditLocIndex) {
                    this.#m[idx] = modifyRange(this.#m[idx], lineDelta, 0)
                    ++idx
                }
            }
        }
    }

    private applyFulfilled(success:boolean):void {
        // If changes failed, then clear pending and leave document alone.
        const edits = this.#waitingEdits
        // This should always be defined.
        if (edits !== null) {
            if (success) this.applySucceeded(edits)
            this.#waitingEdits = null
        }
        let changes = this.#suspendedChanges
        if (changes) {
            this.#suspendedChanges = null
            this.apply(changes)
        }
    }

    private applyRejected(msg:any) {
        this.#waitingEdits = null
        let changes = this.#suspendedChanges
        if (changes) {
            this.#suspendedChanges = null
            this.apply(changes)
        }
    }
}
