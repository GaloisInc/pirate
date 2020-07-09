import * as U from "../shared/modelUpdates";
//import type * as R from "../shared/viewrequests";
import * as R from "../shared/viewRequests";
import { TextEditorRevealType } from "vscode";
import { Func } from "mocha";
import { exitCode } from "process";

declare var acquireVsCodeApi: any;

const vscode = acquireVsCodeApi();

const svgns = "http://www.w3.org/2000/svg";

interface Rect {
	readonly left:  number;
	readonly right: number;
	readonly top:    number;
	readonly bottom: number;
}

interface XRange {
	readonly left:    number;
	readonly width: number;
}

interface YRange {
	readonly top:    number;
	readonly height: number;
}

interface Shift {
    readonly x: number;
    readonly y: number;
}

const lasterrormsg = document.getElementById('lasterrormsg') as HTMLDivElement;


class System {
    #svg:SVGSVGElement;
    #services:Map<string, Service> = new Map();
    constructor(svg:SVGSVGElement) {
       this.#svg = svg;
    }

    newService(u:U.NewService) {
        if (this.#services.has(u.name))
            return;

        var s = new Service(this, this.#svg, u);
        this.#services.set(u.name, s);
    }

    /**
     * Handle new port request
     * @param u
     */
    newPort(u:U.NewPort) {
        const service = this.#services.get(u.serviceName);
        if (!service) {
            return;
        }
        service.newPort(this.#svg, u);
    }

    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustX(thisService:string, r:YRange, width:number, oldLeft:number, newLeft:number):number {
        const l = this.#services;
        const top = r.top;
        const bottom = top + r.height;
        // If we move left
        if (newLeft < oldLeft) {
            for (const nm of l.keys()) {
               if (nm === thisService) { continue; }
                const o = l.get(nm) as Service;
                if (bottom <= o.top) { continue; }
                if (o.bottom <= top) { continue; }
                // If o is left of oldLeft and right of newLeft
                if (o.right <= oldLeft) {
                    newLeft = Math.max(o.right, newLeft);
                }
            }
        }
        // If we move right
        if (newLeft > oldLeft) {
            let oldRight = oldLeft + width;
            let newRight = newLeft + width;
            for (const nm of l.keys()) {
                if (nm === thisService) { continue; }
                const o = l.get(nm) as Service;
                if (bottom <= o.top) { continue; }
                if (o.bottom <= top) { continue; }
                // If o is right of oldRight and left of newRight.
                if (oldRight <= o.left) {
                    newRight = Math.min(newRight, o.left);
                }
            }
            newLeft = newRight - width;
        }
        return newLeft;
     }

    /**
     * Adjust the left coordinate to avoid overlapping.
     */
    adjustY(thisService:string, r:XRange, height:number, oldTop:number, newTop:number):number {
        const l = this.#services;
        const left = r.left;
        const right = left + r.width;
        // If we move up
        if (newTop < oldTop) {
            for (const nm of l.keys()) {
               if (nm === thisService) { continue; }
                const o = l.get(nm) as Service;
                if (right <= o.left) { continue; }
                if (o.right <= left) { continue; }
                // If o is above of oldTop
                if (o.bottom <= oldTop) {
                    newTop = Math.max(o.bottom, newTop);
                }
            }
        }
        // If we move right
        if (newTop > oldTop) {
            let oldBottom = oldTop + height;
            let newBottom = newTop + height;
            for (const nm of l.keys()) {
                if (nm === thisService) { continue; }
                const o = l.get(nm) as Service;
                if (right <= o.left) { continue; }
                if (o.right <= left) { continue; }
                // If o is right of oldRight and left of newRight.
                if (oldBottom <= o.top) {
                    newBottom = Math.min(newBottom, o.top);
                }
            }
            newTop = newBottom - height;
        }
        return newTop;
     }

    /**
     * Return true if @r@ does overlaps with any service other than `thisService`.
     */
    overlaps(thisService:string, r:Rect):boolean {
       const l = this.#services;
       for (const nm of l.keys()) {
          if (nm === thisService) { continue; }
          const o = l.get(nm) as Service;
          if (r.right <= o.left) { continue; }
          if (o.right <= r.left) { continue; }
          if (r.bottom <= o.top) { continue; }
          if (o.bottom <= r.top) { continue; }
          return true;
       }
       return false;
    }
}

function logEvent(msg:string) {
    lasterrormsg.innerHTML = msg;
}

class Coords implements U.CoordsInterface {
    constructor(c:U.CoordsInterface) {
        this.top = c.top;
        this.left = c.left;
        this.width = c.width;
        this.height = c.height;
    }
    top:number;
    left:number;
    width:number;
    height:number;
}

/**
 * Interface needed for addSVGDragHandlers to make an element dragable.
 */
interface DragableSVGElement extends Element {
    readonly x:SVGAnimatedLength;
    readonly y:SVGAnimatedLength;
    onpointerdown: ((this: GlobalEventHandlers, ev: PointerEvent) => any) | null;
    onpointermove: ((this: GlobalEventHandlers, ev: PointerEvent) => any) | null;
    onpointerup: ((this: GlobalEventHandlers, ev: PointerEvent) => any) | null;
}

/**
 * Coordinates for recording where dragging started.
 */
interface DragCoords {
    readonly rx: number;
    readonly ry: number;
    readonly px: number;
    readonly py: number;
}

/**
 *
 */
interface DragEvent {
    /**
     * Top coordinate in SVG address space.
     */
    readonly top: number;
    /**
     * Left coordinate to drag to in SVG address space.
     */
    readonly left: number;
}

function addSVGDragHandlers(container:SVGSVGElement, innerSVG:DragableSVGElement, drag:(p:DragEvent) => void) {
    let dragOffset:DragCoords|null = null;
    function startDrag(evt:PointerEvent) {
        dragOffset = {
            rx: innerSVG.x.baseVal.value,
            ry: innerSVG.y.baseVal.value,
            px: evt.pageX,
            py: evt.pageY
        };
        innerSVG.setPointerCapture(evt.pointerId);
        evt.stopImmediatePropagation();
    }
    function onpointermove(evt:PointerEvent) {
        if (dragOffset) {
            evt.stopImmediatePropagation();
            const CTM = container.getScreenCTM() as DOMMatrix;
            // Get left
            let left = dragOffset.rx + (evt.pageX - dragOffset.px) / CTM.a;
            let top  = dragOffset.ry + (evt.pageY - dragOffset.py) / CTM.d;
            drag({left: left, top: top});
         }
    };

    function endDrag(evt:PointerEvent) {
        if (dragOffset) {
            dragOffset = null;
            innerSVG.releasePointerCapture(evt.pointerId);
            evt.stopImmediatePropagation();
        }
    };
    innerSVG.onpointerdown = startDrag;
    innerSVG.onpointermove = onpointermove;
    innerSVG.onpointerup = endDrag;
}

class Service {
    #coords:Coords;
    #innerSVG:SVGSVGElement;
    #ports:Map<string, Port> = new Map();

    constructor(sys:System, svg:SVGSVGElement, p:U.NewService) {
        const coords = new Coords(p.coords);
        // Offset for content
        const offset = { x : 20, y: 20 };
        this.#coords = coords;
        // Create content for ports

        const innerSVG  = document.createElementNS(svgns, 'svg') as SVGSVGElement;
        innerSVG.x.baseVal.value = coords.left;
        innerSVG.y.baseVal.value = coords.top;
        innerSVG.setAttributeNS('', 'width', coords.width.toString());
        innerSVG.setAttributeNS('', 'height', coords.height.toString());
        this.#innerSVG = innerSVG;

        const rect          = document.createElementNS(svgns, 'rect') as SVGRectElement;
        rect.classList.add('enclave');
        rect.x.baseVal.value = 0;
        rect.y.baseVal.value = 0;
        rect.width.baseVal.value = p.coords.width;
        rect.height.baseVal.value = p.coords.height;
        rect.style.fill = p.color;
        const contentObject = document.createElementNS(svgns, 'foreignObject') as SVGForeignObjectElement;

        let ports = this.#ports;

        function drag(evt:DragEvent) {
            let left = evt.left;
            let top  = evt.top;
            const width  = +(innerSVG.getAttributeNS('', 'width') as any);
            const height = +(innerSVG.getAttributeNS('', 'height') as any);

            // Adjust to not overlap
            left = sys.adjustX(p.name, { top:  top, height: height }, width, innerSVG.x.baseVal.value, left);
            top  = sys.adjustY(p.name, { left: left, width: width }, height, innerSVG.y.baseVal.value, top);

            // Get new coordinates
            let r = { left:   left,
                      top:    top,
                      right:  left + width,
                      bottom: top  + height
                    };
            if (!sys.overlaps(p.name, r)) {
                const delta = { x: left - innerSVG.x.baseVal.value, y: top - innerSVG.y.baseVal.value};
                coords.left = left;
                coords.top  = top;
                innerSVG.x.baseVal.value = left;
                innerSVG.y.baseVal.value = top;
             }
        };
        addSVGDragHandlers(svg, innerSVG, drag);

        var div = document.createElement('div');
        var enclaveName = document.createElement('span') as HTMLSpanElement;
        enclaveName.classList.add('enclave-name');
        enclaveName.innerHTML = p.name;

        var visitClass = document.createElement('a');
        visitClass.classList.add('enclave-visit-class');
        visitClass.innerHTML = '[Visit Class]';
        visitClass.onclick = e => {
            const cmd:R.VisitServiceClass = {
                tag: R.Tag.VisitServiceClass,
                name: p.name
            };
            vscode.postMessage(cmd);
        };

        div.appendChild(enclaveName);
        div.appendChild(visitClass);
        div.onpointerdown = e => { e.stopImmediatePropagation(); };

        contentObject.x.baseVal.value = offset.x;
        contentObject.y.baseVal.value = offset.y;
        contentObject.width.baseVal.value = innerSVG.width.baseVal.value - 40;
        contentObject.height.baseVal.value = innerSVG.height.baseVal.value - 40;
        contentObject.appendChild(div);

        innerSVG.appendChild(rect);
        innerSVG.appendChild(contentObject);
        svg.appendChild(innerSVG);
     }
     get left()   { return this.#coords.left; }
     get top()    { return this.#coords.top; }
     get width()  { return this.#innerSVG.width.baseVal.value; }
     get height() { return this.#innerSVG.height.baseVal.value; }
     get right()  { return this.left + this.width; }
     get bottom() { return this.#coords.top + this.#coords.height; }
     // Return coordinates in center.
     get center() {
        var coords = this.#coords;
        return { x: coords.left + coords.width / 2,
                 y: coords.top + coords.height / 2
               };
     }

     /**
      * Create a new port in this service.
      *
      * Note. The port should already be checked to belong to this service.
      *
      * @param p A port to add to this service.
      */
     newPort(svg:SVGSVGElement, params:U.NewPort) {
        const p = new Port(this, this.#innerSVG, params);
        this.#ports.set(params.portName, p);

     }
}

/**
 * Clamp a number between a minimum and maximum value.
 */
function clamp(v:number, min:number, max:number) {
    return Math.min(Math.max(v, min),  max);
}

/**
 *
 */
function calculatePortRotate(mode:U.PortType, border:U.Border):number {
    const inPort = mode == U.PortType.InPort;
    switch (border) {
    case U.Border.Left:
        return inPort ? 180 : 0;
    case U.Border.Right:
        return inPort ? 0 : 180;
    case U.Border.Top:
        return inPort ? -90 : 90;
    case U.Border.Bottom:
        return inPort ? 90 : -90;
    }
}

/**
 * Generate rotate string for a SVG CSS transform attribute.
 */
function rotateString(rotate:number, xoff:number, yoff:number):string {
    return 'rotate(' + rotate.toString() + ', '
                     + xoff.toString() + ', '
                     + yoff.toString() + ')';

}

/** `outsideRangeDist(x,l,h)` returns the amount `x` is outside the range `[l,h]`. */
function outsideRangeDist(x:number, l:number, h:number):number {
    if (x < l) {
        return l - x;
    } else if (x > h) {
        return h - x;
    } else {
        return 0;
    }
}

/**
 * `euclid2Dist(x, y)` returns `sqrt(x * x + y * y)`.
 *
 * It has special cases and assumes `x` and `y` are non-negative.
 */
function euclid2Dist(x:number, y:number) {
    if (x == 0) {
        return y;
    } else if (y == 0) {
        return x;
    } else {
        return Math.sqrt(x * x + y * y);
    }
}

/**
 * Set the position and orientationof a port use element.
 *
 * @param svg Outer SVG element
 * @param elt Use element
 * @param bbox Bounding box of a element
 * @param portType Type of port
 * @param border Border that port sits on
 * @param position Position along border that port is on.
 */
function setPortElementPosition(svg:SVGSVGElement, elt:SVGUseElement, bbox:DOMRect, portType:U.PortType, border:U.Border, position:number) {
    // Maximum left value
    const maxLeft = svg.width.baseVal.value - bbox.width;
    // Maximum top value
    const maxTop = svg.height.baseVal.value - bbox.height;
    switch (border) {
    case U.Border.Left:
        elt.x.baseVal.value = 0;
        elt.y.baseVal.value = clamp(position, 0, maxTop);
        break;
    case U.Border.Right:
        elt.x.baseVal.value = maxLeft;
        elt.y.baseVal.value = clamp(position, 0, maxTop);
        break;
    case U.Border.Top:
        elt.x.baseVal.value = clamp(position, 0, maxLeft);
        elt.y.baseVal.value = 0;
        break;
    case U.Border.Bottom:
        elt.x.baseVal.value = clamp(position, 0, maxLeft);
        elt.y.baseVal.value = maxTop;
        break;
    }
    // Populate initial transform attribute.
    const rotate = calculatePortRotate(portType, border);
    const centerX = elt.x.baseVal.value + bbox.width / 2;
    const centerY = elt.y.baseVal.value + bbox.height / 2;
    elt.setAttributeNS('', 'transform', rotateString(rotate, centerX, centerY));
}

/**
 * Represents a port on a service.
 */
class Port {
    // Channels connected to this port.
    #channels:Channel[] = [];

    /**
     * Create a new port.
     */
    constructor(sys:Service, svg:SVGSVGElement, p:U.NewPort) {
        const portType = p.mode;
        const elt = document.createElementNS(svgns, 'use') as SVGUseElement;
        const inPort = p.mode == U.PortType.InPort;

        elt.setAttributeNS('', 'href', '#inPort');
        svg.appendChild(elt);
        var bbox = elt.getBBox();

        setPortElementPosition(svg, elt, bbox, portType, p.border, p.position);


        addSVGDragHandlers(svg, elt, (evt:DragEvent) => {
            // Maximum left value
            const maxLeft = svg.width.baseVal.value - bbox.width;
            // Maximum top value
            const maxTop = svg.height.baseVal.value - bbox.height;
            // Calculate x distance above or below max.
            let xDist = outsideRangeDist(evt.left, 0, maxLeft);
            // Calculate y distance above or below max.
            let yDist = outsideRangeDist(evt.top, 0, maxTop);

            const distLeft   = euclid2Dist(Math.abs(evt.left),           yDist);
            const distRight  = euclid2Dist(Math.abs(maxLeft - evt.left), yDist);
            const distTop    = euclid2Dist(Math.abs(evt.top),            xDist);
            const distBottom = euclid2Dist(Math.abs(maxTop  - evt.top),  xDist);
            const distMin = Math.min(distLeft, distTop, distRight, distBottom);

            let border:U.Border;
            let position:number;
            if (distLeft == distMin) {
                border = U.Border.Left;
                position = evt.top;
            } else if (distRight == distMin) {
                border = U.Border.Right;
                position = evt.top;
            } else if (distTop == distMin) {
                border = U.Border.Top;
                position = evt.left;
            } else {
                border = U.Border.Bottom;
                position = evt.left;
            }
            setPortElementPosition(svg, elt, bbox, portType, border, position);
        });
    }

    addChannel(c:Channel) {
        this.#channels.push(c);
     }

}

class Channel {
    update() {
    }
}

const panel = (document.getElementById('panel') as unknown) as SVGSVGElement;

const sys = new System(panel);

window.addEventListener('message', event => {
    const message = event.data as U.ModelUpdate; // The json data that the extension sent
    switch (message.tag) {
    case U.Tag.NewService:
        sys.newService(message as U.NewService);
        break;
    case U.Tag.NewPort:
        sys.newPort(message as U.NewPort);
        break;
    default:
        logEvent('Unknown command: ' + message.tag);
    }
});