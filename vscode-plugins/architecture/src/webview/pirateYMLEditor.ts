import * as D from "./dragHandlers.js"
import * as U from "../shared/modelUpdates.js"
import * as R from "../shared/viewRequests.js"
import * as A from "../shared/architecture.js"

declare var acquireVsCodeApi: any

const vscode = acquireVsCodeApi()

const svgns = "http://www.w3.org/2000/svg"

interface Rect {
	readonly left:  number;
	readonly right: number;
	readonly top:    number;
	readonly bottom: number;
}

interface XRange {
	readonly left:  number;
	readonly width: number;
}

interface YRange {
	readonly top:    number;
	readonly height: number;
}

const lasterrormsg = document.getElementById('lasterrormsg') as HTMLDivElement

class System {
    #svg:SVGSVGElement
    #actors:Map<string, ActorView> = new Map()
    constructor(svg:SVGSVGElement) {
       this.#svg = svg
    }

    setSystemLayout(m:A.SystemLayout):void {
        // Dispose existing components
        this.#actors.forEach((a,nm,m) => a.dispose())

        for (const a of m.actors) {
            var av = new ActorView(this, this.#svg, a)
            this.#actors.set(a.name.value, av)
        }
    }

    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustX(thisService:string, r:YRange, width:number, oldLeft:number, newLeft:number):number {
        const l = this.#actors
        const top = r.top
        const bottom = top + r.height
        // If we move left
        if (newLeft < oldLeft) {
            for (const nm of l.keys()) {
               if (nm === thisService) continue 
                const o = l.get(nm) as ActorView
                if (bottom <= o.top) continue 
                if (o.bottom <= top) continue 
                // If o is left of oldLeft and right of newLeft
                if (o.right <= oldLeft) 
                    newLeft = Math.max(o.right, newLeft)
            }
        }
        // If we move right
        if (newLeft > oldLeft) {
            let oldRight = oldLeft + width
            let newRight = newLeft + width
            for (const nm of l.keys()) {
                if (nm === thisService) continue 
                const o = l.get(nm) as ActorView
                if (bottom <= o.top) continue 
                if (o.bottom <= top) continue 
                // If o is right of oldRight and left of newRight.
                if (oldRight <= o.left) 
                    newRight = Math.min(newRight, o.left)
            }
            newLeft = newRight - width
        }
        return newLeft
     }

    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustY(thisService:string, r:XRange, height:number, oldTop:number, newTop:number):number {
        const l = this.#actors
        const left = r.left
        const right = left + r.width
        // If we move up
        if (newTop < oldTop) {
            for (const nm of l.keys()) {
                if (nm === thisService) continue 
                const o = l.get(nm) as ActorView
                if (right <= o.left) continue 
                if (o.right <= left) continue 
                // If o is above of oldTop
                if (o.bottom <= oldTop) 
                    newTop = Math.max(o.bottom, newTop)
            }
        }
        // If we move right
        if (newTop > oldTop) {
            let oldBottom = oldTop + height
            let newBottom = newTop + height
            for (const nm of l.keys()) {
                if (nm === thisService) continue 
                const o = l.get(nm) as ActorView
                if (right <= o.left) continue 
                if (o.right <= left) continue 
                // If o is right of oldRight and left of newRight.
                if (oldBottom <= o.top) 
                    newBottom = Math.min(newBottom, o.top)
                
            }
            newTop = newBottom - height
        }
        return newTop
     }

    /**
     * Return true if @r@ does overlaps with any service other than `thisService`.
     */
    overlaps(thisService:string, r:Rect):boolean {
       const l = this.#actors
       for (const nm of l.keys()) {
          if (nm === thisService) continue 
          const o = l.get(nm) as ActorView
          if (r.right <= o.left) continue 
          if (o.right <= r.left) continue 
          if (r.bottom <= o.top) continue 
          if (o.bottom <= r.top) continue 
          return true
       }
       return false
    }
}

function logEvent(msg:string) {
    lasterrormsg.innerHTML = msg
}

interface CoordsInterface {
	readonly left: number;
	readonly top: number;
	readonly width: number;
	readonly height: number;
}

class Coords implements CoordsInterface {
    constructor(c: CoordsInterface) {
        this.top = c.top
        this.left = c.left
        this.width = c.width
        this.height = c.height
    }
    top:number
    left:number
    width:number
    height:number
}

class ActorView {
    #coords:Coords
    #innerSVG:SVGSVGElement
    #ports:Map<string, PortView> = new Map()

