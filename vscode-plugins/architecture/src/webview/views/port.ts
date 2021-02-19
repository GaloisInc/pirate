import * as A from '../../shared/architecture.js'
import { ChangeSet } from '../changeSet.js'
import * as D from '../dragHandlers.js'
import * as M from '../mathematics.js'
import * as svg from '../svg.js'
import { SystemServices } from '../systemServices.js'

import type { ActorView } from './actor'

export enum PortDir { Out = 'Out', In = 'In' }

/**
 * Represents a port on a service.
 */
export class PortView {
    border: A.Border
    readonly borderId: A.TrackIndex
    // Element we created for port
    readonly elt: SVGUseElement
    readonly name: string
    offset: number
    readonly offsetId: A.TrackIndex

    get actorSVG(): SVGSVGElement { return this.actor.draggableRectangle.svgContainer }

    /**
     * Create a new port.
     */
    constructor(
        sys: SystemServices,
        readonly actor: ActorView,
        readonly direction: PortDir,
        p: A.Port
    ) {

        this.border = p.border.value
        this.borderId = p.border.trackId
        this.name = p.name.value
        this.offset = p.offset.value
        this.offsetId = p.offset.trackId

        sys.whenIntTrackChanged(this.offsetId, (offset) => {
            this.offset = offset
            this.setPortElementPosition()
        })

        sys.whenTrackedChanged(
            this.borderId,
            (s: string) => s as A.Border, // assumes we get a value from the enum
            (border) => {
                this.border = border
                this.setPortElementPosition()
            },
        )

        const elt = document.createElementNS(svg.ns, 'use') as SVGUseElement
        elt.setAttributeNS('', 'href', '#inPort')
        this.actorSVG.appendChild(elt)
        this.elt = elt

        const bbox = elt.getBBox()

        this.setPortElementPosition()

        D.addSVGDragHandlers(this.actorSVG, elt, (evt: D.SVGDragEvent) => {
            // Maximum left value
            const maxLeft = this.actorSVG.width.baseVal.value - bbox.width
            // Maximum top value
            const maxTop = this.actorSVG.height.baseVal.value - bbox.height
            // Calculate x distance above or below max.
            const xDist = outsideRangeDist(evt.left, 0, maxLeft)
            // Calculate y distance above or below max.
            const yDist = outsideRangeDist(evt.top, 0, maxTop)

            // Figuring out which side of the actor rectangle is closest to evt
            const distLeft = euclid2Dist(Math.abs(evt.left), yDist)
            const distRight = euclid2Dist(Math.abs(maxLeft - evt.left), yDist)
            const distTop = euclid2Dist(Math.abs(evt.top), xDist)
            const distBottom = euclid2Dist(Math.abs(maxTop - evt.top), xDist)
            const distMin = Math.min(distLeft, distTop, distRight, distBottom)

            let border: A.Border
            let offset: number
            if (distLeft === distMin) {
                border = A.Border.Left
                offset = evt.top
            } else if (distRight === distMin) {
                border = A.Border.Right
                offset = evt.top
            } else if (distTop === distMin) {
                border = A.Border.Top
                offset = evt.left
            } else {
                border = A.Border.Bottom
                offset = evt.left
            }
            // setPortElementPosition(actorSVG, elt, bbox, portType, border, offset)
            const changes = new ChangeSet()
            if (this.border !== border) {
                changes.replace(p.border.trackId, border)
            }
            if (this.offset !== offset) {
                changes.replace(p.offset.trackId, offset)
            }
            sys.sendUpdateDoc(changes)
        })
    }

    get centerX(): number {
        const bbox = this.elt.getBBox()
        return this.x + bbox.width / 2
    }

    get centerY(): number {
        const bbox = this.elt.getBBox()
        return this.y + bbox.height / 2
    }

    get x(): number { return this.elt.x.baseVal.value }
    set x(x: number) { this.elt.x.baseVal.value = x }
    get y(): number { return this.elt.y.baseVal.value }
    set y(y: number) { this.elt.y.baseVal.value = y }

    setPortElementPosition(): void {
        const bbox = this.elt.getBBox()

        // Maximum left value
        const maxLeft = this.actorSVG.width.baseVal.value - bbox.width
        // Maximum top value
        const maxTop = this.actorSVG.height.baseVal.value - bbox.height
        let offset: number
        const position = this.offset
        switch (this.border) {
            case A.Border.Left:
                offset = M.clamp(position, 0, maxTop)
                this.x = 0
                this.y = offset
                break
            case A.Border.Right:
                offset = M.clamp(position, 0, maxTop)
                this.x = maxLeft
                this.y = offset
                break
            case A.Border.Top:
                offset = M.clamp(position, 0, maxLeft)
                this.x = offset
                this.y = 0
                break
            case A.Border.Bottom:
                offset = M.clamp(position, 0, maxLeft)
                this.x = offset
                this.y = maxTop
                break
        }

        const rotate = calculatePortRotate(this.direction, this.x, maxLeft, this.y, maxTop)
        this.elt.setAttributeNS('', 'transform', rotateString(rotate, this.centerX, this.centerY))
    }

}

/** `outsideRangeDist(x,l,h)` returns the amount `x` is outside the range `[l,h]`. */
function outsideRangeDist(x: number, l: number, h: number): number {
    if (x < l) { return l - x } else if (x > h) { return h - x } else { return 0 }
}

/**
 * `euclid2Dist(x, y)` returns `sqrt(x * x + y * y)`.
 *
 * It has special cases and assumes `x` and `y` are non-negative.
 */
function euclid2Dist(x: number, y: number) {
    if (x === 0) { return y } else if (y === 0) { return x } else { return Math.sqrt(x * x + y * y) }
}

/**
 * Generate rotate string for a SVG CSS transform attribute.
 */
function rotateString(rotate: number, xoff: number, yoff: number): string {
    return `rotate(${rotate.toString()}, ${xoff.toString()}, ${yoff.toString()})`
}

/**
 * This calculate the orientation of the
 */
function calculatePortRotate(dir: PortDir, left: number, maxLeft: number, top: number, maxTop: number) {
    let orient: number
    if (top <= 0) {
        orient = 0
    } else if (top < maxTop) {
        orient = (left <= 0) ? 270 : 90
    } else {
        orient = 180
    }

    const inPort = dir === PortDir.In
    const angle = inPort ? 90 : 270

    return (orient + angle) % 360
}
