/**
 * Sets up the webview component of the plug-in, and gives the implementation
 * of the system services.
 */

import { Context, Spec } from 'immutability-helper'
import * as React from 'react'
import * as ReactDOM from 'react-dom'

import * as A from '@shared/architecture'
import { common, extension, webview } from '@shared/webviewProtocol'

import { ChangeSet, TrackedIndex } from './changeSet'
import { Rectangle, XRange, XYRange, YRange } from './geometry'
import * as svg from './svg'
import { Actor } from './views/actor'
import { Bus } from './views/bus'
import { Connectable, Connection } from './views/connection'
import { getPortLocalX, getPortLocalY, portDimensions } from './views/port'


const update = (new Context()).update

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

interface State {
    actors: A.TrackedValue<A.Actor>[]
    buses: A.TrackedValue<A.Bus>[]
    connections: A.Connection[]
}

function getXYRange(o: XYRange<A.TrackedValue<number>>): XYRange<number> {
    return {
        height: o.height.value,
        left: o.left.value,
        top: o.top.value,
        width: o.width.value,
    }
}

function WebviewComponent(props: {
    sys: Webview,
}): JSX.Element {

    // WARNING: if you modify the state, make sure to update the state update
    // See the other occurrences of #state-update in this file
    const [actors, setActors] = React.useState<A.TrackedValue<A.Actor>[]>([])
    const [buses, setBuses] = React.useState<A.TrackedValue<A.Bus>[]>([])
    const [connections, setConnections] = React.useState<A.Connection[]>([])

    const svgRef: React.RefObject<SVGSVGElement> = React.createRef()

    // this is just a wrapper to be used when we want to update the whole state
    // guaranteeing we don't forget to update certain fields in different parts
    // of the code, when we inevitable change the code
    const fullStateUpdate = (state: {
        actors: A.TrackedValue<A.Actor>[],
        buses: A.TrackedValue<A.Bus>[],
        connections: A.Connection[],
    }) => {
        // #state-update
        setActors(state.actors)
        setBuses(state.buses)
        setConnections(state.connections)
    }

    // Listen to messages from the extension backend
    React.useEffect(() => {

        const onMessage = (event: MessageEvent) => {
            // The JSON data that the extension sent
            const message = event.data as extension.Event
            // console.log(message)

            switch (message.tag) {
                case extension.Tag.SetSystemModel: {
                    console.log('SetSystemModel')

                    const sys = message.system

                    // TODO: make this part of the state
                    const svg = svgRef.current
                    if (svg) {
                        svgSetLength(svg.width, sys.pagewidth)
                        svgSetLength(svg.height, sys.pageheight)
                        svg.height.baseVal.convertToSpecifiedUnits(svg.width.baseVal.unitType)
                    }

                    fullStateUpdate(message.system)

                    setSystemModelDone()
                    break
                }
                case extension.Tag.InvalidateModel: {
                    console.log('InvalidateModel')

                    fullStateUpdate({
                        actors: [],
                        buses: [],
                        connections: [],
                    })

                    setSystemModelDone()
                    break
                }
                case extension.Tag.DocumentEdited: {
                    props.sys.documentEdited(message.edits)
                    break
                }
            }
        }

        window.addEventListener('message', onMessage)

        return () => {
            window.removeEventListener('message', onMessage)
        }

    })

    const broadcastUpdate =
        (
            trackId: A.TrackIndex,
            value: number | string,
        ) => {
            const changes = new ChangeSet()
            changes.replace(trackId, value)
            sendUpdateDoc(changes)
        }

    /**
     * Often times we want to update our state and also broadcast the change.
     * Note that in general we cannot just broadcast the update and wait until
     * we hear the change back from the backend, as it makes the UI laggy. This
     * is why the webview updates its own state while broadcasting the update,
     * and the backend only sends the broadcasted updates to the **other**
     * views.
     * @param setTrackedObject - React's 'setter' for the state field of choice (currently 'setActors' or 'setBuses')
     * @param trackedObject - Object containing the field to track
     * @param index - Index of 'trackedObject' within its respective array in the system architecture
     * @param fieldKey - Which field of 'trackedObject' to track
     */
    const updateStateAndBroadcast =
        <
            ModelKey extends keyof SubModel & keyof State,
            SubModel extends State & Record<ModelKey, TrackedObjectType[]>,
            ValueType extends number | string,
            FieldKey extends keyof TrackedObjectType['value'],
            TrackedFieldType extends A.TrackedValue<ValueType>,
            TrackedObjectType extends A.TrackedValue<Record<FieldKey, TrackedFieldType>>
        >(
            trackedObject: TrackedObjectType,
            index: number,
            setTrackedObject: React.Dispatch<React.SetStateAction<TrackedObjectType[]>>,
            fieldKey: FieldKey,
        ) =>
            (value: ValueType) => {
                setStateField(index, setTrackedObject, fieldKey)(value)
                broadcastUpdate(trackedObject.value[fieldKey].trackId, value)
            }

    /**
     * Set up a listener for changes to a number value coming from the
     * extension backend.
     * @param setTrackedObject - React's 'setter' for the state field of choice (currently 'setActors' or 'setBuses')
     * @param trackedObject - Object containing the field to track
     * @param index - Index of 'trackedObject' within its respective array in the system architecture
     * @param fieldKey - Which field of 'trackedObject' to track
     */
    const listenToTrackedInt =
        <
            FieldKey extends keyof TrackedObjectType['value'],
            TrackedFieldType extends A.TrackedValue<number>,
            TrackedObjectType extends A.TrackedValue<Record<FieldKey, TrackedFieldType>>,
            ModelKey extends keyof SubModel & keyof State,
            SubModel extends Record<ModelKey, TrackedObjectType[]>
        >
            (
                setTrackedObject: React.Dispatch<React.SetStateAction<TrackedObjectType[]>>,
                trackedObject: TrackedObjectType,
                index: number,
                fieldKey: FieldKey,
        ): void => {
            props.sys.whenIntTrackChanged(
                trackedObject.value[fieldKey].trackId,
                setStateField(index, setTrackedObject, fieldKey)
            )
        }

    const updateInPort =
        (portIndex: number, updatePort: Spec<A.Port, never>): Spec<A.Actor, never> => (
            { inPorts: { [portIndex]: { value: updatePort } } }
        )

    const updateOutPort =
        (portIndex: number, updatePort: Spec<A.Port, never>): Spec<A.Actor, never> => (
            { outPorts: { [portIndex]: { value: updatePort } } }
        )

    const updateActor = (actorIndex: number, updateField: Spec<A.Actor, never>) =>
        setActors((actors) => update(actors, { [actorIndex]: { value: updateField } }))

    const trackPortFields = (
        port: A.TrackedValue<A.Port>,
        updateThisPort: (spec: Spec<A.Port, never>) => void,
    ) => {
        props.sys.whenTrackedChanged(
            port.value.border.trackId,
            (border: string) => border as A.Border,
            (border: A.Border) => updateThisPort({ border: { value: { $set: border } } }),
        )
        props.sys.whenIntTrackChanged(
            port.value.location.trackId,
            (location: A.LocationIndex) => updateThisPort({ location: { value: { $set: location } } }),
        )
        props.sys.whenIntTrackChanged(
            port.value.offset.trackId,
            (offset: number) => updateThisPort({ offset: { value: { $set: offset } } }),
        )
    }

    // Register listeners for tracked actor fields
    actors.map((actor, actorIndex) => {

        listenToTrackedInt(setActors, actor, actorIndex, 'height')
        listenToTrackedInt(setActors, actor, actorIndex, 'left')
        listenToTrackedInt(setActors, actor, actorIndex, 'top')
        listenToTrackedInt(setActors, actor, actorIndex, 'width')

        actor.value.inPorts.map((port, portIndex) => {
            const updateThisPort = (spec: Spec<A.Port, never>) => updateActor(actorIndex, updateInPort(portIndex, spec))
            trackPortFields(port, updateThisPort)
        })

        actor.value.outPorts.map((port, portIndex) => {
            const updateThisPort = (spec: Spec<A.Port, never>) => updateActor(actorIndex, updateOutPort(portIndex, spec))
            trackPortFields(port, updateThisPort)
        })

    })

    // Register listeners for tracked bus fields
    buses.map((bus, index) => {
        listenToTrackedInt(setBuses, bus, index, 'height')
        listenToTrackedInt(setBuses, bus, index, 'left')
        listenToTrackedInt(setBuses, bus, index, 'top')
        listenToTrackedInt(setBuses, bus, index, 'width')
    })

    const actorsComponents = actors.map((trackedActor, index) => {
        const actor = trackedActor.value

        const setPortBorder = (whichPorts: 'inPorts' | 'outPorts') => (portIndex: number) => {
            return (border: A.Border) => {
                setActors((prevActors) => {
                    return update(prevActors, { [index]: { value: { [whichPorts]: { [portIndex]: { value: { border: { value: { $set: border } } } } } } } })
                })
            }
        }

        const setPortOffset = (whichPorts: 'inPorts' | 'outPorts') => (portIndex: number) => {
            return (offset: number) => {
                console.log(`Setting offset to ${offset}`)
                setActors((prevActors) => {
                    return update(prevActors, { [index]: { value: { [whichPorts]: { [portIndex]: { value: { offset: { value: { $set: offset } } } } } } } })
                })
            }
        }

        return (
            <Actor
                color={actor.color.value}
                height={actor.height.value}
                inPorts={actor.inPorts}
                left={actor.left.value}
                location={actor.location}
                name={actor.name.value}
                onVisitClassClick={visitURI}
                outPorts={actor.outPorts}
                key={actor.name.value}
                setMyHeight={updateStateAndBroadcast(trackedActor, index, setActors, 'height')}
                setMyInPortBorder={setPortBorder('inPorts')}
                setMyInPortOffset={setPortOffset('inPorts')}
                setMyOutPortBorder={setPortBorder('outPorts')}
                setMyOutPortOffset={setPortOffset('outPorts')}
                setMyLeft={updateStateAndBroadcast(trackedActor, index, setActors, 'left')}
                setMyTop={updateStateAndBroadcast(trackedActor, index, setActors, 'top')}
                setMyWidth={updateStateAndBroadcast(trackedActor, index, setActors, 'width')}
                top={actor.top.value}
                width={actor.width.value}
            />
        )
    })

    const busesComponents = buses.map((bus, index) =>
        <Bus
            height={bus.value.height.value}
            left={bus.value.left.value}
            key={bus.value.name.value}
            locatedBus={bus}
            setMyHeight={updateStateAndBroadcast(bus, index, setBuses, 'height')}
            setMyLeft={updateStateAndBroadcast(bus, index, setBuses, 'left')}
            setMyTop={updateStateAndBroadcast(bus, index, setBuses, 'top')}
            setMyWidth={updateStateAndBroadcast(bus, index, setBuses, 'width')}
            top={bus.value.top.value}
            width={bus.value.width.value}
        />
    )

    // FIXME: the logic in here is a bit gnarly, try to improve it
    const connectionsComponents = connections.flatMap(c => {
        const connectables = getConnectables({ actors, buses }, c)
        if (!connectables) return []

        let actor: A.Actor | undefined = undefined
        let bus: A.Bus | undefined = undefined
        let port: A.Port | undefined = undefined

        // Assumption: one endpoint is an actor port, the other a bus

        if (c.source.type === A.EndpointType.Port) {
            actor = (connectables.source as { actor: A.Actor }).actor
            port = (connectables.source as { port: A.Port }).port
        }
        if (c.target.type === A.EndpointType.Port) {
            actor = (connectables.target as { actor: A.Actor }).actor
            port = (connectables.target as { port: A.Port }).port
        }

        if (c.source.type === A.EndpointType.Bus) {
            bus = connectables.source as A.Bus
        }
        if (c.target.type === A.EndpointType.Bus) {
            bus = connectables.target as A.Bus
        }

        if (!actor || !bus || !port) { return [] }

        const busLeft = bus.left.value
        const busRight = busLeft + bus.width.value
        const busTop = bus.top.value
        const busBottom = busTop + bus.height.value

        const portX = actor.left.value + getPortLocalX(
            port.border.value,
            { height: actor.height.value, width: actor.width.value },
            port.offset.value,
            portDimensions,
        ) + portDimensions.width / 2
        const portY = actor.top.value + getPortLocalY(
            port.border.value,
            { height: actor.height.value, width: actor.width.value },
            port.offset.value,
            portDimensions,
        ) + portDimensions.height / 2

        const sourceKey = getEndpointKey(c.source)
        const targetKey = getEndpointKey(c.target)

        return [
            <Connection
                busBottom={busBottom}
                busLeft={busLeft}
                busOrientation={bus.orientation}
                busRight={busRight}
                busTop={busTop}
                key={`${sourceKey}-${targetKey}`}
                portX={portX}
                portY={portY}
            />,
        ]
    })

    const makeListItem = (
        description: string,
        value: JSX.Element,
    ): JSX.Element => {
        return <li style={{ width: '100%' }}>
            <span>{description}</span>
            <span style={{ float: 'right' }}>{value}</span>
        </li>
    }

    const busesList = buses.map((locatedBus) => {
        const bus = locatedBus.value

        return (
            <li key={bus.name.value}>
                <details style={{ padding: 0 }}>
                    <summary>{bus.name.value}</summary>
                    <ul style={{ listStyle: 'none', padding: '2px' }}>
                        {makeListItem(
                            'Left x Top:',
                            <span>{bus.left.value} x {bus.top.value}</span>
                        )}
                        {makeListItem(
                            'Width x Height:',
                            <span>{bus.width.value} x {bus.height.value}</span>
                        )}
                        {makeListItem('Orientation:', <span>{bus.orientation}</span>)}
                    </ul>
                </details>
            </li>
        )

    })

    const actorsList = actors.map((locatedActor) => {
        const actor = locatedActor.value

        const inPorts = actor.inPorts.map((locatedPort) => {
            const port = locatedPort.value
            return <li key={port.name.value}>
                {port.name.value}
            </li>
        })

        const outPorts = actor.outPorts.map((locatedPort) => {
            const port = locatedPort.value
            return <li key={port.name.value}>
                {port.name.value}
            </li>
        })

        return (
            <li key={actor.name.value}>
                <details style={{ padding: 0 }}>
                    <summary>{actor.name.value}</summary>
                    <ul>
                        {makeListItem(
                            'Left x Top:',
                            <span>{actor.left.value} x {actor.top.value}</span>
                        )}
                        {makeListItem(
                            'Width x Height:',
                            <span>{actor.width.value} x {actor.height.value}</span>
                        )}
                        {makeListItem('Color:', <span>{actor.color.value}</span>)}
                        <li>
                            <span>In ports:</span>
                            <ul>{inPorts}</ul>
                        </li>
                        <li>
                            <span>Out ports:</span>
                            <ul>{outPorts}</ul>
                        </li>
                    </ul>
                </details>
            </li>
        )
    })

    // order matters: later will appear on top
    return (
        <span>
            <span
                className='.svg-container'
            >
                <svg ref={svgRef}>
                    {connectionsComponents}
                    {busesComponents}
                    {actorsComponents}
                </svg>
            </span>
            <span
                className="controls"
            >
                <div id="lasterrormsg"></div>
                <p style={{ margin: '2px' }}>Actors</p>
                <ul>{actorsList}</ul>
                <p style={{ margin: '2px' }}>Buses</p>
                <ul>{busesList}</ul>
            </span>
        </span>
    )

}

