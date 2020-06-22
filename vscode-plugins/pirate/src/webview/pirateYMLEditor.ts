import * as U from "../shared/modelUpdates";
//import type * as R from "../shared/viewrequests";
import * as R from "../shared/viewRequests";

declare var acquireVsCodeApi: any;

const vscode = acquireVsCodeApi();

const svgns = "http://www.w3.org/2000/svg";

class System {
    #svg:SVGSVGElement;
    #services:Map<string, Service> = new Map();
    constructor(svg:SVGSVGElement) {
       this.#svg = svg;
    }

    newService(u:U.NewService) {
        var s = new Service(this, this.#svg, u);
        this.#services.set(u.name, s);
        return s;
    }

    newPort(u:U.NewPort) {
    }

    /**
     * Return true if @r@ does not overlap with any service other than `thisService`.
     */
    noOverlap(thisService:string, r:any) {
       const l = this.#services;
       for (var nm in l) {
          if (nm === thisService) { continue; }
          var o = l.get(nm) as Service;
          if (r.right < o.left) { continue; }
          if (o.right < r.left) { continue; }
          if (r.bottom < o.top) { continue; }
          if (o.bottom < r.top) { continue; }
          return false;
       }
       return true;
    }
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

class Service {
    // Channels that start from this enclave.
    #srcChannels:Channel[] = [];
    #tgtChannels:Channel[] = [];
    #coords:Coords;
    constructor(sys:System, svg:SVGSVGElement, p:U.NewService) {
        const coords = new Coords(p.coords);
        // Offset for content
        const offset = { x : 20, y: 20 };
        this.#coords = coords;
        const rect          = document.createElementNS(svgns, 'rect') as SVGRectElement;
        const contentObject = document.createElementNS(svgns, 'foreignObject') as SVGForeignObjectElement;

        var dragOffset:any = null;
        var srcChannels = this.#srcChannels;
        var tgtChannels = this.#tgtChannels;
        rect.classList.add('enclave');

        function startDrag(evt:PointerEvent) {
            dragOffset = {
                rx: rect.x.baseVal.value,
                ry: rect.y.baseVal.value,
                px: evt.pageX,
                py: evt.pageY
            };
            rect.setPointerCapture(evt.pointerId);
        }

        function drag(evt:PointerEvent) {
            if (dragOffset) {
                evt.preventDefault();
                var CTM = svg.getScreenCTM() as DOMMatrix;
                var left = dragOffset.rx + (evt.pageX - dragOffset.px) / CTM.a;
                var top  = dragOffset.ry + (evt.pageY - dragOffset.py) / CTM.d;
                var r = { left:   left,
                          top:    top,
                          right:  left + coords.width,
                          bottom: top  + coords.height
                        };
                if (sys.noOverlap(p.name, r)) {
                   coords.top = top;
                   coords.left = left;
                   rect.x.baseVal.value = left;
                   rect.y.baseVal.value = top;
                   contentObject.x.baseVal.value = left + offset.x;
                   contentObject.y.baseVal.value = top  + offset.y;
                   srcChannels.forEach(chan => chan.update());
                   tgtChannels.forEach(chan => chan.update());
                }
             }
        };
        function endDrag(evt:PointerEvent) {
            dragOffset = null;
            rect.releasePointerCapture(evt.pointerId);
        };

        rect.x.baseVal.value = coords.left;
        rect.y.baseVal.value = coords.top;
        rect.width.baseVal.value = coords.width;
        rect.height.baseVal.value = coords.height;
        rect.onpointerdown = startDrag;
        rect.onpointermove = drag;
        rect.onpointerup = endDrag;

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
        div.onpointerdown = e => { e.stopPropagation(); };

        contentObject.x.baseVal.value = coords.left + offset.x;
        contentObject.y.baseVal.value = coords.top  + offset.y;
        contentObject.width.baseVal.value = coords.width - 40;
        contentObject.height.baseVal.value = coords.height - 40;
        contentObject.onpointerdown = startDrag;
        contentObject.onpointermove = drag;
        contentObject.onpointerup = endDrag;
        contentObject.appendChild(div);

        svg.appendChild(rect);
        svg.appendChild(contentObject);
     }
     get top()    { return this.#coords.top; }
     get left()   { return this.#coords.left; }
     get height() { return this.#coords.height; }
     get width()  { return this.#coords.width; }
     get right()  { return this.#coords.left + this.#coords.width; }
     get bottom() { return this.#coords.top + this.#coords.height; }
     // Return coordinates in center.
     get center() {
        var coords = this.#coords;
        return { x: coords.left + coords.width / 2,
                 y: coords.top + coords.height / 2
               };
     }
     addSrcChannel(c:Channel) {
        this.#srcChannels.push(c);
     }
     addTgtChannel(c:Channel) {
        this.#tgtChannels.push(c);
     }
}

class Channel {
    update() {
    }
}

const panel = (document.getElementById('panel') as unknown) as SVGSVGElement;

const sys = new System(panel);

const lasterrormsg = document.getElementById('lasterrormsg') as HTMLDivElement;

window.addEventListener('message', event => {
    const message = event.data as U.ModelUpdate; // The json data that the extension sent
    switch (message.tag) {
    case U.Tag.NewService:
        sys.newService(message as U.NewService);
        break;
    case U.Tag.NewPort:
        sys.newPort(message as U.NewPort);
    default:
        lasterrormsg.innerHTML = 'Unknown command: ' + message.tag;
        console.log('Unknown command: ' + message.tag);
    }
});