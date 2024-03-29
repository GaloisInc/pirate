/**
 * Interface via which changes to the documents are batched before being sent
 * to the document updater for processing.
 */

import { common } from '../shared/webviewProtocol'

export type TrackedIndex = number

export class ChangeSet {
    #edits: common.TrackUpdate[] = []

    replace(locationId: number, newText: number | string): void {
        this.#edits.push({
            trackIndex: locationId,
            newText: newText.toString(),
        })
    }

    get edits(): readonly common.TrackUpdate[] { return this.#edits }
}
