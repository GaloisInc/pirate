import { TextPosition, TextRange } from './position'

export interface Identifier extends TextRange {
    readonly kind: '#keyword'
    readonly value: string
}

export interface NumberLiteral extends TextRange {
    readonly kind: '#number'
    readonly value: number
}

export interface OperatorToken extends TextRange {
    readonly kind: '#operator'
    readonly value: string
}

export interface StringError extends TextRange {
    readonly message: string
}

export interface StringLiteral extends TextRange {
    readonly kind: '#string'
    readonly value: string

    // Errors from reading contents of string literal.
    readonly errors: StringError[]
}

export interface EndToken extends TextRange {
    readonly kind: '#end'
}

export interface ErrorToken extends TextRange {
    readonly kind: '#error'
    readonly message: string
}

export type Token = Identifier|OperatorToken|NumberLiteral|StringLiteral|EndToken|ErrorToken

/**
 * Read digits from string starting from end.
 */
export function readDigitsRev(s:string, endOff:number): { count: number, value: number } {
    const start = endOff
    let value = 0
    let mult = 1
    let c = s.charAt(endOff)
    while ('0' <= c && c <= '9') {
        const d = (c as any) - ('0' as any)
        endOff--
        value = d * mult + value
        mult = mult * 10
        c = s.charAt(endOff)
    }
    return { count: start - endOff, value: value}
}

/**
 * Read digits from string starting from index.
 */
export function readDigits(s:string, o:number): { count: number, value: number } {
    const start = o
    let value = 0
    let c = s.charAt(o)
    while ('0' <= c && c <= '9') {
        const d = (c as any) - ('0' as any)
        value = 10 * value + d
        o++
        c = s.charAt(o)
    }
    return { count: o - start, value: value}
}

export interface Options {
    // Number of spaces a tab is used.
    readonly tabstopWidth : number
}

/**
 * Return true if an alphabetic character
 */
function isAlpha(c: string): boolean {
    return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z'
}

/**
 * Return error token for unexpected character.
 */
function unexpectedChar(start: TextPosition, end: TextPosition, msg:string): ErrorToken {
    return { kind: '#error', start: start, end: end, message: msg }
}

export class Lexer {
    #input: string
    #options: Options

    // Next token in string to return

    #next: Token

    #position: TextPosition

//    #index: number
//    #line: number
//    #column: number

    constructor(input: string, options: Options) {
        this.#input = input
        this.#options = options
        this.#position = { index: 0, line: 0, column: 0 }

        this.#next = this.getNext()
    }

    /**
     * Reads a string literal after first double quot e scanned.
     */
    private readStringLit(index: number, line: number, col: number): StringLiteral {
        const input = this.#input
        const start = { index: index, line: line, column: col }                        
        // Advance position over first doublequote
        ++index
        ++col
        let value: string = ""
        let errors: StringError[] = []
        while (true) {
            const d = input.charAt(index)
            switch (d) {
            // End of string literal
            case '"':
                {
                    const end = { index: index+1, line: line, column: col+1 }
                    this.#position = end
                    return { kind: '#string', start: start, end: end, value: value, errors: errors }
                }
            // End of file
            case '':
                {
                    const end = { index: index, line: line, column: col }
                    errors.push({ start: start, end: end, message: "Unterminated string literal." })
                    this.#position = end
                    return { kind: '#string', start: start, end: end, value: value, errors: errors }
                }
            case '\t':
                {
                    const start = { index: index, line: line, column: col }
                    index = index+1
                    col = this.advanceTab(col+1)
                    const end = { index: index, line: line, column: col }
                    errors.push({start: start, end: end, message: "Unescaped tab characters not allowed in strings."})
                    value = value + '\t'
                }
                break
            case '\r':
            case '\n':
                {
                    const start = { index: index, line: line, column: col }
                    index = index + 1
                    line = line + 1
                    col = 0
                    const end = { index: index, line: line, column: col }
                    errors.push({start: start, end: end, message: "Unterminated string literal."})                    
                    this.#position = end
                    return { kind: '#string', start: start, end: end, value: value, errors: errors }
               }
            // Escape sequence
            case '\\':
                {
                    const e = input.charAt(index+1)
                    switch (e) {
                    // Let outer loop handle end of string, tab, carriage return and newline.
                    case '':
                    case '\t':
                    case '\r':
                    case '\n':
                        index++
                        col++
                        break
                    // Escape sequences
                    case 't':
                        index+=2
                        col+=2
                        value = value + '\t'
                        break
                    case 'r':
                        index+=2
                        value = value + '\r'
                        break
                    case 'n':
                        index+=2
                        value = value + '\n'
                        break
                    case '\\':
                        index+=2
                        value = value + '\\'
                        break
                    case '"':
                        index+=2
                        value = value + '"'
                        break
                    default:
                        {
                            const escapeStart = { index: index, line: line, column: col }
                            const end =   { index: index+1, line: line, column: col+1 }
                            errors.push({ start: escapeStart, end: end, message: "Invalid escape sequence '\\" + e + "'" })
                            index = index+2
                            col = col + 2
                        }
                    }
                }
                break
            default:
                index++
                col++
                value = value + d
                break
            }
        }
    }