    constructor(sys:System, svg:SVGSVGElement, a:A.Actor) {
        const coords = new Coords({ left: a.left.value, width: a.width.value, top: a.top.value, height: a.height.value})
        // Offset for content
        const offset = { x : 20, y: 20 }
        this.#coords = coords
        // Create content for ports

        const innerSVG  = document.createElementNS(svgns, 'svg') as SVGSVGElement
        innerSVG.x.baseVal.value = coords.left
        innerSVG.y.baseVal.value = coords.top
        innerSVG.setAttributeNS('', 'width', coords.width.toString())
        innerSVG.setAttributeNS('', 'height', coords.height.toString())
        this.#innerSVG = innerSVG

        const rect          = document.createElementNS(svgns, 'rect') as SVGRectElement
        rect.classList.add('enclave')
        rect.x.baseVal.value = 0
        rect.y.baseVal.value = 0
        rect.width.baseVal.value = coords.width
        rect.height.baseVal.value = coords.height
        rect.style.fill = a.color.value
        const contentObject = document.createElementNS(svgns, 'foreignObject') as SVGForeignObjectElement

        let ports = this.#ports

        function drag(evt:D.SVGDragEvent) {
            let left = evt.left
            let top  = evt.top
            const width  = +(innerSVG.getAttributeNS('', 'width') as any)
            const height = +(innerSVG.getAttributeNS('', 'height') as any)

            // Adjust to not overlap
            left = sys.adjustX(a.name.value, { top:  top, height: height }, width, innerSVG.x.baseVal.value, left)
            top  = sys.adjustY(a.name.value, { left: left, width: width }, height, innerSVG.y.baseVal.value, top)

            // Get new coordinates
            let r = { left:   left,
                      top:    top,
                      right:  left + width,
                      bottom: top  + height
                    }
            if (!sys.overlaps(a.name.value, r)) {
                const delta = { x: left - innerSVG.x.baseVal.value, y: top - innerSVG.y.baseVal.value}
                coords.left = left
                coords.top  = top
                innerSVG.x.baseVal.value = left
                innerSVG.y.baseVal.value = top
             }
        };
        D.addSVGDragHandlers(svg, innerSVG, drag)

        var div = document.createElement('div')
        var enclaveName = document.createElement('span') as HTMLSpanElement
        enclaveName.classList.add('enclave-name')
        enclaveName.innerHTML = a.name.value

        var visitClass = document.createElement('a')
        visitClass.classList.add('enclave-visit-class')
        visitClass.innerHTML = '[Visit Class]'
        visitClass.onclick = e => {
            const cmd:R.VisitURI = {
                tag: R.Tag.VisitURI,
                filename: a.location.value.filename,
                line: a.location.value.line,
                column: a.location.value.column
            }
            vscode.postMessage(cmd)
        }

        div.appendChild(enclaveName)
        div.appendChild(visitClass)
        div.onpointerdown = e => { e.stopImmediatePropagation() }

        contentObject.x.baseVal.value = offset.x
        contentObject.y.baseVal.value = offset.y
        contentObject.width.baseVal.value = innerSVG.width.baseVal.value - 40
        contentObject.height.baseVal.value = innerSVG.height.baseVal.value - 40
        contentObject.appendChild(div)

        innerSVG.appendChild(rect)
        innerSVG.appendChild(contentObject)
        svg.appendChild(innerSVG)
        for (const p of a.inPorts) {
            const pv = new PortView(this, this.#innerSVG, PortDir.In, p)
            this.#ports.set(p.name.value, pv)
        }
        for (const p of a.outPorts) {
            const pv = new PortView(this, this.#innerSVG, PortDir.Out, p)
            this.#ports.set(p.name.value, pv)
        }

     }

     /** Remove all components from SVG */
     dispose() {
         this.#innerSVG.remove()
     }

     get left()   { return this.#coords.left }
     get top()    { return this.#coords.top }
     get width()  { return this.#innerSVG.width.baseVal.value }
     get height() { return this.#innerSVG.height.baseVal.value }
     get right()  { return this.left + this.width }
     get bottom() { return this.#coords.top + this.#coords.height }
     // Return coordinates in center.
     get center() {
        var coords = this.#coords
        return { x: coords.left + coords.width / 2,
                 y: coords.top + coords.height / 2
               }
     }
}

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
 * Set the position and orientationof a port use element.
 *
 * @param svg Outer SVG element
 * @param elt Use element
 * @param bbox Bounding box of a element
 * @param portDir Type of port
 * @param border Border that port sits on
 * @param position Position along border that port is on.
 */
function setPortElementPosition(svg:SVGSVGElement, elt:SVGUseElement, bbox:DOMRect, portDir: PortDir, border: A.Border, position:number) {
    // Maximum left value
    const maxLeft = svg.width.baseVal.value - bbox.width
    // Maximum top value
    const maxTop = svg.height.baseVal.value - bbox.height
    let maxPosition:number
    switch (border) {
    case A.Border.Left:
        elt.x.baseVal.value = 0
        elt.y.baseVal.value = clamp(position, 0, maxTop)
        break
    case A.Border.Right:
        elt.x.baseVal.value = maxLeft
        elt.y.baseVal.value = clamp(position, 0, maxTop)
        break
    case A.Border.Top:
        elt.x.baseVal.value = clamp(position, 0, maxLeft)
        elt.y.baseVal.value = 0
        break
    case A.Border.Bottom:
        elt.x.baseVal.value = clamp(position, 0, maxLeft)
        elt.y.baseVal.value = maxTop
        break
    }
    // Populate initial transform attribute.
    const rotate = calculatePortRotate(portDir, elt.x.baseVal.value, maxLeft, elt.y.baseVal.value, maxTop)
    const centerX = elt.x.baseVal.value + bbox.width / 2
    const centerY = elt.y.baseVal.value + bbox.height / 2
    elt.setAttributeNS('', 'transform', rotateString(rotate, centerX, centerY))
}

/**
 * Represents a port on a service.
 */
class PortView {
    // Channels connected to this port.
    #channels:Channel[] = []
    // Element we created for port

    readonly elt:SVGUseElement

    /**
     * Create a new port.
     */
    constructor(sys:ActorView, svg:SVGSVGElement, dir:PortDir, p:A.Port) {
        const portType = dir
        const elt = document.createElementNS(svgns, 'use') as SVGUseElement
        elt.setAttributeNS('', 'href', '#inPort')
        svg.appendChild(elt)
        this.elt = elt

        var bbox = elt.getBBox()

        setPortElementPosition(svg, elt, bbox, portType, p.border.value, p.offset.value)


        D.addSVGDragHandlers(svg, elt, (evt:D.SVGDragEvent) => {
            // Maximum left value
            const maxLeft = svg.width.baseVal.value - bbox.width
            // Maximum top value
            const maxTop = svg.height.baseVal.value - bbox.height
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
            let position:number
            if (distLeft === distMin) {
                border = A.Border.Left
                position = evt.top
            } else if (distRight === distMin) {
                border = A.Border.Right
                position = evt.top
            } else if (distTop === distMin) {
                border = A.Border.Top
                position = evt.left
            } else {
                border = A.Border.Bottom
                position = evt.left
            }
            setPortElementPosition(svg, elt, bbox, portType, border, position)
        })
    }

    addChannel(c:Channel) {
        this.#channels.push(c)
     }

}

class Channel {
    update() {
    }
}

const panel = (document.getElementById('panel') as unknown) as SVGSVGElement

const sys = new System(panel)

window.addEventListener('message', event => {
    const message = event.data as U.ModelUpdate // The json data that the extension sent
    switch (message.tag) {
    case U.Tag.SetSystemLayout:
        sys.setSystemLayout(message.system)
        break
    }
})