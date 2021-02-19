import * as A from '../../shared/architecture.js'
import { DraggableRectangle } from '../draggableRectangle.js'
import { SystemServices } from '../systemServices.js'

export class BusView {
    readonly draggableRectangle: DraggableRectangle
    readonly name: string
    readonly orientation: A.BusOrientation

    constructor(sys: SystemServices, parentSVG: SVGSVGElement, b: A.Bus) {

        this.name = b.name.value
        this.orientation = b.orientation

        this.draggableRectangle = new DraggableRectangle(
            sys,
            parentSVG,
            {
                left: b.left,
                top: b.top,
                width: b.width,
                height: b.height,
            }
        )

        this.draggableRectangle.rect.style.fill = 'red'

        const div = document.createElement('div')
        const enclaveName = document.createElement('span') as HTMLSpanElement
        enclaveName.classList.add('enclave-name')
        enclaveName.innerHTML = b.name.value

        div.appendChild(enclaveName)
        div.onpointerdown = e => { e.stopImmediatePropagation() }

        this.draggableRectangle.contentObject.append(div)
    }

    /** Remove all components from SVG */
    dispose(): void {
        this.draggableRectangle.dispose()
    }

    // get left(): number { return this.#svgContainer.x.baseVal.value }
    // get top(): number { return this.#svgContainer.y.baseVal.value }
    // get width(): number { return this.#svgContainer.width.baseVal.value }
    // get height(): number { return this.#svgContainer.height.baseVal.value }
    // get right(): number { return this.left + this.width }
    // get bottom(): number { return this.top + this.height }
}
