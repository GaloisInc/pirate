/**
 * @module
 * Actor view, responsible for displaying itself and its ports.
 */

import * as A from '../../shared/architecture.js'
import { DraggableRectangle } from '../draggableRectangle.js'
import { SystemServices } from '../systemServices.js'

import { PortDir, PortView } from './port.js'

export class ActorView {
    readonly draggableRectangle: DraggableRectangle
    readonly name: string
    readonly inPorts: PortView[]
    readonly outPorts: PortView[]

    get allPorts(): readonly PortView[] {
        return ([] as PortView[]).concat(
            this.inPorts,
            this.outPorts,
        )
    }

    constructor(sys: SystemServices, parentSVG: SVGSVGElement, a: A.Actor) {

        this.name = a.name.value

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

        this.draggableRectangle.rect.style.fill = a.color.value

        const div = document.createElement('div')
        const enclaveName = document.createElement('div') as HTMLSpanElement
        enclaveName.classList.add('enclave-name')
        enclaveName.innerHTML = a.name.value

        const visitClass = document.createElement('a')
        visitClass.classList.add('enclave-visit-class')
        visitClass.innerHTML = '[Visit Class]'
        const loc = a.location
        visitClass.onclick = () => sys.visitURI(loc)

        div.appendChild(enclaveName)
        div.appendChild(visitClass)
        div.onpointerdown = e => { e.stopImmediatePropagation() }

        this.draggableRectangle.contentObject.appendChild(div)

        this.inPorts = a.inPorts.map(p =>
            new PortView(sys, this, PortDir.In, p)
        )
        this.outPorts = a.outPorts.map(p =>
            new PortView(sys, this, PortDir.Out, p)
        )
    }

    /** Remove all components from SVG */
    dispose(): void {
        this.draggableRectangle.dispose()
    }

}
