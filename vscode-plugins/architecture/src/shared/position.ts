/**
 * Interfaces for dealing with text positions in files.
 */

/**
 * A position including character index, line and column.
 */
export interface TextPosition {
    /**
     * 0-based index of character within string.
     */
    readonly index: number

    /**
     * 0-based index indicating line
     */
    readonly line: number

    /**
     * 0-based index indicating column.
     */
    readonly character: number
}

/**
 * A range of positions
 */
export interface TextRange {
    readonly start: TextPosition
    readonly end: TextPosition
}

export interface TextLocated<T> extends TextRange {
    value: T
}
