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

    // Next token in string to return
    #next: Token

    #position: TextPosition

    constructor(input: string) {
        this.#input = input
        this.#position = { index: 0, line: 0, character: 0 }

        this.#next = this.getNext()
    }

    /**
     * Reads a string literal after first double quite scanned scanned.
     */
    private readStringLit(start: TextPosition): StringLiteral {
        const input = this.#input
        // Advance position over first doublequote
		let index = start.index+1
		let line  = start.line
        let character = start.character+1

        let value: string = ""
        const errors: StringError[] = []
        while (true) {
            const d = input.charAt(index)
            switch (d) {
            // End of string literal
            case '"':
                {
                    const end:TextPosition = { index: index+1, line: line, character: character+1 }
                    this.#position = end
                    return { kind: '#string', start: start, end: end, value: value, errors: errors }
                }
            // End of file
            case '':
                {
                    const end:TextPosition = { index: index, line: line, character: character }
                    errors.push({ start: start, end: end, message: "Unterminated string literal." })
                    this.#position = end
                    return { kind: '#string', start: start, end: end, value: value, errors: errors }
                }
            case '\t':
                {
                    const start:TextPosition = { index: index, line: line, character: character }
                    index++
                    character++
                    const end:TextPosition = { index: index, line: line, character: character }
                    errors.push({start: start, end: end, message: "Unescaped tab characters not allowed in strings."})
                    value = value + '\t'
                }
                break
            case '\r':
            case '\n':
                {
                    const start:TextPosition = { index: index, line: line, character: character }
                    index++
                    line++
                    character = 0
                    const end:TextPosition = { index: index, line: line, character: character }
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
                        character++
                        break
                    // Escape sequences
                    case 't':
                        index+=2
                        character+=2
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
                            const escapeStart:TextPosition = { index: index, line: line, character: character }
                            const end:TextPosition =   { index: index+1, line: line, character: character+1 }
                            errors.push({ start: escapeStart, end: end, message: "Invalid escape sequence '\\" + e + "'" })
                            index = index+2
                            character = character + 2
                        }
                    }
                }
                break
            default:
                index++
                character++
                value = value + d
                break
            }
        }
    }

    private getNext(): Token {
        const input = this.#input
        let index = this.#position.index
        let line = this.#position.line
        let character = this.#position.character

        while (true) {
            const c = input.charAt(index)
            if (c === '') {
                const p:TextPosition = { index: index, line: line, character: character }
                this.#position = p
                return { kind: '#end', start: p, end: p }
            } else if (c === ' ' || c === '\t') {
                ++index
                ++character
            } else if (c === '\r') {
                index += input.charAt(index+1) === '\n' ? 2 : 1
                ++line
                character = 0
            } else if (c === '\n') {
                ++index
                ++line
                character = 0
            } else if (c === '/') {
                // If the next character is not / then, we have an invalid character
                if (input.charAt(index+1) !== '/') {
                    let start:TextPosition = { index: index, line: line, character: character }
                    let end:TextPosition = { index: index+2, line: line, character: character+2 }
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
                        const p:TextPosition = { index: index, line: line, character: character }
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
                character = 0
                // Continue to read next character
            } else if (isAlpha(c)) {
                // Read Keyword
                const start:TextPosition = { index: index, line: line, character: character }
                let d = input.charAt(++index)
                while (isAlpha(d) || '0' <= d && d <= '9' || ['_'].indexOf(d) !== -1)
                    d = input.charAt(++index)
                const v = input.substr(start.index, index - start.index)
                const end:TextPosition = {
                    index: index,
                    line: line,
                    character: character + (index - start.index)
                }
                this.#position = end
                return { kind: '#keyword', start: start, end: end, value: v}
            } else if ('0' <= c && c <= '9') {
                // Read a number literal
                const start:TextPosition = { index: index, line: line, character: character }
                let r = readDigits(input, index)
                const value  = r.value
                const end:TextPosition = {
                    index: index + r.count,
                    line: line,
				    character: character + r.count
			    }
                this.#position = end
                return { kind: '#number', start: start, end: end, value: value}
            } else if (c === '"') {
				const start:TextPosition = { index: index, line: line, character: character }
                return this.readStringLit(start)
            } else if ([';', '{', '}', ':', '.'].indexOf(c) !== -1) {
                // Operator
                const start:TextPosition = { index: index, line: line, character: character }
                const end:TextPosition   = { index: index+1, line: line, character: character+1 }
                this.#position = end
                return { kind: '#operator', start: start, end: end, value: c}
            } else {
                let start:TextPosition = { index: index,   line: line, character: character }
                let end:TextPosition   = { index: index+1, line: line, character: character+1 }
                this.#position = end
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
        this.#position = { index: index, line: p.line, character: p.character }
        this.#next = this.getNext()
    }
}