/**
 * Class that manages webview state.
 * TODO: this class no longer needs to exist, or can be focused solely on the
 * listeners
 */
class Webview {

    #errormsgDiv = document.getElementById('lasterrormsg') as HTMLDivElement

    constructor() {
        return
    }

    setSystemModel(): void {
        console.log('setSystemModel, ignoring')
    }

    clearModel(errorMsgText: string): void {
        if (this.#errormsgDiv) { this.#errormsgDiv.innerText = errorMsgText }
    }

    #trackListeners: Map<TrackedIndex, TrackedValueListener> = new Map()

    /* Returns the listener for that index. */
    getListener(idx: TrackedIndex): TrackedValueListener {
        const listener = this.#trackListeners.get(idx)
        if (!listener) { return () => { return } }
        return listener
    }

    whenTrackedChanged<V>(
        idx: TrackedIndex,
        parser: (newValue: string) => V | undefined,
        listener: (newValue: V) => void,
    ): void {
        const handler = (newValue: string) => {
            const parsed = parser(newValue)
            if (parsed) { listener(parsed) }
        }
        this.#trackListeners.set(idx, handler)
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
            const listener = this.getListener(e.trackIndex)
            listener(e.newText)
        }
    }

}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
declare const acquireVsCodeApi: any

const vscode = acquireVsCodeApi()

