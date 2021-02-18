import * as A from '../shared/architecture.js'
import { common, extension, webview } from '../shared/webviewProtocol.js'

import { ChangeSet, TrackedIndex } from './changeSet.js'
import { Rectangle, XRange, YRange } from './geometry.js'
import * as svg from './svg.js'
import { SystemServices } from './systemServices.js'
import { ActorView } from './views/actor.js'
import { BusView } from './views/bus.js'
import { Connectable, ConnectionView } from './views/connection.js'
import { PortView } from './views/port.js'

type TrackedValueListener = (newValue: string) => void

function svgSetLength(e: SVGAnimatedLength, l: A.Length) {
    switch (l.units) {
        case A.Units.CM:
            svg.setCmUnits(e, l.value)
            break
        case A.Units.IN:
            svg.setInchUnits(e, l.value)
            break
    }
}

/**
 *  Class that manages webview state.
 */
class Webview implements SystemServices {
    // Container for SVG
    #svg: SVGSVGElement

    #actors: ActorView[] = []
    #buses: BusView[] = []
    #connections: ConnectionView[] = []

    #errormsgDiv = document.getElementById('lasterrormsg') as HTMLDivElement

    constructor(thisSVG: SVGSVGElement) {
        this.#svg = thisSVG
        thisSVG.preserveAspectRatio.baseVal.align = thisSVG.preserveAspectRatio.baseVal.SVG_PRESERVEASPECTRATIO_XMINYMIN
    }

