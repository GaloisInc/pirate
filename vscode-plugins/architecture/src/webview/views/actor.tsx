/**
 * Actor view, responsible for displaying itself and its ports.
 */

import * as React from 'react'

import * as A from '@shared/architecture'

import { useSVGDrag } from './draggable'
import { Port, PortDir } from './port'

export function Actor(props: Readonly<{
    color: string
    height: number
    inPorts: readonly A.TrackedValue<A.Port>[]
    left: number
    location: A.LocationIndex
    name: string
    onVisitClassClick: (location: A.LocationIndex) => void
    outPorts: readonly A.TrackedValue<A.Port>[]
    setMyHeight: (height: number) => void
    setMyInPortBorder: (portIndex: number) => (border: A.Border) => void
    setMyInPortOffset: (portIndex: number) => (offset: number) => void
    setMyLeft: (left: number) => void
    setMyOutPortBorder: (portIndex: number) => (border: A.Border) => void
    setMyOutPortOffset: (portIndex: number) => (offset: number) => void
    setMyTop: (top: number) => void
    setMyWidth: (width: number) => void
    top: number
    width: number
}>): JSX.Element {

    const containerDimensions = {
        height: props.height,
        width: props.width,
    }

    const foreignObjectRef: React.RefObject<SVGForeignObjectElement> = React.createRef()

    const inPorts = props.inPorts.map(({ value }, index) =>
        <Port
            border={value.border.value}
            containerDimensions={containerDimensions}
            direction={PortDir.In}
            key={value.name.value}
            name={value.name.value}
            offset={value.offset.value}
            setMyBorder={props.setMyInPortBorder(index)}
            setMyOffset={props.setMyInPortOffset(index)}
        />
    )

    const outPorts = props.outPorts.map(({ value }, index) =>
        <Port
            border={value.border.value}
            containerDimensions={containerDimensions}
            direction={PortDir.Out}
            key={value.name.value}
            name={value.name.value}
            offset={value.offset.value}
            setMyBorder={props.setMyOutPortBorder(index)}
            setMyOffset={props.setMyOutPortOffset(index)}
        />
    )

    const rect = (
        <rect
            className='enclave'
            fill={props.color}
            height={props.height}
            width={props.width}
        />
    )

    const foreignObject = (
        <foreignObject
            height={props.height}
            ref={foreignObjectRef}
            width={props.width}
        >
            <div className='enclave-content'>
                <span className='enclave-name'>{props.name}</span>
                <a
                    className='enclave-visit-class'
                    onMouseDown={() => props.onVisitClassClick(props.location)}
                >[Visit Class]</a>
            </div>
        </foreignObject>
    )

    const onDrag = (deltaLeft: number, deltaTop: number) => {
        props.setMyLeft(props.left + deltaLeft)
        props.setMyTop(props.top + deltaTop)
    }

    return (
        <g
            {...useSVGDrag(foreignObjectRef, onDrag)}
            transform={`translate(${props.left}, ${props.top})`}
        >
            {rect}
            {foreignObject}
            {inPorts}
            {outPorts}
        </g>
    )

}
