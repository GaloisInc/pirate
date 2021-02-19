import * as A from '../shared/architecture.js'

import { ChangeSet, TrackedIndex } from './changeSet.js'
import { Rectangle, XRange, YRange } from './geometry.js'

export interface SystemServices {

    /**
     * Return true if @r@ overlaps with any object other than `thisObject`.
     */
    overlaps(_thisObject: unknown, _r: Rectangle<number>): boolean

    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustX(_thisObject: unknown, _r: YRange<number>, _width: number, _oldLeft: number, _newLeft: number): number;

    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustY(_thisObject: unknown, _r: XRange<number>, _height: number, _oldTop: number, _newTop: number): number;

    /**
     * Request we open a text editor at the given location.
     */
    visitURI(_idx: A.LocationIndex): void

    /**
     * Send an update doc request
     */
    sendUpdateDoc(_changes: ChangeSet): void

    /**
     * Add listener to respond to when a tracked value changes.
     */
    whenIntTrackChanged(_trackedIndex: TrackedIndex, _listener: (_newValue: number) => void): void

    /**
     * Add listener to respond to when a tracked value changes.
     */
    whenStringTrackChanged(_trackedIndex: TrackedIndex, _listener: (_newValue: string) => void): void

    whenTrackedChanged<V>(
        _idx: TrackedIndex,
        _parser: (_newValue: string) => V | undefined,
        _listener: (_newValue: V) => void,
    ): void


}
