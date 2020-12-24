import { TextRange } from "./position.js"

export interface TextLocated<T> extends TextRange {
    value: T
}

export interface SourceLocation {
    readonly filename: string
    readonly line: number
    readonly column: number
}

export const enum Border {
    Left = 'left',
    Top = 'top',
    Right = 'right',
    Bottom = 'bottom'
}

export interface Port {
    readonly name: TextLocated<string>
    readonly location: TextLocated<SourceLocation>
    readonly border: TextLocated<Border>
    readonly offset: TextLocated<number>
}

export interface Actor {
    readonly name: TextLocated<string>
    readonly location: TextLocated<SourceLocation>
    readonly left: TextLocated<number>
    readonly top: TextLocated<number>
    readonly width: TextLocated<number>
    readonly height: TextLocated<number>
    readonly color: TextLocated<string>
    readonly inPorts: Port[]
    readonly outPorts: Port[]
}

/**
 * Orientation of bus
 */
export const enum BusOrientation {
    Horizontal = 'horizontal',
    Vertical = 'vertical'
}

/**
 * A bus connects one or more input ports to one or more output ports.
 * 
 * All messages sent on any input ports are forwarded to output ports.
 */
export interface Bus {
    readonly name: TextLocated<string>
    readonly orientation: TextLocated<BusOrientation>
    readonly left:   TextLocated<number>
    readonly top:    TextLocated<number>
    readonly height: TextLocated<number>
    readonly width:  TextLocated<number>
}

export const enum EndpointType { Port = 'port', Bus = 'bus' }

/** Identifies a port by actor name and port */
export interface PortId {
    readonly type: EndpointType.Port
    readonly actor: string
    readonly port: string
}

export interface BusId {
    readonly type: EndpointType.Bus
    readonly bus: string
}

export type Endpoint = PortId | BusId

export interface Connection {
    readonly source : Endpoint
    readonly target : Endpoint
}

export interface SystemLayout {
    readonly pagewidth: TextLocated<string>
    readonly pageheight: TextLocated<string>
    readonly width: TextLocated<number>
    readonly height: TextLocated<number>
    readonly actors: Actor[]
    readonly buses: Bus[]
    readonly connections: Connection[]
}