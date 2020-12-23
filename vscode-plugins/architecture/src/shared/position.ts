
/**
 * A position including character index, line and column.
 */
export interface TextPosition {
    /**
     * 0-based index of character within string.
     */
    readonly index: number
    
    /**
     * 1-based index indicating line
     */
    readonly line: number

    /**
     * 1-based index indicating column.
     */
    readonly column: number
}

/**
 * A range of positions
 */
export interface TextRange {
    readonly start: TextPosition
    readonly end: TextPosition
}