    private advanceTab(column: number) : number {
        const tabstop = this.#options.tabstopWidth
        return tabstop * Math.ceil(column / tabstop) + 1
    }

    private getNext(): Token {
        const input = this.#input
        const tabstop = this.#options.tabstopWidth
        let index = this.#position.index
        let line = this.#position.line
        let col = this.#position.column

        while (true) {
            const c = input.charAt(index)
            if (c === '') {
                const p = { index: index, line: line, column: col }  
                this.#position = p
                return { kind: '#end', start: p, end: p }
            } else if (c === ' ') {
                ++index; ++col
            } else if (c === '\r') {
                index += input.charAt(index+1) === '\n' ? 2 : 1
                ++line
                col = 0 
            } else if (c === '\n') {
                ++index
                ++line
                col = 0
            } else if (c === '\t') {
                const oldCol = col
                col = tabstop * Math.ceil(col / tabstop) + 1
                index += (col - oldCol)
            } else if (c === '/') {
                // If the next character is not / then, we have an invalid character
                if (input.charAt(index+1) !== '/') {
                    let start = { index: index, line: line, column: col }
                    let end = { index: index+2, line: line, column: col+2 }
                    this.#position = end
                    return unexpectedChar(start, end, "Unexpected character '/'")
                }
                // Check for comment that reads to end of line
                index += 2
                let done = false
                while (!done) {
                    const c = input.charAt(index)
                    switch (input.charAt(index)) {
                    case '':
                        const p = { index: index, line: line, column: col }
                        this.#position = p
                        return { kind: '#end', start: p, end: p }    
                    case '\r':
                        index += input.charAt(index+1) === '\n' ? 2 : 1
                        done = true
                        break
                    case '\n':
                        ++index
                        done = true
                        break
                    default:
                        ++index
                    }
                }
                // Advance to next line
                line++
                col = 0
                // Continue to read next character
            } else if (isAlpha(c)) {
                // Read Keyword
                const startIndex = index
                const start = { index: startIndex, line: line, column: col }
                let d = input.charAt(++index)
                while (isAlpha(d) || '0' <= d && d <= '9' || ['_'].indexOf(d) !== -1)
                    d = input.charAt(++index)
                const end = { index: index, line: line, column: col + (index - startIndex) }
                this.#position = end
                const v = input.substr(startIndex, index - startIndex)
                return { kind: '#keyword', start: start, end: end, value: v}
            } else if ('0' <= c && c <= '9') {
                // Read a number literal
                const start = { index: index, line: line, column: col }
                let r = readDigits(input, index)
                const value  = r.value
                index += r.count
                col   += r.count
                const end = { index: index, line: line, column: col }
                this.#position = end
                return { kind: '#number', start: start, end: end, value: value}
            } else if (c === '"') {
                return this.readStringLit(index, line, col)
            } else if ([';', '{', '}', ':', '.'].indexOf(c) !== -1) {
                // Operator
                const start = { index: index, line: line, column: col }
                const end = { index: index+1, line: line, column: col+1 }
                this.#position = end
                return { kind: '#operator', start: start, end: end, value: c}
            } else {
                let start = { index: index, line: line, column: col }
                let end = { index: index+1, line: line, column: col+1 }
                return unexpectedChar(start, end, "Unexpected character '" + c + "'")
            }
        }
    }

    peek(): Token {
        return this.#next
    }

    next(): Token {
        const r = this.#next
        this.#next = this.getNext()
        return r
    }

    /**
     * Read to end of line and be ready to start tokenizing next line.
     * 
     * Used for recovering parse tree.
     */
    skipToNewLine() : void {
        const input = this.#input
        const p = this.#position
        let index = p.index
        // Read to end of string, carriage return or newline
        while (['', '\r', '\n'].indexOf(input.charAt(index)) === -1)
            ++index
        this.#position = { index: index, line: p.line, column: p.column }
        this.#next = this.getNext()
    }
}