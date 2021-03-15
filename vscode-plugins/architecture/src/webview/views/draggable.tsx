/**
 * Implements a React helper hook for dealing with draggable elements. Takes
 * care of the logic of capturing and releasing the pointer events, and sends
 * updates for every movement.
 */

import * as React from 'react'
// import { LeftTopPosition, XYRange } from '../geometry'

export function useSVGDrag(
    draggedElementRef: React.RefObject<SVGElement>,
    onDragCallback: (deltaX: number, deltaY: number) => void,
): {
    onPointerDown: (event: React.PointerEvent<SVGElement>) => void,
    onPointerMove: (event: React.PointerEvent<SVGElement>) => void,
    onPointerOut: (event: React.PointerEvent<SVGElement>) => void,
    onPointerUp: (event: React.PointerEvent<SVGElement>) => void,
} {
    const [isDragging, setDragging] = React.useState(false)

    const onDragStart = (event: React.PointerEvent<SVGElement>) => {
        event.stopPropagation()
        draggedElementRef.current?.setPointerCapture(event.pointerId)
        setDragging(true)
    }

    function onDrag(event: React.PointerEvent<SVGElement>): void {
        event.stopPropagation()
        if (!isDragging) return
        onDragCallback(event.movementX, event.movementY)
    }

    const onDragEnd = (event: React.PointerEvent<SVGElement>): void => {
        event.stopPropagation()
        if (isDragging) {
            draggedElementRef.current?.releasePointerCapture(event.pointerId)
        }
        setDragging(false)
    }

    return {
        onPointerDown: onDragStart,
        onPointerMove: onDrag,
        onPointerOut: onDragEnd,
        onPointerUp: onDragEnd,
    }

}
