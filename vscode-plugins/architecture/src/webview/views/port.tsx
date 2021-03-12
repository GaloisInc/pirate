/**
 * Port view, currently only ever occurring on actors.
 */

import * as React from 'react'

import * as A from '@shared/architecture'

import { Dimensions } from '../geometry'
import * as M from '../mathematics'

import { useSVGDrag } from './draggable'

export const portDimensions = {
    height: 20,
    width: 20,
}

export enum PortDir { Out = 'Out', In = 'In' }

function getMaxLeft(
    containerDimensions: Dimensions<number>,
    portDimensions: Dimensions<number>,
): number {
    return containerDimensions.width - portDimensions.width
}

function getMaxTop(
    containerDimensions: Dimensions<number>,
    portDimensions: Dimensions<number>,
): number {
    return containerDimensions.height - portDimensions.height
}

export function getPortLocalX(
    border: A.Border,
    containerDimensions: Dimensions<number>,
    offset: number,
    portDimensions: Dimensions<number>,
): number {
    const maxLeft = getMaxLeft(containerDimensions, portDimensions)
    switch (border) {
        case A.Border.Left: return 0
        case A.Border.Right: return maxLeft
        default: return M.clamp(offset, 0, maxLeft)
    }
}

export function getPortLocalY(
    border: A.Border,
    containerDimensions: Dimensions<number>,
    offset: number,
    portDimensions: Dimensions<number>,
): number {
    const maxTop = getMaxTop(containerDimensions, portDimensions)
    switch (border) {
        case A.Border.Top: return 0
        case A.Border.Bottom: return maxTop
        default: return M.clamp(offset, 0, maxTop)
    }
}

function getPortTransform(
    portDimensions: Dimensions<number>,
    props: Readonly<{
        border: A.Border,
        containerDimensions: Dimensions<number>,
        direction: PortDir,
        offset: number,
    }>
): string {
    const x = getPortLocalX(props.border, props.containerDimensions, props.offset, portDimensions)
    const y = getPortLocalY(props.border, props.containerDimensions, props.offset, portDimensions)

    const centerX = portDimensions.width / 2
    const centerY = portDimensions.height / 2

    const rotate = calculatePortRotate(props.border, props.direction)
    return `${translateString(x, y)} ${rotateString(rotate, centerX, centerY)}`
}

export function Port(props: Readonly<{
    border: A.Border,
    containerDimensions: Dimensions<number>,
    direction: PortDir,
    name: string,
    offset: number,
    setMyBorder: (border: A.Border) => void,
    setMyOffset: (offset: number) => void,
}>): JSX.Element {

    const rectRef: React.RefObject<SVGRectElement> = React.createRef()

    const onDrag = (deltaLeft: number, deltaTop: number) => {

        const oldLeft = getPortLocalX(props.border, props.containerDimensions, props.offset, portDimensions)
        const oldTop = getPortLocalY(props.border, props.containerDimensions, props.offset, portDimensions)

        const left = oldLeft + deltaLeft
        const top = oldTop + deltaTop

        const maxLeft = props.containerDimensions.width - portDimensions.width
        const maxTop = props.containerDimensions.height - portDimensions.height

        // Calculate x distance above or below max.
        const xDist = outsideRangeDist(left, 0, maxLeft)
        // Calculate y distance above or below max.
        const yDist = outsideRangeDist(top, 0, maxTop)

        // Figuring out which side of the actor rectangle is closest to evt
        const distLeft = euclid2Dist(Math.abs(left), yDist)
        const distRight = euclid2Dist(Math.abs(maxLeft - left), yDist)
        const distTop = euclid2Dist(Math.abs(top), xDist)
        const distBottom = euclid2Dist(Math.abs(maxTop - top), xDist)
        const distMin = Math.min(distLeft, distTop, distRight, distBottom)

        let newBorder: A.Border
        let newOffset: number
        if (distLeft === distMin) {
            newBorder = A.Border.Left
            newOffset = top
        } else if (distRight === distMin) {
            newBorder = A.Border.Right
            newOffset = top
        } else if (distTop === distMin) {
            newBorder = A.Border.Top
            newOffset = left
        } else {
            newBorder = A.Border.Bottom
            newOffset = left
        }

        props.setMyBorder(newBorder)
        props.setMyOffset(newOffset)
    }

    return (
        <g
            {...useSVGDrag(rectRef, onDrag)}
            className="port"
            height={portDimensions.height}
            transform={getPortTransform(portDimensions, props)}
            width={portDimensions.width}
        >
            <rect
                fill='white'
                height={portDimensions.height}
                ref={rectRef}
                stroke='black'
                strokeWidth='1pt'
                width={portDimensions.width}
            />
            <path d="M 5 5 L 15 10 L 5 15 z" pointerEvents='none' />
        </g>
    )

}

/** `outsideRangeDist(x,l,h)` returns the amount `x` is outside the range `[l,h]`. */
function outsideRangeDist(x: number, l: number, h: number): number {
    if (x < l) { return l - x } else if (x > h) { return h - x } else { return 0 }
}

/**
 * `euclid2Dist(x, y)` returns `sqrt(x * x + y * y)`.
 *
 * It has special cases and assumes `x` and `y` are non-negative.
 */
function euclid2Dist(x: number, y: number) {
    if (x === 0) { return y } else if (y === 0) { return x } else { return Math.sqrt(x * x + y * y) }
}

/**
 * Generate rotate string for a SVG CSS transform attribute.
 */
function rotateString(rotate: number, xoff: number, yoff: number): string {
    return `rotate(${rotate.toString()},${xoff.toString()},${yoff.toString()})`
}

function translateString(x: number, y: number): string {
    return `translate(${x}, ${y})`
}

/**
 * This calculate the orientation of the
 */
function calculatePortRotate(
    border: A.Border,
    dir: PortDir,
): number {
    switch (dir) {
        case PortDir.In: {
            switch (border) {
                case A.Border.Top: return 90
                case A.Border.Right: return 180
                case A.Border.Bottom: return 270
                case A.Border.Left: return 0
                default: return 0
            }
        }
        case PortDir.Out: {
            return (calculatePortRotate(border, PortDir.In) + 180) % 360
        }
    }
}
