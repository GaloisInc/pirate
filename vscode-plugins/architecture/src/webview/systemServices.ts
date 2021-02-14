import * as A from "../shared/architecture.js"
import { ChangeSet, TrackedIndex } from "./changeSet.js"
import { Rectangle, XRange, YRange } from "./geometry.js"

export interface SystemServices {
    /**
     * Return true if @r@ overlaps with any object other than `thisObject`.
     */
    overlaps(thisObject: any, r: Rectangle<number>): boolean
    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustX(thisObject: any, r: YRange<number>, width: number, oldLeft: number, newLeft: number): number;
    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustY(thisObject: any, r: XRange<number>, height: number, oldTop: number, newTop: number): number;
    /**
     * Request we open a text editor at the given location.
     */
    visitURI(idx: A.LocationIndex): void
    /**
     * Send an update doc request
     */
    sendUpdateDoc(changes: ChangeSet): void
    /**
     * Add listener to respond to when a tracked value changes.
     */
    whenStringTrackChanged(trackedIndex: TrackedIndex, listener: (newValue: string) => void): void
    /**
     * Add listener to respond to when a tracked value changes.
     */
    whenIntTrackChanged(trackedIndex: TrackedIndex, listener: (newValue: number) => void): void
}
