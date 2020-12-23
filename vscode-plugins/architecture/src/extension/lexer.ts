import { endianness } from 'os'
import { mainModule } from 'process'
import { start } from 'repl'
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

export interface StringLiteral extends TextRange {
    readonly kind: '#string'
    readonly value: string
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
                    this.#index = index
                    this.#line = line  
                    this.#column = col
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
                while (isAlpha(d) || ['_'].indexOf(d) !== -1) {
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
                this.#index = index+1
                this.#line = line
                this.#column = col + index + 1 - startIndex
                const start = { index: startIndex, line: line, column: col }
                const end = { index: index, line: line, column: col + index - startIndex }
                return { kind: '#number', start: start, end: end, value: value}

            } else if (c === '"') {
                const startIndex = index
                ++index
                let value = ""
                while (true) {
                    const d = input.charAt(index)
                    switch (d) {
                    case '':
                        {
                            this.#index = index
                            this.#line = line
                            this.#column = col + (index - startIndex)
                            const start = { index: startIndex, line: line, column: col }
                            const end = { index: index, line: line, column: index-1-startIndex }
                            return { kind: '#error', start: start, end: end, message: "Unterminated string literal." }
                        }
                    case '"':
                        this.#index = index+1
                        this.#line = line
                        this.#column = col + (index-startIndex) + 1
                        const start = { index: startIndex, line: line, column: col }                        
                        const end = { index: index, line: line, column: col + (index-startIndex) }
                        return { kind: '#string', start: start, end: end, value: value }
                    case '\\':
                        {
                            const e = input.charAt(index+1)
                            switch (e) {
                            case '':
                                {
                                    const col2 = col + (index-startIndex)
                                    this.#index = index
                                    this.#line = line
                                    this.#column = col2
                                    const start = { index: index, line: line, column: col2 }
                                    const end =   { index: index, line: line, column: col2 }

                                    return { kind: '#error', start: start, end: end, message: "Unterminated escape sequence." }                                    
                                }
                            case 't':
                                index+=2
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
                                const nextCol = col + (index-startIndex)
                                this.#index = index+2
                                this.#line = line
                                this.#column = nextCol+2
                                const start = { index: index, line: line, column: nextCol }
                                const end =   { index: index+1, line: line, column: nextCol+1 }
                                return { kind: '#error', start: start, end: end, message: "Invalid escape sequence '\\" + e + "'" }    
                            }
                        }

                        if (['t', 'r', 'n'].indexOf(input))
                        break
                    case '\t':
                        return unexpectedChar({ index: index, line: line, column: col+(index-startIndex) }, "Tab characters not allowed in strings.")
                    case '\r':
                    case '\n':
                        return unexpectedChar({ index: index, line: line, column: col+(index-startIndex) }, "Unterminated string literal.")
                    default:
                        index++
                        value = value + d
                        break
                    }
                }
            } else if ([';', '{', '}', ':', '.'].indexOf(c) !== -1) {
                // Operator
                this.#index = index+1
                this.#line = line
                this.#column = col+1
                const p = { index: index, line: line, column: col }
                const value = input.substr(index, 1)
                return { kind: '#operator', start: p, end: p, value: value}
            } else {
                return unexpectedChar({ index: index, line: line, column: col }, "Unexpected character '" + c + "'")
            }
        }
        
    }

    constructor(input: string, options: Options) {
        this.#input = input
        this.#options = options
        this.#index = 0
        this.#line = 1
        this.#column = 1

        this.#next = this.getNext()
    }

    peek(): Token {
        return this.#next
    }

    next(): Token {
        const r = this.#next
        this.#next = this.getNext()
        return r
    }
}