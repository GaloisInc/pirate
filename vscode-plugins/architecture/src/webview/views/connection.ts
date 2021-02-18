import * as A from '../../shared/architecture.js'
import * as svg from '../svg.js'
import { SystemServices } from '../systemServices.js'

import { BusView } from './bus.js'
import { PortView } from './port.js'

export type Connectable = BusView | PortView

export class ConnectionView {

    readonly line: SVGLineElement

    updateSourcePosition(): void {

        if (this.source instanceof BusView) {
            this.x1 = this.x2
            this.y1 = this.source.draggableRectangle.top
        }

        if (this.source instanceof PortView) {
            this.x1 = this.source.actor.draggableRectangle.left + this.source.centerX
            this.y1 = this.source.actor.draggableRectangle.top + this.source.centerY
            if (this.target instanceof BusView) {
                if (this.target.orientation === A.BusOrientation.Horizontal) {
                    this.x2 = this.x1
                }
                if (this.target.orientation === A.BusOrientation.Vertical) {
                    this.y2 = this.y1
                }
            }
        }

    }

    updateTargetPosition(): void {

        if (this.target instanceof BusView) {
            this.x2 = this.x1
            this.y2 = this.target.draggableRectangle.top
        }

        if (this.target instanceof PortView) {
            this.x2 = this.target.actor.draggableRectangle.left + this.target.centerX
            this.y2 = this.target.actor.draggableRectangle.top + this.target.centerY
            if (this.source instanceof BusView) {
                if (this.source.orientation === A.BusOrientation.Horizontal) {
                    this.x1 = this.x2
                }
                if (this.source.orientation === A.BusOrientation.Vertical) {
                    this.y1 = this.y2
                }
            }
        }

    }

    getPortViewX(connectable: Connectable): number {
        if (connectable instanceof PortView) {
            return connectable.actor.draggableRectangle.left + connectable.centerX
        }
        if (connectable instanceof BusView) {
            console.log("Error: don't know how to display a connection between two BusViews")
        }
        console.log('Error: unexpected Connectable in getPortViewX')
        return 0
    }

    constructor(
        sys: SystemServices,
        parentSVG: SVGSVGElement,
        readonly source: Connectable,
        readonly target: Connectable,
    ) {
        this.line = document.createElementNS(svg.ns, 'line') as SVGLineElement
        this.line.classList.add('connection')
        this.line.setAttribute('stroke', 'green')
        this.line.setAttribute('stroke-width', '5')

        if (source instanceof PortView) {
            // initial values
            this.x1 = source.actor.draggableRectangle.left + source.centerX
            this.y1 = source.actor.draggableRectangle.top + source.centerY
            // updates
            sys.whenIntTrackChanged(source.offsetId, () => this.updateSourcePosition())
            sys.whenIntTrackChanged(source.actor.draggableRectangle.trackedIds.left, () => this.updateSourcePosition())
            sys.whenIntTrackChanged(source.actor.draggableRectangle.trackedIds.top, () => this.updateSourcePosition())
            sys.whenIntTrackChanged(source.actor.draggableRectangle.trackedIds.width, () => this.updateSourcePosition())
            sys.whenIntTrackChanged(source.actor.draggableRectangle.trackedIds.height, () => this.updateSourcePosition())
            sys.whenIntTrackChanged(source.actor.draggableRectangle.trackedIds.left, () => this.updateTargetPosition())
            sys.whenIntTrackChanged(source.actor.draggableRectangle.trackedIds.top, () => this.updateTargetPosition())
            sys.whenIntTrackChanged(source.actor.draggableRectangle.trackedIds.width, () => this.updateTargetPosition())
            sys.whenIntTrackChanged(source.actor.draggableRectangle.trackedIds.height, () => this.updateTargetPosition())
        }

        if (source instanceof BusView) {
            if (source.orientation === A.BusOrientation.Horizontal) {
                this.x1 = this.getPortViewX(target)
                this.y1 = source.draggableRectangle.top
                sys.whenIntTrackChanged(
                    source.draggableRectangle.trackedIds.top,
                    () => this.updateSourcePosition(),
                )
            }
            if (source.orientation === A.BusOrientation.Vertical) {
                this.x1 = source.draggableRectangle.left
                this.y1 = this.y2
                sys.whenIntTrackChanged(
                    source.draggableRectangle.trackedIds.left,
                    () => this.updateSourcePosition(),
                )
            }
        }

        if (target instanceof PortView) {
            this.x2 = target.actor.draggableRectangle.left + target.centerX
            this.y2 = target.actor.draggableRectangle.top + target.centerY
            sys.whenIntTrackChanged(target.offsetId, () => this.updateTargetPosition())
            sys.whenIntTrackChanged(target.actor.draggableRectangle.trackedIds.left, () => this.updateTargetPosition())
            sys.whenIntTrackChanged(target.actor.draggableRectangle.trackedIds.top, () => this.updateTargetPosition())
            sys.whenIntTrackChanged(target.actor.draggableRectangle.trackedIds.width, () => this.updateTargetPosition())
            sys.whenIntTrackChanged(target.actor.draggableRectangle.trackedIds.height, () => this.updateTargetPosition())
            sys.whenIntTrackChanged(target.actor.draggableRectangle.trackedIds.left, () => this.updateSourcePosition())
            sys.whenIntTrackChanged(target.actor.draggableRectangle.trackedIds.top, () => this.updateSourcePosition())
            sys.whenIntTrackChanged(target.actor.draggableRectangle.trackedIds.width, () => this.updateSourcePosition())
            sys.whenIntTrackChanged(target.actor.draggableRectangle.trackedIds.height, () => this.updateSourcePosition())
        }

        if (target instanceof BusView) {
            if (target.orientation === A.BusOrientation.Horizontal) {
                this.x2 = this.getPortViewX(source)
                this.y2 = target.draggableRectangle.top
                sys.whenIntTrackChanged(
                    target.draggableRectangle.trackedIds.top,
                    () => this.updateTargetPosition(),
                )
            }
            if (target.orientation === A.BusOrientation.Vertical) {
                this.x2 = target.draggableRectangle.left
                this.y2 = this.y1
                sys.whenIntTrackChanged(
                    target.draggableRectangle.trackedIds.left,
                    () => this.updateTargetPosition(),
                )
            }
        }

        parentSVG.appendChild(this.line)
    }

    private get x1() { return this.line.x1.baseVal.value }
    private set x1(x1: number) { this.line.x1.baseVal.value = x1 }
    private get x2() { return this.line.x2.baseVal.value }
    private set x2(x2: number) { this.line.x2.baseVal.value = x2 }
    private get y1() { return this.line.y1.baseVal.value }
    private set y1(y1: number) { this.line.y1.baseVal.value = y1 }
    private get y2() { return this.line.y2.baseVal.value }
    private set y2(y2: number) { this.line.y2.baseVal.value = y2 }

    /** Remove all components from SVG */
    dispose(): void {
        this.line.remove()
    }

}
