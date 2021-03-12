/**
 * @module
 * Connection view.  A connection is displayed as a link connection its
 * endpoints.  As such, it has no "proper" coordinates, and must instead rely on
 * staying up-to-date with the coordinates of the endpoints it links.
 */

import * as React from 'react'

import * as A from '@shared/architecture'

import * as M from '../mathematics'

export type Connectable = A.Bus | { actor: A.Actor, port: A.Port }

export function Connection(props: {
    busOrientation: A.BusOrientation
    busLeft: number
    busTop: number
    busRight: number
    busBottom: number
    portX: number
    portY: number
}): JSX.Element {

    // always copy the port XY
    const x1 = props.portX
    const y1 = props.portY

    // try to reach the bus perpendicularly, if possible
    const x2 = (
        props.busOrientation === A.BusOrientation.Horizontal
            ? M.clamp(props.portX, props.busLeft, props.busRight)
            : (props.portX <= props.busLeft ? props.busLeft : props.busRight)
    )
    const y2 = (
        props.busOrientation === A.BusOrientation.Horizontal
            ? (props.portY <= props.busTop ? props.busTop : props.busBottom)
            : M.clamp(props.portY, props.busTop, props.busBottom)
    )

    return (
        <line
            className='connection'
            x1={x1}
            x2={x2}
            y1={y1}
            y2={y2}
        />
    )

}
