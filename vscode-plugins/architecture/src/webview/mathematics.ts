/**
 * Library functions for common mathematic operations.
 */

/**
 * Clamp a number between a minimum and maximum value.
 * @param v - Value to be clamped
 * @param min - Minimal value for the ouput
 * @param max - Maximal value for the output
 */
export function clamp(v: number, min: number, max: number): number {
    return Math.min(Math.max(v, min), max)
}
