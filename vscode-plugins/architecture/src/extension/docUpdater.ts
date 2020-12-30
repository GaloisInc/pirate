import * as vscode from 'vscode'

import * as A     from "../shared/architecture"
import { common } from "../shared/webviewProtocol"

import { Tracker, SourceLocation } from "./parser"
import { TextRange } from "./position"

type DocEdit = common.TrackUpdate

/**
 * Return number of lines in string and number of characters in last line.
 *
 * This treats carriage return and new lines as line terminators,
 * and treats DOS newlines (\r\n) as a single newline.
 */
function lineSourceDelta(x: string): { line: number, lastCharCount: number } {
    let lines = x.match(/^.*(\r\n)?/gm)
    // The match should not ever fail, but we just return a reasonable default.
    if (!lines) return { line: 1, lastCharCount: x.length }

    let n = lines.length
    return { line: n, lastCharCount: lines[n - 1].length }
}

/**
 * This returns a position obtained by applying a line and column delt to an existing position.
 */
function modifyPosition(p: vscode.Position, lineDelta: number, columnDelta: number) {
    return new vscode.Position(p.line + lineDelta, p.character + columnDelta)
}

function modifyRange(r: vscode.Range, lineDelta: number, columnDelta: number): vscode.Range {
    return new vscode.Range(modifyPosition(r.start, lineDelta, columnDelta),
                            modifyPosition(r.end, lineDelta, columnDelta))
}

class Edit implements DocEdit {
    constructor(e: DocEdit) {
        this.trackIndex = e.trackIndex
        this.newText = e.newText
    }
    readonly trackIndex: number
    readonly newText: string
}

/** Compare track indices of doc edit */
function compareTrackIndex(x: Edit, y: Edit) {
    return x.trackIndex - y.trackIndex
}

// Type synonym intended for arrays that are sorted by
// trackIndex and have no duplicate track index assignments.
export class NormalizedEdits {
    readonly array:readonly Edit[]

    private constructor(edits: readonly Edit[]) {
        this.array = edits
    }

    static fromArray(edits: readonly DocEdit[]):NormalizedEdits {
        let sorted = edits.map((e) => new Edit(e)).sort(compareTrackIndex)
        let r: Edit[] = []
        if (edits.length > 0) {
            for (let i = 0; i + 1 !== sorted.length; ++i) {
                if (sorted[i].trackIndex < sorted[i + 1].trackIndex)
                    r.push(sorted[i])
            }
            r.push(sorted[sorted.length - 1])
        }
        return new NormalizedEdits(r)
    }

    merge(newer: NormalizedEdits): NormalizedEdits {
        const oldEdits = this.array
        const newEdits = newer.array
        if (oldEdits.length === 0) return newer
        if (newEdits.length === 0) return this

        let oldIdx = 0
        let newIdx = 0
        let res: Edit[] = []

        while (oldIdx < oldEdits.length && newIdx < newEdits.length) {
            const o = oldEdits[oldIdx]
            const n = newEdits[newIdx]
            // Old tracks are only preserved when not overwritten.
            if (o.trackIndex < n.trackIndex) {
                res.push(o)
                ++oldIdx
            } else {
                res.push(n)
                ++newIdx
            }
        }
        while (oldIdx < oldEdits.length)
            res.push(oldEdits[oldIdx++])
        while (newIdx < newEdits.length)
            res.push(newEdits[newIdx++])
        return new NormalizedEdits(res)
    }
}

export interface LocationInfo {
    trackIdx: A.TrackIndex
    loc: SourceLocation
}
/**
 * A tracked document maintains a set of locations in a VSCode document
 * at a specific URI.
 *
 * To use it, one first should call `track` to define each location to be
 * tracked.  Calls to One can generate a list of updates
 * to perform, and the tracker will apply them asynchronously to the undering
 * VScode URI.
 *
 * One can then call `apply` to indicate changes.  As apply changes the text
 * document, we can check whether the changes are expected in an event handler
 * with a call to `expected`.
 */
export class TrackedDoc implements Tracker {
    // URI f file.
    readonly #uri: vscode.Uri
    // List of tracked ranges in file
    readonly #ranges: vscode.Range[] = []

    readonly #locations: LocationInfo[] = []

