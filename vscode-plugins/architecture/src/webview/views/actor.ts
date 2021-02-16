import * as A from "../../shared/architecture.js"
import { ChangeSet } from "../changeSet.js"
import { DraggableRectangle } from "../draggableRectangle.js"
import * as D from "../dragHandlers.js"
import * as svg from '../svg.js'
import { SystemServices } from '../systemServices.js'

/** `outsideRangeDist(x,l,h)` returns the amount `x` is outside the range `[l,h]`. */
function outsideRangeDist(x:number, l:number, h:number):number {
    if (x < l)
        return l - x
    else if (x > h)
        return h - x
    else
        return 0
}

/**
 * `euclid2Dist(x, y)` returns `sqrt(x * x + y * y)`.
 *
 * It has special cases and assumes `x` and `y` are non-negative.
 */
function euclid2Dist(x:number, y:number) {
    if (x === 0)
        return y
    else if (y === 0)
        return x
    else
        return Math.sqrt(x * x + y * y)
}

enum PortDir { Out = "Out", In = "In"}

/**
 * Clamp a number between a minimum and maximum value.
 */
function clamp(v:number, min:number, max:number) {
    return Math.min(Math.max(v, min),  max)
}

/**
 * This calculate the orientation of the
 */
function calculatePortRotate(dir: PortDir, left:number, maxLeft:number, top:number, maxTop:number) {
    let orient:number
    if (top <= 0)
        orient = 0
    else if (top < maxTop)
        orient = (left <= 0) ? 270 : 90
    else
        orient = 180

    const inPort = dir === PortDir.In

    if (inPort)
        return (orient + 90) % 360
    else
        return (orient + 270) % 360
}

/**
 * Generate rotate string for a SVG CSS transform attribute.
 */
function rotateString(rotate:number, xoff:number, yoff:number):string {
    return 'rotate(' + rotate.toString() + ', '
                     + xoff.toString() + ', '
                     + yoff.toString() + ')'

}
/**
 * Set the position and orientationof a port use element.
 *
 * @param svg Outer SVG element
 * @param elt Use element
 * @param bbox Bounding box of a element
 * @param portDir Type of port
 * @param border Border that port sits on
 * @param position Position along border that port is on.
 */
function setPortElementPosition(svg:SVGSVGElement, elt:SVGUseElement, bbox:DOMRect, portDir: PortDir, border: A.Border, position:number):number {
    // Maximum left value
    const maxLeft = svg.width.baseVal.value - bbox.width
    // Maximum top value
    const maxTop = svg.height.baseVal.value - bbox.height
    let offset:number
    switch (border) {
    case A.Border.Left:
        offset = clamp(position, 0, maxTop)
        elt.x.baseVal.value = 0
        elt.y.baseVal.value = offset
        break
    case A.Border.Right:
        offset = clamp(position, 0, maxTop)
        elt.x.baseVal.value = maxLeft
        elt.y.baseVal.value = offset
        break
    case A.Border.Top:
        offset = clamp(position, 0, maxLeft)
        elt.x.baseVal.value = offset
        elt.y.baseVal.value = 0
        break
    case A.Border.Bottom:
        offset = clamp(position, 0, maxLeft)
        elt.x.baseVal.value = offset
        elt.y.baseVal.value = maxTop
        break
    }
    // Populate initial transform attribute.
    const rotate = calculatePortRotate(portDir, elt.x.baseVal.value, maxLeft, elt.y.baseVal.value, maxTop)
    const centerX = elt.x.baseVal.value + bbox.width / 2
    const centerY = elt.y.baseVal.value + bbox.height / 2
    elt.setAttributeNS('', 'transform', rotateString(rotate, centerX, centerY))
    return offset
}

/**
 * Represents a port on a service.
 */
class PortView {
    // Element we created for port
    readonly elt:SVGUseElement

    /**
     * Create a new port.
     */
    constructor(sys:SystemServices, actorSVG:SVGSVGElement, dir:PortDir, p:A.Port) {
        const portType = dir
        const elt = document.createElementNS(svg.ns, 'use') as SVGUseElement
        elt.setAttributeNS('', 'href', '#inPort')
        actorSVG.appendChild(elt)
        this.elt = elt

        var bbox = elt.getBBox()

        let curBorder = p.border.value
        let curOffset  = p.offset.value

        setPortElementPosition(actorSVG, elt, bbox, portType, curBorder, p.offset.value)

        D.addSVGDragHandlers(actorSVG, elt, (evt:D.SVGDragEvent) => {
            // Maximum left value
            const maxLeft = actorSVG.width.baseVal.value - bbox.width
            // Maximum top value
            const maxTop = actorSVG.height.baseVal.value - bbox.height
            // Calculate x distance above or below max.
            let xDist = outsideRangeDist(evt.left, 0, maxLeft)
            // Calculate y distance above or below max.
            let yDist = outsideRangeDist(evt.top, 0, maxTop)

            const distLeft   = euclid2Dist(Math.abs(evt.left),           yDist)
            const distRight  = euclid2Dist(Math.abs(maxLeft - evt.left), yDist)
            const distTop    = euclid2Dist(Math.abs(evt.top),            xDist)
            const distBottom = euclid2Dist(Math.abs(maxTop  - evt.top),  xDist)
            const distMin = Math.min(distLeft, distTop, distRight, distBottom)

            let border: A.Border
            let offset:number
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
            offset = setPortElementPosition(actorSVG, elt, bbox, portType, border, offset)
            let changes = new ChangeSet()
            if (curBorder !== border)
                changes.replace(p.border.trackId, border)
            if (curOffset !== offset) {
                curOffset = offset
                changes.replace(p.offset.trackId, offset)
            }
            sys.sendUpdateDoc(changes)
        })
    }

}

export class ActorView {
    readonly draggableRectangle: DraggableRectangle

    constructor(sys: SystemServices, parentSVG: SVGSVGElement, a: A.Actor) {

        this.draggableRectangle = new DraggableRectangle(
            sys,
            parentSVG,
            {
                left: a.left,
                top: a.top,
                width: a.width,
                height: a.height,
            }
        )

        const width = a.width.value
        const height = a.height.value

        this.draggableRectangle.rect.style.fill = a.color.value

        var div = document.createElement('div')
        var enclaveName = document.createElement('span') as HTMLSpanElement
        enclaveName.classList.add('enclave-name')
        enclaveName.innerHTML = a.name.value

        var visitClass = document.createElement('a')
        visitClass.classList.add('enclave-visit-class')
        visitClass.innerHTML = '[Visit Class]'
        let loc = a.location
        visitClass.onclick = e => sys.visitURI(loc)

        div.appendChild(enclaveName)
        div.appendChild(visitClass)
        div.onpointerdown = e => { e.stopImmediatePropagation() }

        this.draggableRectangle.contentObject.appendChild(div)

        for (const p of a.inPorts)
            new PortView(sys, this.draggableRectangle.svgContainer, PortDir.In, p)
        for (const p of a.outPorts)
            new PortView(sys, this.draggableRectangle.svgContainer, PortDir.Out, p)
    }

    /** Remove all components from SVG */
    dispose() {
        this.draggableRectangle.dispose()
    }

}
