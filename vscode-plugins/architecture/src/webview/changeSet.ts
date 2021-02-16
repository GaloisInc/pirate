import { common } from '../shared/webviewProtocol.js'

export type TrackedIndex = number

export class ChangeSet {
    #edits: common.TrackUpdate[] = []

    replace(locationId: number, newText: number | string): void {
        this.#edits.push({
            trackIndex: locationId,
            newText: newText.toString(),
        })
    }

    get edits(): common.TrackUpdate[] { return this.#edits }
}
