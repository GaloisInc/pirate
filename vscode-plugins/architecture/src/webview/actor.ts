import * as A from "../shared/architecture.js"
import * as D from "./dragHandlers.js"
import { webview } from "../shared/webviewProtocol.js"

import * as svg from './svg.js'
import { symlinkSync } from "fs"

export class ChangeSet {
    #changes: webview.DocEdit[] = []

    replace(locationId: number, newText: string): void {
        this.#changes.push({locationId: locationId, newText: newText})
    }

    get changes() { return this.#changes }
}

export interface SystemServices {
    /**
     * Return true if @r@ does overlaps with any service other than `thisService`.
     */
    overlaps(thisActor:string, r:Rect):boolean
    /**
     * Adjust the left coordinate to avoid overlapping.
     */    
    adjustX(thisActor:string, r:YRange, width:number, oldLeft:number, newLeft:number):number;
    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustY(thisActor:string, r:XRange, height:number, oldTop:number, newTop:number):number;

    sendAsyncRequest(m:webview.VisitURI):void

    /**
     * Send an update doc request
     */
    sendUpdateDoc(changes: ChangeSet):void
}

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

const svgns = svg.ns

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
                changes.replace(p.border.locationId, border)
            if (curOffset !== offset) {
                curOffset = offset
                changes.replace(p.offset.locationId, offset.toString())
            }
            sys.sendUpdateDoc(changes)
        })
    }

}

export interface XRange {
	readonly left:  number;
	readonly width: number;
}

export interface YRange {
	readonly top:    number;
	readonly height: number;
}

export interface Rect {
	readonly left:  number;
	readonly right: number;
	readonly top:    number;
	readonly bottom: number;
}

export class ActorView {
    #svgContainer:SVGSVGElement

    constructor(sys:SystemServices, parentSVG:SVGSVGElement, a:A.Actor) {
        const width = a.width.value
        const height = a.height.value

        // Offset for content
        const offset = { x : 20, y: 20 }

        // Create content
        const svgContainer  = document.createElementNS(svgns, 'svg') as SVGSVGElement
        svg.setUserUnits(svgContainer.x, a.left.value)
        svg.setUserUnits(svgContainer.y, a.top.value)
        svg.setUserUnits(svgContainer.width, width)
        svg.setUserUnits(svgContainer.height, height)
        this.#svgContainer = svgContainer

        const rect = document.createElementNS(svgns, 'rect') as SVGRectElement
        rect.classList.add('enclave')
        svg.setPercentageUnits(rect.x, 0)
        svg.setPercentageUnits(rect.y, 0)
        svg.setPercentageUnits(rect.width, 100)
        svg.setPercentageUnits(rect.height, 100)
        rect.style.fill = a.color.value

        function drag(evt:D.SVGDragEvent) {
            let newLeft = evt.left
            let newTop  = evt.top
            const width  = svgContainer.width.baseVal.value
            const height = svgContainer.height.baseVal.value
            
            // Adjust to not overlap
            newLeft = sys.adjustX(a.name.value, { top:  newTop, height: height }, width, svgContainer.x.baseVal.value, newLeft)
            newTop  = sys.adjustY(a.name.value, { left: newLeft, width: width }, height, svgContainer.y.baseVal.value, newTop)

            // Get new coordinates
            let r = { left:   newLeft,
                      top:    newTop,
                      right:  newLeft + width,
                      bottom: newTop  + height
                    }
            if (!sys.overlaps(a.name.value, r)) {
                let changes = new ChangeSet()
                if (svgContainer.x.baseVal.value !== newLeft) {
                    svgContainer.x.baseVal.value = newLeft
                    changes.replace(a.left.locationId, newLeft.toString())
                }
                if (svgContainer.y.baseVal.value !== newTop) {
                    svgContainer.y.baseVal.value = newTop
                    changes.replace(a.top.locationId, newTop.toString())
                }
                sys.sendUpdateDoc(changes)
            }
        };
        D.addSVGDragHandlers(parentSVG, svgContainer, drag)

        var div = document.createElement('div')
        var enclaveName = document.createElement('span') as HTMLSpanElement
        enclaveName.classList.add('enclave-name')
        enclaveName.innerHTML = a.name.value

        var visitClass = document.createElement('a')
        visitClass.classList.add('enclave-visit-class')
        visitClass.innerHTML = '[Visit Class]'
        let loc = a.location
        visitClass.onclick = e => {
            const cmd:webview.VisitURI = {
                tag: webview.Tag.VisitURI,
                filename: loc.filename,
                line: loc.line,
                column: loc.column
            }
            sys.sendAsyncRequest(cmd)
        }

        div.appendChild(enclaveName)
        div.appendChild(visitClass)
        div.onpointerdown = e => { e.stopImmediatePropagation() }

        const contentObject = document.createElementNS(svgns, 'foreignObject') as SVGForeignObjectElement
        contentObject.x.baseVal.value = offset.x
        contentObject.y.baseVal.value = offset.y
        svg.setPercentageUnits(contentObject.width, 100)
        svg.setPercentageUnits(contentObject.height, 100)
        contentObject.appendChild(div)

        svgContainer.appendChild(rect)
        svgContainer.appendChild(contentObject)
        parentSVG.appendChild(svgContainer)
        
        for (const p of a.inPorts)
            new PortView(sys, this.#svgContainer, PortDir.In, p)
        for (const p of a.outPorts)
            new PortView(sys, this.#svgContainer, PortDir.Out, p)
     }

     /** Remove all components from SVG */
     dispose() {
         this.#svgContainer.remove()
     }

     get left():number { return this.#svgContainer.x.baseVal.value }
     get top():number { return this.#svgContainer.y.baseVal.value }
     get width():number  { return this.#svgContainer.width.baseVal.value }
     get height():number { return this.#svgContainer.height.baseVal.value }
     get right():number { return this.left + this.width }
     get bottom():number { return this.top + this.height }
}