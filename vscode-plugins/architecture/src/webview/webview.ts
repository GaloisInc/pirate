import { extension, webview } from "../shared/webviewProtocol.js"
import * as A from "../shared/architecture.js"

import { ActorView, XRange, YRange, Rect, SystemServices, ChangeSet } from "./actor.js"
import * as svg from "./svg.js"

declare var acquireVsCodeApi: any

const vscode = acquireVsCodeApi()

const lasterrormsg = document.getElementById('lasterrormsg') as HTMLDivElement

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

class System implements SystemServices {
    // Container for SVG
    #svg:SVGSVGElement
    #actors:Map<string, ActorView> = new Map()
    constructor(thisSVG:SVGSVGElement) {
       this.#svg = thisSVG
       thisSVG.preserveAspectRatio.baseVal.align = thisSVG.preserveAspectRatio.baseVal.SVG_PRESERVEASPECTRATIO_XMINYMIN
    }

    setSystemLayout(m:A.SystemLayout):void {
        // Dispose existing components
        this.#actors.forEach((a,nm,m) => a.dispose())
        const thisSVG = this.#svg

        svgSetLength(thisSVG.width, m.pagewidth)
        svgSetLength(thisSVG.height, m.pageheight)
        thisSVG.height.baseVal.convertToSpecifiedUnits(thisSVG.width.baseVal.unitType)

        thisSVG.viewBox.baseVal.width  = m.width.value
        thisSVG.viewBox.baseVal.height = m.width.value * (thisSVG.height.baseVal.value / thisSVG.width.baseVal.value)
        
        // Add new actors
        for (const a of m.actors) {
            var av = new ActorView(this, this.#svg, a)
            this.#actors.set(a.name.value, av)
        }
    }

    adjustX(thisActor:string, r:YRange, width:number, oldLeft:number, newLeft:number):number {
        const l = this.#actors
        const top = r.top
        const bottom = top + r.height
        // If we move left
        if (newLeft < oldLeft) {
            for (const nm of l.keys()) {
               if (nm === thisActor) continue 
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
                if (nm === thisActor) continue 
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

    adjustY(thisActor:string, r:XRange, height:number, oldTop:number, newTop:number):number {
        const l = this.#actors
        const left = r.left
        const right = left + r.width
        // If we move up
        if (newTop < oldTop) {
            for (const nm of l.keys()) {
                if (nm === thisActor) continue 
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
                if (nm === thisActor) continue 
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


    overlaps(thisActor:string, r:Rect):boolean {
       const l = this.#actors
       for (const nm of l.keys()) {
          if (nm === thisActor) continue 
          const o = l.get(nm) as ActorView
          if (r.right <= o.left) continue 
          if (o.right <= r.left) continue 
          if (r.bottom <= o.top) continue 
          if (o.bottom <= r.top) continue 
          return true
       }
       return false
    }

    sendAsyncRequest(r: webview.Event):void {
        vscode.postMessage(r)
    }
    sendUpdateDoc(s: ChangeSet) {
        if (s.changes.length > 0)
            this.sendAsyncRequest({tag: webview.Tag.UpdateDoc, changes: s.changes })
    }
}

function logEvent(msg:string) {
    lasterrormsg.innerHTML = msg
}

const sys = new System((document.getElementById('panel') as unknown) as SVGSVGElement)

window.addEventListener('message', event => {
    const message = event.data as extension.Event // The json data that the extension sent
    switch (message.tag) {
    case extension.Tag.SetSystemLayout:
        sys.setSystemLayout(message.system)
        sys.sendAsyncRequest({tag: webview.Tag.SetSystemLayoutDone})
        break
    }
})