import { common, extension, webview } from "../shared/webviewProtocol.js"
import * as A from "../shared/architecture.js"

import { ActorView, XRange, YRange, Rect, SystemServices, ChangeSet, TrackedIndex } from "./actor.js"
import * as svg from "./svg.js"

function svgSetLength(e:SVGAnimatedLength, l:A.Length) {
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
    #svg:SVGSVGElement
    #actors:ActorView[] = []

    constructor(thisSVG:SVGSVGElement) {
       this.#svg = thisSVG
       thisSVG.preserveAspectRatio.baseVal.align = thisSVG.preserveAspectRatio.baseVal.SVG_PRESERVEASPECTRATIO_XMINYMIN
    }

    setSystemModel(m:A.SystemModel):void {
        console.log('setSystemModel')
        // Dispose existing components
        for (const a of this.#actors) a.dispose()
        this.#actors = []

        const thisSVG = this.#svg

        svgSetLength(thisSVG.width, m.pagewidth)
        svgSetLength(thisSVG.height, m.pageheight)
        thisSVG.height.baseVal.convertToSpecifiedUnits(thisSVG.width.baseVal.unitType)

        thisSVG.viewBox.baseVal.width  = m.width
        thisSVG.viewBox.baseVal.height = m.width * (thisSVG.height.baseVal.value / thisSVG.width.baseVal.value)

        // Add new actors
        for (const a of m.actors) {
            var av = new ActorView(this, this.#svg, a)
            this.#actors.push(av)
        }
    }

    adjustX(thisActor:ActorView, r:YRange, width:number, oldLeft:number, newLeft:number):number {
        const l = this.#actors
        const top = r.top
        const bottom = top + r.height
        // If we move left
        if (newLeft < oldLeft) {
            for (const o of l) {
               if (o === thisActor) continue
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
            for (const o of l) {
                if (o === thisActor) continue
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

    adjustY(thisActor:ActorView, r:XRange, height:number, oldTop:number, newTop:number):number {
        const l = this.#actors
        const left = r.left
        const right = left + r.width
        // If we move up
        if (newTop < oldTop) {
            for (const o of l) {
                if (o === thisActor) continue
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
            for (const o of l) {
                if (o === thisActor) continue
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


    overlaps(thisActor:ActorView, r:Rect):boolean {
       const l = this.#actors
       for (const o of l) {
          if (o === thisActor) continue
          if (r.right <= o.left) continue
          if (o.right <= r.left) continue
          if (r.bottom <= o.top) continue
          if (o.bottom <= r.top) continue
          return true
       }
       return false
    }

    sendToExtension(r: webview.Event):void {
        vscode.postMessage(r)
    }

    sendUpdateDoc(s: ChangeSet) {
        if (s.edits.length > 0) {
            let msg:webview.UpdateDocument = {tag: webview.Tag.UpdateDocument, edits: s.edits }
            this.sendToExtension(msg)
        }
    }

    #trackListeners:Map<TrackedIndex, (newValue:string) => void> = new Map()

    whenStringTrackChanged(idx:TrackedIndex, listener:(newValue:string) => void):void {
        this.#trackListeners.set(idx, listener)
    }

    whenIntTrackChanged(idx:TrackedIndex, listener:(newValue:number) => void):void {
        this.#trackListeners.set(idx, (newValue) => {
            let i = parseInt(newValue)
            if (i === NaN) {
                console.log('Received ' + newValue + ' when int expected.')
                return
            }
            listener(i)
        })
    }

    documentEdited(edits:readonly common.TrackUpdate[]) {
        for (const e of edits) {
            const listener = this.#trackListeners.get(e.trackIndex)
            if (listener)
                listener(e.newText)
        }
    }
}

declare var acquireVsCodeApi: any

const vscode = acquireVsCodeApi()

const sys = new Webview((document.getElementById('panel') as unknown) as SVGSVGElement)

console.log('Start loop')

window.addEventListener('message', event => {
    const message = event.data as extension.Event // The json data that the extension sent
    switch (message.tag) {
    case extension.Tag.SetSystemModel:
        sys.setSystemModel(message.system)
        let upd:webview.SetSystemModelDone = {tag: webview.Tag.SetSystemModelDone}
        sys.sendToExtension(upd)
        break
    case extension.Tag.DocumentEdited:
        sys.documentEdited(message.edits)
        break
    }
})