    setSystemModel(m: A.SystemModel): void {
        console.log('setSystemModel')
        // Dispose existing components
        this.clearModel('')

        const thisSVG = this.#svg

        svgSetLength(thisSVG.width, m.pagewidth)
        svgSetLength(thisSVG.height, m.pageheight)
        thisSVG.height.baseVal.convertToSpecifiedUnits(thisSVG.width.baseVal.unitType)

        thisSVG.viewBox.baseVal.width = m.width
        thisSVG.viewBox.baseVal.height = m.width * (thisSVG.height.baseVal.value / thisSVG.width.baseVal.value)

        // Add new actors
        for (const a of m.actors) {
            const av = new ActorView(this, this.#svg, a)
            this.#actors.push(av)
        }

        // Add new buses
        for (const b of m.buses) {
            const bv = new BusView(this, this.#svg, b)
            this.#buses.push(bv)
        }

        // Add connections (must be after actors and buses)
        for (const c of m.connections) {
            const connectables = this.findConnectables(
                c,
                this.#actors,
                this.#buses,
            )
            if (!connectables) { continue }
            const source = connectables.source
            const target = connectables.target
            const cv = new ConnectionView(this, this.#svg, source, target)
            this.#connections.push(cv)
        }
    }

    getConnectablePosition(c: Connectable): number {
        if (c instanceof BusView) {
            return c.draggableRectangle.left
        }
        if (c instanceof PortView) {
            return c.offset
        }
        console.log('Expected an instance of BusView of PortView, please report.')
        return 0
    }

    clearModel(errorMsgText: string): void {
        for (const a of this.#actors) { a.dispose() }
        this.#actors = []
        if (this.#errormsgDiv) { this.#errormsgDiv.innerText = errorMsgText }

    }

    getCollidableObjects(): readonly Rectangle<number>[] {
        return ([] as readonly Rectangle<number>[]).concat(
            this.#actors.map(a => a.draggableRectangle),
            this.#buses.map(b => b.draggableRectangle),
        )
    }

    findEndpoint(
        endpoint: A.Endpoint,
        actors: ReadonlyArray<ActorView>,
        buses: ReadonlyArray<BusView>,
    ): Connectable | undefined {
        switch (endpoint.type) {
            case A.EndpointType.Port: {
                const actor = actors.find(a => a.name === endpoint.actor)
                if (!actor) { return }
                return actor.allPorts.find(p => p.name === endpoint.port)
            }
            case A.EndpointType.Bus: {
                return buses.find(b => b.name === endpoint.bus)
            }
        }
    }

    findConnectables(
        connection: A.Connection,
        actors: ReadonlyArray<ActorView>,
        buses: ReadonlyArray<BusView>,
    ): { source: Connectable, target: Connectable } | undefined {
        const source = this.findEndpoint(connection.source, actors, buses)
        if (!source) { return }
        const target = this.findEndpoint(connection.target, actors, buses)
        if (!target) { return }
        return { source, target }
    }

    adjustX(thisObject: any, r: YRange<number>, width: number, oldLeft: number, newLeft: number): number {
        const collidables = this.getCollidableObjects()

        const top = r.top
        const bottom = top + r.height
        // If we move left
        if (newLeft < oldLeft) {
            for (const collidable of collidables) {
                if (collidable === thisObject) continue
                if (bottom <= collidable.top) continue
                if (collidable.bottom <= top) continue
                // If o is left of oldLeft and right of newLeft
                if (collidable.right <= oldLeft) { newLeft = Math.max(collidable.right, newLeft) }
            }
        }
        // If we move right
        if (newLeft > oldLeft) {
            const oldRight = oldLeft + width
            let newRight = newLeft + width
            for (const collidable of collidables) {
                if (collidable === thisObject) continue
                if (bottom <= collidable.top) continue
                if (collidable.bottom <= top) continue
                // If o is right of oldRight and left of newRight.
                if (oldRight <= collidable.left) { newRight = Math.min(newRight, collidable.left) }
            }
            newLeft = newRight - width
        }
        return newLeft
    }

    adjustY(thisActor: unknown, r: XRange<number>, height: number, oldTop: number, newTop: number): number {
        const l = this.getCollidableObjects()
        const left = r.left
        const right = left + r.width
        // If we move up
        if (newTop < oldTop) {
            for (const o of l) {
                if (o === thisActor) continue
                if (right <= o.left) continue
                if (o.right <= left) continue
                // If o is above of oldTop
                if (o.bottom <= oldTop) { newTop = Math.max(o.bottom, newTop) }
            }
        }
        // If we move right
        if (newTop > oldTop) {
            const oldBottom = oldTop + height
            let newBottom = newTop + height
            for (const o of l) {
                if (o === thisActor) continue
                if (right <= o.left) continue
                if (o.right <= left) continue
                // If o is right of oldRight and left of newRight.
                if (oldBottom <= o.top) { newBottom = Math.min(newBottom, o.top) }

            }
            newTop = newBottom - height
        }
        return newTop
    }

    overlaps(thisObject: unknown, r: Rectangle<number>): boolean {
        const collidables = this.getCollidableObjects()
        for (const collidable of collidables) {
            if (collidable === thisObject) continue
            if (r.right <= collidable.left) continue
            if (collidable.right <= r.left) continue
            if (r.bottom <= collidable.top) continue
            if (collidable.bottom <= r.top) continue
            return true
        }
        return false
    }

    private sendToExtension(r: webview.Event): void {
        vscode.postMessage(r)
    }

    visitURI(idx: A.LocationIndex): void {
        const cmd: webview.VisitURI = {
            tag: webview.Tag.VisitURI,
            locationIdx: idx,
        }
        this.sendToExtension(cmd)
    }

    setSystemModelDone(): void {
        const upd: webview.SetSystemModelDone = { tag: webview.Tag.SetSystemModelDone }
        this.sendToExtension(upd)
    }

    sendUpdateDoc(s: ChangeSet) {
        if (s.edits.length > 0) {
            const msg: webview.UpdateDocument = {
                tag: webview.Tag.UpdateDocument,
                edits: s.edits,
            }
            this.sendToExtension(msg)
        }
    }

    #trackListeners: Map<TrackedIndex, TrackedValueListener[]> = new Map()

    /* Returns the array of listeners for that index. This is **not** a copy, so
     * one can mutate it to change the listeners. */
    getListeners(idx: TrackedIndex): TrackedValueListener[] {
        let listeners = this.#trackListeners.get(idx)
        if (! listeners) {
            listeners = []
            this.#trackListeners.set(idx, listeners)
        }
        return listeners
    }

    whenTrackedChanged<V>(
        idx: TrackedIndex,
        parser: (newValue: string) => V | undefined,
        listener: (newValue: V) => void,
    ): void {
        const listeners = this.getListeners(idx)
        const handler = (newValue: string) => {
            const parsed = parser(newValue)
            if (parsed) { listener(parsed) }
        }
        listeners.push(handler)
    }

    whenStringTrackChanged(idx: TrackedIndex, listener: (newValue: string) => void): void {
        this.whenTrackedChanged(idx, s => s, listener)
    }

    whenIntTrackChanged(idx: TrackedIndex, listener: (newValue: number) => void): void {
        this.whenTrackedChanged(
            idx,
            (newValue) => {
                const i = parseInt(newValue)
                if (isNaN(i)) {
                    console.log(`Received ${newValue} when int expected.`)
                    return
                }
                return i
            },
            listener
        )
    }

    documentEdited(edits: readonly common.TrackUpdate[]) {
        for (const e of edits) {
            const listeners = this.getListeners(e.trackIndex)
            for (const listener of listeners) {
                listener(e.newText)
            }
        }
    }
}

declare const acquireVsCodeApi: any

const vscode = acquireVsCodeApi()

{
    const svg = (document.getElementById('panel') as unknown) as SVGSVGElement
    if (svg === null) {
        console.log('SVG panel is missing')
    } else {
        const sys = new Webview(svg)

        console.log('Start loop')

        window.addEventListener('message', event => {
            const message = event.data as extension.Event // The json data that the extension sent
            switch (message.tag) {
                case extension.Tag.SetSystemModel:
                    sys.setSystemModel(message.system)
                    sys.setSystemModelDone()
                    break
                case extension.Tag.InvalidateModel:
                    sys.clearModel('Invalid model.')
                    sys.setSystemModelDone()
                    break
                case extension.Tag.DocumentEdited:
                    sys.documentEdited(message.edits)
                    break
            }
        })
        vscode.postMessage({tag:  webview.Tag.Ready})
    }
}