{
    const container = (document.getElementById('webview-container') as unknown) as HTMLDivElement
    if (container === null) {
        console.log('Webview container <div> is missing')
    } else {
        const sys = new Webview()

        ReactDOM.render(
            <WebviewComponent sys={sys} />,
            container,
        )

        console.log('Start loop')

        vscode.postMessage({ tag: webview.Tag.Ready })
    }
}

function getCollidables(state: {
    actors: A.TrackedValue<A.Actor>[],
    buses: A.TrackedValue<A.Bus>[],
}): readonly XYRange<number>[] {
    return ([] as readonly XYRange<number>[]).concat(
        state.actors.map(a => getXYRange(a.value)),
        state.buses.map(b => getXYRange(b.value)),
    )
}

// TODO: reimplement collision
// eslint-disable-next-line @typescript-eslint/no-unused-vars
function overlaps(
    state: {
        actors: A.TrackedValue<A.Actor>[],
        buses: A.TrackedValue<A.Bus>[],
    },
    thisObject: unknown, r: Rectangle<number>
): boolean {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const collidables = getCollidables(state)
    for (const collidable of collidables) {
        const collidableBottom = collidable.top + collidable.height
        const collidableRight = collidable.left + collidable.width
        if (collidable === thisObject) continue
        if (r.right <= collidable.left) continue
        if (collidableRight <= r.left) continue
        if (r.bottom <= collidable.top) continue
        if (collidableBottom <= r.top) continue
        return true
    }
    return false
}

