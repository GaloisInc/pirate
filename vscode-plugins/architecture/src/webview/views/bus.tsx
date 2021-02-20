/**
 * Bus view.
 */

import * as React from 'react'

import * as A from '@shared/architecture'

import { useSVGDrag } from './draggable'

export function Bus(props: Readonly<{
    height: number,
    left: number,
    locatedBus: A.TrackedValue<A.Bus>,
    setMyHeight: (height: number) => void,
    setMyLeft: (left: number) => void,
    setMyTop: (top: number) => void,
    setMyWidth: (width: number) => void,
    top: number,
    width: number,
}>): JSX.Element {

    const rectRef: React.RefObject<SVGRectElement> = React.createRef()

    const onDrag = (deltaLeft: number, deltaTop: number) => {
        props.setMyLeft(props.left + deltaLeft)
        props.setMyTop(props.top + deltaTop)
    }

    return (
        <g
            {...useSVGDrag(rectRef, onDrag)}
            height={props.height}
            transform={`translate(${props.left}, ${props.top})`}
            width={props.width}
        >
            <rect
                height={props.height}
                ref={rectRef}
                width={props.width}
            />
        </g>
    )
}
