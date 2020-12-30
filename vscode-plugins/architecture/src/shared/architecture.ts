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

export interface StringField {
    value: string
}

/**
 * A value whose source location is tracked so we can
 * efficiently update the underlying document when it changes.
 */
export interface TrackedValue<T> {
    readonly trackId: number
    readonly value: T
}

export interface Port {
    readonly name: StringField
    readonly location: SourceLocation
    readonly border: TrackedValue<Border>
    readonly offset: TrackedValue<number>
}

export interface Actor {
    readonly name: StringField
    readonly location: SourceLocation
    readonly left: TrackedValue<number>
    readonly top: TrackedValue<number>
    readonly width: TrackedValue<number>
    readonly height: TrackedValue<number>
    readonly color: StringField
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
    readonly name: StringField
    readonly orientation: BusOrientation
    readonly left:   number
    readonly top:    number
    readonly height: number
    readonly width:  number
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

export const enum Units {
    IN = "in",
    CM = "cm"
}

/**
 * Length with units
 */
export interface Length {
    readonly value: number
    readonly units: Units
}

export interface SystemModel {
    readonly pagewidth: Length
    readonly pageheight: Length
    readonly width: number
    readonly actors: Actor[]
    readonly buses: Bus[]
    readonly connections: Connection[]
}