function getConnectables(
    state: {
        actors: readonly A.TrackedValue<A.Actor>[],
        buses: readonly A.TrackedValue<A.Bus>[],
    },
    connection: A.Connection,
): { source: Connectable, target: Connectable } | undefined {
    const source = getEndpoint(state, connection.source)
    if (!source) { return }
    const target = getEndpoint(state, connection.target)
    if (!target) { return }
    return { source, target }
}

function getEndpoint(
    state: {
        actors: readonly A.TrackedValue<A.Actor>[],
        buses: readonly A.TrackedValue<A.Bus>[],
    },
    endpoint: A.Endpoint,
): Connectable | undefined {
    switch (endpoint.type) {
        case A.EndpointType.Port: {
            const actor = state.actors.find(a =>
                a.value.name.value === endpoint.actor
            )
            if (!actor) { return }
            const allPorts = ([] as readonly A.Port[]).concat(
                actor.value.inPorts.map(p => p.value),
                actor.value.outPorts.map(p => p.value),
            )
            const port = allPorts.find(p => p.name.value === endpoint.port)
            if (!port) { return }
            return { actor: actor.value, port: port }
        }
        case A.EndpointType.Bus: {
            return state.buses.find(b => b.value.name.value === endpoint.bus)?.value
        }
    }
}