    constructor(uri: vscode.Uri) { this.#uri = uri }

    /**
     * Start tracking a location and return tracker index or undefined if an
     * error occured.
     *
     * Locations should be tracked in order of appearance of the document so
     * that we can ensure lower tracked indices come before higher ones.
     */
    track(r: TextRange): A.TrackIndex|undefined {
        const start = new vscode.Position(r.start.line, r.start.character)
        const end   = new vscode.Position(r.end.line, r.end.character)
        // Fail if end posiition is not at or before the start
        if (start.isAfter(end)) return undefined

        const m = this.#ranges
        const lastEnd = m.length > 0 ? m[m.length-1].end : undefined

        // Fail if last end position was not at or before the start of this one.
        if (lastEnd && lastEnd.isAfter(start))
            return undefined

        m.push(new vscode.Range(start, end))
        return m.length - 1
    }

    get locations(): readonly LocationInfo[] { return this.#locations}

    location(r: TextRange, loc:SourceLocation): A.LocationIndex|undefined {
        const trackIdx = this.track(r)
        if (trackIdx === undefined) return undefined
        this.#locations.push({trackIdx: trackIdx, loc: loc})
        return this.#locations.length - 1
    }

    /**
     * Use edits to update the file ranges associated with locations.
     */
    public updateRanges(edits: NormalizedEdits) {
        // Delta to modify lines by
        let lineDelta: number = 0
        // Line that previous edit moved columns for that may still need to b
        // considered
        let adjustedLine: number = 0
        // Number of characters to adjust column in adjusted line by
        let columnDelta: number = 0
        let editArray = edits.array
        for (let editIndex = 0; editIndex < editArray.length; ++editIndex) {
            let e = edits.array[editIndex]
            let trackIndex = e.trackIndex
            let newTextDelta = lineSourceDelta(e.newText)

            let editLoc = this.#ranges[trackIndex]
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
                ? new vscode.Position(newStart.line, newStart.character + newTextDelta.lastCharCount)
                : new vscode.Position(newStart.line + newTextDelta.line, newTextDelta.lastCharCount)
            this.#ranges[trackIndex] = new vscode.Range(newStart, newEnd)
            // Adjust line delta by number of lines deleted in this edit.
            lineDelta = newEnd.line - editEnd.line
            // Line to adjust columns for
            adjustedLine = editEnd.line
            // Number of characters to adjust column by
            columnDelta = newEnd.character - editEnd.character
            // Handle case where neither old text or  new text have new lines
            // Get loc index of next edit (or length of locs if this is the last endit.)
            const nextTrackIndex
                = editIndex+1 < editArray.length
                ? editArray[editIndex+1].trackIndex
                : this.#ranges.length
            // While we aren't updating a modified location and the location is on the line of
            // this edit.
            let idx = trackIndex + 1
            while (idx < nextTrackIndex && this.#ranges[idx].start.line === adjustedLine) {
                this.#ranges[idx] = modifyRange(this.#ranges[idx], lineDelta, columnDelta)
                ++idx
            }
            if (lineDelta !== 0) {
                while (idx < nextTrackIndex) {
                    this.#ranges[idx] = modifyRange(this.#ranges[idx], lineDelta, 0)
                    ++idx
                }
            }
        }
    }

    mkWorkspaceEdit(edits:NormalizedEdits):vscode.WorkspaceEdit {
        const edit = new vscode.WorkspaceEdit()
        for (const e of edits.array)
            edit.replace(this.#uri, this.#ranges[e.trackIndex], e.newText)
        return edit
    }

    /**
     * Checks whether the change to the range is expected.
     */
    private findEditIndex(edits: NormalizedEdits, updatedRange:vscode.Range, newText:string): number|null {
        const editArray = edits.array
        let l = 0
        let h = editArray.length - 1
        while (l <= h) {
            const m = Math.floor((l + h)/2)
            const e = editArray[m]
            const r = this.#ranges[e.trackIndex]
            if (r.start.isEqual(updatedRange.start)) {
                const isSame = r.end.isEqual(updatedRange.end) && e.newText === newText
                return isSame ? m : null
            } else if (r.start.isBefore(updatedRange.start)) {
                l = m+1
            } else {
                h = m-1
            }
        }
        return null
    }

    allExpected(edits: NormalizedEdits, changes:readonly vscode.TextDocumentContentChangeEvent[]):boolean {
        const editArray = edits.array
        if (edits === null) return false
        if (changes.length !== editArray.length) return false
        if (editArray.length === 0) return false

        const seen:(boolean|undefined)[] = new Array(editArray.length)
        // Check to see if all changes are expected
        let r = true
        for (const ce of changes) {
            let idx = this.findEditIndex(edits, ce.range, ce.text)
            // Fail if we cannot find this edit or we have already seen it
            if (idx === null || seen[idx])
                return false
            seen[idx] = true
        }
        // By pigeon hole principal we know we have seen all edits.
        return true
    }
}
