import { endianness } from 'os'
import { mainModule } from 'process'
import { start } from 'repl'
import { runInThisContext } from 'vm'
import { TextPosition, TextRange } from '../shared/position'

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
 * @param x 
 * @param y 
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
function unexpectedStringChar(p: TextPosition, msg:string): StringError {
    return { start: p, end: p, message: msg }
}
/**
 * Return error token for unexpected character.
 */
function unexpectedChar(p: TextPosition, msg:string): ErrorToken {
    return { kind: '#error', start: p, end: p, message: msg }
}

export class Lexer {
    #input: string
    #options: Options

    // Next token in string to return

    #next: Token

    #index: number
    #line: number
    #column: number

    constructor(input: string, options: Options) {
        this.#input = input
        this.#options = options
        this.#index = 0
        this.#line = 1
        this.#column = 1

        this.#next = this.getNext()
    }

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
                    const end = { index: index, line: line, column: col }
                    this.#index = index + 1
                    this.#line = line
                    this.#column = col + 1                
                    return { kind: '#string', start: start, end: end, value: value, errors: errors }
                }
            // End of file
            case '':
                {
                    const end = { index: index, line: line, column: col }
                    errors.push({ start: start, end: end, message: "Unterminated string literal." })
                    this.#index = index
                    this.#line = line
                    this.#column = col
                    return { kind: '#string', start: start, end: end, value: value, errors: errors }
                }
            case '\t':
                {
                    errors.push(unexpectedStringChar({ index: index, line: line, column: col }, "Unescaped tab characters not allowed in strings."))
                    value = value + '\t'
                    let r = this.advanceTab({index: index+1, column: col+1})
                    index = r.index
                    col = r.column
                }
                break
            case '\r':
            case '\n':
                {
                    errors.push(unexpectedStringChar({ index: index, line: line, column: col },  "Unterminated string literal."))
                    const end = { index: index, line: line, column: col } 
                    this.#index = index + 1
                    this.#line = line + 1
                    this.#column = 1
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
                value = value + d
                break
            }
        }
    }

    private advanceTab(p:{index: number, column: number}) : {index: number, column: number} {
        const tabstop = this.#options.tabstopWidth
        const oldCol = p.column
        const newCol = tabstop * Math.ceil(oldCol / tabstop) + 1
        return {
            index: p.index + (newCol - oldCol),
            column: newCol
        }

    }

    private getNext(): Token {
        const input = this.#input
        const tabstop = this.#options.tabstopWidth
        let index = this.#index
        let line = this.#line
        let col = this.#column

        while (true) {
            const c = input.charAt(index)
            if (c === '') {
                this.#index = index
                this.#line = line  
                this.#column = col
                const p = { index: index, line: line, column: col }  
                return { kind: '#end', start: p, end: p }
            } else if (c === ' ') {
                ++index; ++col
            } else if (c === '\r') {
                index += input.charAt(index+1) === '\n' ? 2 : 1
                ++line
                col = 1 
            } else if (c === '\n') {
                ++index
                ++line
                col = 1
            } else if (c === '\t') {
                const oldCol = col
                col = tabstop * Math.ceil(col / tabstop) + 1
                index += (col - oldCol)
            } else if (c === '/') {
                // If the next character is not / then, we have an invalid character
                if (input.charAt(index+1) !== '/') {
                    this.#index = index+2
                    this.#line = line  
                    this.#column = col+2
                    return unexpectedChar({ index: index, line: line, column: col }, "Unexpected character '/'")
                }
                // Check for comment that reads to end of line
                index += 2
                let done = false
                while (!done) {
                    const c = input.charAt(index)
                    switch (input.charAt(index)) {
                    case '':
                        this.#index = index
                        this.#line = line  
                        this.#column = col
                        const p = { index: index, line: line, column: col }
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
                // Continue to read next character
            } else if (isAlpha(c)) {
                // Read Keyword
                const startIndex = index
                let d = input.charAt(index+1)
                while (isAlpha(d) || '0' <= d && d <= '9' || ['_'].indexOf(d) !== -1) {
                    ++index
                    d = input.charAt(index+1)
                }
                this.#index = index+1
                this.#line = line
                this.#column = col + index + 1 - startIndex
                const start = { index: startIndex, line: line, column: col }
                const end = { index: index, line: line, column: col + index - startIndex }
                const v = input.substr(startIndex, index + 1 - startIndex)
                return { kind: '#keyword', start: start, end: end, value: v}
            } else if ('0' <= c && c <= '9') {
                // Read a number literal
                let value: number = (c as any) - ('0' as any)
                const startIndex = index
                let d = input.charAt(index+1)
                while ('0' <= d && d <= '9') {
                    value = 10 * value + ((d as any) - ('0' as any))
                    ++index
                    d = input.charAt(index+1)
                }
                const start = { index: startIndex, line: line, column: col }
                const end = { index: index, line: line, column: col + index - startIndex }
                this.#index = index+1
                this.#line = line
                this.#column = col + index + 1 - startIndex
                return { kind: '#number', start: start, end: end, value: value}

            } else if (c === '"') {
                return this.readStringLit(index, line, col)
            } else if ([';', '{', '}', ':', '.'].indexOf(c) !== -1) {
                // Operator
                const p = { index: index, line: line, column: col }
                const value = input.substr(index, 1)
                this.#index = index+1
                this.#line = line
                this.#column = col+1
                return { kind: '#operator', start: p, end: p, value: value}
            } else {
                return unexpectedChar({ index: index, line: line, column: col }, "Unexpected character '" + c + "'")
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
        let index = this.#index
        // Read to end of string, carriage return or newline
        while (['', '\r', '\n'].indexOf(input.charAt(index)) === -1)
            ++index
        this.#index = index
        this.#next = this.getNext()
    }
}