function getEndpointKey(
    endpoint: A.Endpoint,
): string {
    switch (endpoint.type) {
        case A.EndpointType.Port: return `${endpoint.actor}#${endpoint.port}`
        case A.EndpointType.Bus: return `${endpoint.bus}`
    }
}

// sets a field whose type is A.TrackedValue<T>
const setField =
    <ValueType, R extends Record<Key, A.TrackedValue<ValueType>>, Key extends keyof R>
        (fieldName: Key) =>
        (fieldValue: ValueType) =>
            ({ [fieldName]: { value: { $set: fieldValue } } })

const setFieldAtIndex =
    <ValueType, R extends Record<Key, A.TrackedValue<ValueType>>, Key extends keyof R>
        (fieldName: Key, index: number) =>
        (fieldValue: ValueType) =>
            ({ [index]: { value: setField<ValueType, R, Key>(fieldName)(fieldValue) } })

/**
 * Generic function for setting a field value within the state.
 * Calls look like:
 *
 *   setStateField(index, setActors, 'width')
 *
 * @param index - Index of the type of object (e.g. 'actor') in its array field
 * (e.g. 'actors')
 * @param setTrackedObject - React setter for the array field (must accept and
 * return an array)
 * @param fieldKey - Key of the field to be updated within the given object
 * (e.g. 'width')
 */
