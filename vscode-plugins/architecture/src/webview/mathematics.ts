/**
 * Clamp a number between a minimum and maximum value.
 */
export function clamp(v: number, min: number, max: number): number {
    return Math.min(Math.max(v, min), max)
}
