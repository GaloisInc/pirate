import { TextPosition, TextRange } from "../shared/position"
import { Actor, Port } from "../shared/architecture"
import * as A from "../shared/architecture"

import * as lexer from './lexer'

export { TextPosition as Position }
export type Options = lexer.Options

/**
 * Error from parser
 */
export interface Error extends TextRange {
    message: string
}

function mkLocated<T>(r:TextRange, v:T): A.TextLocated<T> {
    return { start: r.start, end: r.end, value: v }
}

class ParserStream {
    #lexer: lexer.Lexer

    constructor(value: string, options: lexer.Options) {
        this.#lexer = new lexer.Lexer(value, options)
    }

    errors: Error[] = []

    pushError(t:TextRange, msg:string) {
        this.errors.push({
            start: t.start,
            end: t.end,
            message: msg
        })
    }
    
    peek(): lexer.Token { return this.#lexer.peek() }
    next(): lexer.Token { return this.#lexer.next() }
}

function consumeOperator(p: ParserStream, v:string):lexer.OperatorToken|undefined { 
    let t = p.next()
    switch (t.kind) {
    case '#end':
        p.pushError(t, "Unexpected end of stream.")
        return undefined
    case '#error':
        p.pushError(t, t.message)
        return undefined
    case '#operator':
        if (t.value === v) return t
        p.pushError(t, "Unexpected operator '" + t.value + "'")
        return undefined
    case '#keyword':
        p.pushError(t, "Unexpected keyword '" + t.value + "'")
        return undefined
    default:
        p.pushError(t, "Expected '" + v + "'")
        return undefined
    }
}

/////////////////////////////////////////////////////////////////////////////////////

type Parser<T> = (p: ParserStream) => T|undefined


function parseFailure(p:ParserStream, t:TextRange, msg:string) : undefined {
    p.pushError(t, msg)
    return undefined
}

const identParser : Parser<lexer.Identifier> = (p: ParserStream) => {
    let t = p.next()
    switch (t.kind) {
    case '#end':
        return parseFailure(p, t, "Unexpected end of stream.")
    case '#error':
        return parseFailure(p, t, t.message)
    case '#keyword':
        return t
    default:
        return parseFailure(p, t, "Expected identifier.")
    }
}


/**
 * Parse a number field 
 */
const numberParser: Parser<A.TextLocated<number>> = (p:ParserStream) => {

    let t = p.next()
    switch (t.kind) {
    case '#end':
        p.pushError(t, "Unexpected end of stream.")
        return undefined
    case '#error':
        p.pushError(t, t.message)
        return undefined
    case '#number':
        break
    default:
        p.pushError(t, "Expected numeric literal.")
        return undefined
    }
    return mkLocated(t, t.value) 
}

/**
 * Parse a string field 
 */
const stringParser: Parser<A.TextLocated<string>> = (p:ParserStream) => {
    let t = p.next()
    switch (t.kind) {
    case '#end':
        p.pushError(t, "Unexpected end of stream.")
        return undefined
    case '#error':
        p.pushError(t, t.message)
        return undefined
    case '#string':
        break
    default:
        p.pushError(t, "Expected string literal.")
        return undefined
    }
    return mkLocated(t, t.value) 
}

function choicesMessage(choices:string[]): string {
    switch (choices.length) {
    case 0:
        return "internal error: Invalid enumerator."
    case 1:
        return "Expected " + choices[0] + "."
    default:
        let v = "Expected one of " + choices[0]
        for (let i = 1; i < choices.length - 1; ++i) 
            v = v + ", " + choices[i]
        v = v + " or " + choices[choices.length-1] + "."
        return v
    }
}
function enumParser(choices:string[]): Parser<A.TextLocated<string>> {
    return (p: ParserStream) => {
        let t = p.next()
        let v:string
        switch (t.kind) {
        case '#end':
            return parseFailure(p, t, "Unexpected end of stream.")
        case '#error':
            p.pushError(t, t.message)
            return undefined
        case '#keyword':
            break
        default:
            p.pushError(t, choicesMessage(choices))
            return undefined
        }

        if (choices.indexOf(t.value) === -1) {
            p.pushError(t, choicesMessage(choices))
            return undefined
        }

        return mkLocated(t, t.value) 
    }
}


/**
 * Parse a location
 */
const locationParser: Parser<A.TextLocated<A.SourceLocation>> = (p:ParserStream) => {
    const t = stringParser(p)
    if (t === undefined) return undefined

    // Read column from end
    const v = t.value 
    let lastIdx = v.length - 1
    const colPair = lexer.readDigitsRev(v, lastIdx)
    if (colPair.count === 0) {
        p.pushError(t, "Could not find column.")
        return undefined
    }
    lastIdx -= colPair.count
    // Read line from end
    if (v.charAt(lastIdx) !== ':') {
        p.pushError(t, "Could not find column separator")
        return undefined
    }
    --lastIdx
    const linePair = lexer.readDigitsRev(v, lastIdx)
    if (linePair.count === 0) {
        p.pushError(t, "Could not find line number.")
        return undefined
    }
    lastIdx -= linePair.count
    if (v.charAt(lastIdx) !== ':') {
        p.pushError(t, "Could not find line separator "
            + colPair.value + ' ' + linePair.value + ' ' + lastIdx.toString() + ' ' + v.charAt(lastIdx) + ' ' + linePair.count)
        return undefined
    }

    const filename = v.slice(0, lastIdx)

    let loc: A.SourceLocation = {
        filename: filename,
        line: linePair.value,
        column: colPair.value
    }

    return mkLocated(t, loc)
}

const enum Arity { Required, Array }


interface ObjectField {
    fieldName: string
    lexName: string
    arity: Arity
    setter: ( p: ParserStream
            , obj: any
            , key: lexer.Identifier
            ) => boolean
}


function reqObjField<T>(nm: string, tp:Parser<T>):ObjectField {
    return { 
        fieldName: nm, 
        lexName: nm, 
        arity: Arity.Required, 
        setter: (p: ParserStream, obj:any, key:lexer.Identifier) => {
            if (!consumeOperator(p, ':')) return false

            if (obj[nm] !== undefined) {
                p.pushError(key, nm + " already defined.")
                return false
            }

            const r = tp(p)
        
            if (!consumeOperator(p, ';')) {
                obj[nm] = null
                return false
            }
            
            obj[nm] = r
            return (r !== undefined)
        }
    }
}

function arrayObjField(fieldName: string, lexName: string, fields: ObjectField[]): ObjectField {
    return { 
        fieldName: fieldName, 
        lexName: lexName,
        arity: Arity.Array,
        setter: (p: ParserStream, o:any, k:lexer.Identifier) => objectType(fields, p, o, fieldName, k)
    }
}

/**
 * Call one of matches if next token is a keyword that matches.
 * 
 * @param fields List of keyword actions to match against.
 * @returns true if a match is found
 */
function objectType(fields: ObjectField[],
                    p: ParserStream,
                    obj: any,
                    fieldName: string,
                    tkn: lexer.Identifier): boolean {                    
    const name = identParser(p)
    if (!name) return false
    if (!consumeOperator(p, '{')) return false

    let partial: any = {name: mkLocated(name, name.value)}
    for (const c of fields) {
        if (c.arity === Arity.Array) 
            partial[c.fieldName] = []
    }
    
    let reading:boolean = true
    while (reading) {

        let t = p.peek()
        // Keep parsing while we get keywords
        if (t.kind !== '#keyword') break

        let read:boolean = false
        for (const c of fields) {
            if (t.value === c.lexName) {
                p.next() // Read keyword

                read = c.setter(p, partial, t)
                break
            }
        }
        if (!read) break

    }

    const rcurly = consumeOperator(p, '}')
    if (!rcurly) return false

    // Check fields are defined.
    let r : TextRange = { start: tkn.start, end: tkn.end }
    for (const c of fields) {
        if (c.arity === Arity.Required && !partial[c.fieldName]) {
            if (partial[c.fieldName] === undefined) p.pushError(r, "Missing " + c.lexName + ".")
            return false
        }    
    }
    obj[fieldName].push(partial)
    return true
}    

const portType: ObjectField[] = [
    reqObjField('location', locationParser),
    reqObjField('border', enumParser([A.Border.Left, A.Border.Right, A.Border.Top, A.Border.Bottom])),
    reqObjField('offset', numberParser)
]

/** Parser for actors */
const actorType: ObjectField[] = [
    reqObjField('location', locationParser),
    reqObjField('left',   numberParser),
    reqObjField('top',    numberParser),
    reqObjField('width',  numberParser),
    reqObjField('height', numberParser),
    reqObjField('color',  stringParser),
    arrayObjField('inPorts', 'in_port', portType),
    arrayObjField('outPorts', 'out_port', portType),
]

/** Parser for bus */
const busType: ObjectField[] = [
    reqObjField('orientation', enumParser([A.BusOrientation.Horizontal, A.BusOrientation.Vertical])),
    reqObjField('left',   numberParser),
    reqObjField('top',    numberParser),
    reqObjField('width',  numberParser),
    reqObjField('height', numberParser),
]

/**
 * Call one of matches if next token is a keyword that matches.
 * 
 * @param choices List of keyword actions to match against.
 * @returns true if a match is found
 */
function matchKeyword(p: ParserStream, partial: any, choices: ObjectField[]): boolean {
    let t = p.peek()
    // Keep parsing while we get keywords
    if (t.kind !== '#keyword') return false

    for (const c of choices) {
        if (t.value === c.lexName) {
            p.next() // Read keyword

            return c.setter(p, partial, t)
        }
    }
    return false
}  

function matchOperator(p: ParserStream, v:string):lexer.OperatorToken|undefined { 
    let t = p.peek()
    switch (t.kind) {
    case '#end':
        return undefined
    case '#error':
        return undefined
    case '#operator':
        if (t.value === v) {
            p.next()
            return t
        }
        return undefined
    case '#keyword':
        return undefined
    default:
        return undefined
    }
}

const endpointParser : Parser<A.Endpoint> = (p:ParserStream) => {
    const x = identParser(p)
    if (!x) return undefined

    if (matchOperator(p, '.')) {
        const y = identParser(p)
        if (!y) return undefined
        return { type: A.EndpointType.Port, actor: x.value, port: y.value }
    } else {
        return { type: A.EndpointType.Bus, bus: x.value }
    }

}

/**
 * Read declarations and return services
 */
function consumeLayout(p:ParserStream): A.SystemLayout|null {
    const start = p.peek()

    let partial: any = {actors: [], buses: [], connections: []}
    let cont: Boolean = true
    while (cont) {
        const r = matchKeyword(p, partial, [
            reqObjField('width', numberParser),
            reqObjField('height', numberParser),
            arrayObjField('actors', 'actor', actorType),
            arrayObjField('buses', 'bus', busType),
            {
                fieldName: 'connections',
                lexName: 'connect',
                arity: Arity.Array,
                setter: (p, partial, tkn) => {

                    const x = endpointParser(p)
                    if (!x) return false
                    const y = endpointParser(p)
                    if (!y) return false
                    if (!consumeOperator(p, ';')) return false
                    partial.connections.push({ source: x, target: y })
                    return true
                }
            }
        ])
        if (!r) break
    }

    const t = p.next()
    switch (t.kind) {
    case '#end':
        break
    default:
        p.pushError(t, "Unexpected character after system layout.")
        break
    }

    let r : TextRange = { start: start.start, end: t.end }
    if (!partial.width) {
        if (partial.width === undefined) p.pushError(r, "Missing width.")
        return null
    }
    if (!partial.height) {
        if (partial.height === undefined) p.pushError(r, "Missing height.")
        return null
    }
    

    return partial
}


export function parseArchitectureFile(text: string, options: lexer.Options): { value: A.SystemLayout | null, errors: Error[] } {
    const p = new ParserStream(text, options)

    const r = consumeLayout(p)

    return {
        // This is a pure grammar, the value will be undefined until we add embedded actions
        // or enable automatic CST creation.
        value: r,
        errors: p.errors
    }
}