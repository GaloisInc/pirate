/**
 * Interface needed for addSVGDragHandlers to make an element dragable.
 */
export
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
 * Event called when dragging a SVG element.
 */
export
interface SVGDragEvent {
    /**
     * Top coordinate in SVG address space.
     */
    readonly top: number;
    /**
     * Left coordinate to drag to in SVG address space.
     */
    readonly left: number;
}

export
function addSVGDragHandlers(container:SVGSVGElement, innerSVG:DragableSVGElement, drag:(p:SVGDragEvent) => void) {
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