const setStateField =
    <
        ModelKey extends keyof SubModel & keyof State,
        SubModel extends State & Record<ModelKey, TrackedObjectType[]>,
        ValueType extends number | string,
        FieldKey extends keyof TrackedObjectType['value'],
        TrackedFieldType extends A.TrackedValue<ValueType>,
        TrackedObjectType extends A.TrackedValue<Record<FieldKey, TrackedFieldType>>
    >(
        index: number,
        setTrackedObject: React.Dispatch<React.SetStateAction<TrackedObjectType[]>>,
        fieldKey: FieldKey,
    ) =>
        (value: ValueType) => {
            setTrackedObject((prevTrackedObject: TrackedObjectType[]) =>
                update(
                    prevTrackedObject,
                    // TODO: see if we can make this type-check without upcasts
                    setFieldAtIndex<
                        ValueType,
                        // eslint-disable-next-line @typescript-eslint/no-explicit-any
                        any, // Record<ValueKey, A.TrackedValue<ValueType>>,
                        // eslint-disable-next-line @typescript-eslint/no-explicit-any
                        any // ValueKey
                    // eslint-disable-next-line @typescript-eslint/no-explicit-any
                    >(fieldKey, index)(value) as any,
                )
            )
        }

function sendToExtension(r: webview.Event): void {
    vscode.postMessage(r)
}

function setSystemModelDone(): void {
    const upd: webview.SetSystemModelDone = { tag: webview.Tag.SetSystemModelDone }
    sendToExtension(upd)
}

function visitURI(idx: A.LocationIndex): void {
    const cmd: webview.VisitURI = {
        tag: webview.Tag.VisitURI,
        locationIdx: idx,
    }
    sendToExtension(cmd)
}

function sendUpdateDoc(s: ChangeSet) {
    if (s.edits.length > 0) {
        const msg: webview.UpdateDocument = {
            tag: webview.Tag.UpdateDocument,
            edits: s.edits,
        }
        sendToExtension(msg)
    }
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any, @typescript-eslint/no-unused-vars
function adjustX(thisObject: any, r: YRange<number>, width: number, oldLeft: number, newLeft: number): number {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const collidables: any = []

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

// eslint-disable-next-line @typescript-eslint/no-explicit-any, @typescript-eslint/no-unused-vars
function adjustY(thisActor: unknown, r: XRange<number>, height: number, oldTop: number, newTop: number): number {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const l: any = []
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
