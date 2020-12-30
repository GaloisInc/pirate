//import * as vscode from 'vscode'

import { TextLocated, TextPosition, TextRange } from "./position"
import { TrackedValue, StringField } from "../shared/architecture"
import * as A from "../shared/architecture"
import * as lexer from './lexer'

export { TextPosition as Position }

/**
 * Error from parser
 */
export interface Error extends TextRange {
    message: string
}

function mkLocated<T>(r:TextRange, v:T): TextLocated<T> {
    return { start: r.start, end: r.end, value: v }
}

interface Tracker {
    track : (r:TextRange) => number|undefined
}

class ParserStream {
    #lexer: lexer.Lexer

    constructor(value: string, tracker:Tracker) {
        this.#lexer = new lexer.Lexer(value)
        this.tracker = tracker
    }

    get lexer() { return this.#lexer }

    #errors: Error[] = []

    get errors() { return this.#errors }

    readonly tracker: Tracker

    hasErrors():boolean { return this.#errors.length > 0 }

    pushError(t:TextRange, msg:string) {
        this.errors.push({
            start: t.start,
            end: t.end,
            message: msg
        })
    }

    peek(): lexer.Token { return this.#lexer.peek() }
    next(): lexer.Token { return this.#lexer.next() }

    mkTracked<T>(r:TextRange, v:T): TrackedValue<T> {
        return { trackId: this.tracker.track(r) ?? -1, value: v }
    }
}

function consumeOperator(p: ParserStream, v:string):lexer.OperatorToken|undefined {
    let t = p.peek()
    if (t.kind === '#operator' && t.value === v) {
        p.next()
        return t
    }

    p.pushError(t, "Expected '" + v + "'")
    return undefined
}

/////////////////////////////////////////////////////////////////////////////////////

type Parser<T> = (p: ParserStream) => T|undefined


function parseFailure(p:ParserStream, t:TextRange, msg:string) : undefined {
    p.pushError(t, msg)
    return undefined
}

/**
 * If the next element in stream is an identifier, then it parses it.
 *
 * Otherwise, it leaves the stream unchanged and returns undefined
 */
const identParser : Parser<lexer.Identifier> = (p: ParserStream) => {
    let t = p.peek()
    switch (t.kind) {
    case '#keyword':
        p.next()
        return t
    default:
        return undefined
    }
}


/**
 * Parse a number field
 */
const numberField: Parser<number> = (p:ParserStream) => {

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
    return t.value
}


/**
 * Parse a number field that has tracking information so it can be updated.
 */
const trackedNumberField: Parser<TrackedValue<number>> = (p:ParserStream) => {

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
    return p.mkTracked(t, t.value)
}

/**
 * Parse a string field
 */
const stringParser: Parser<TextLocated<string>> = (p:ParserStream) => {
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
    for (const e of t.errors)
        p.pushError(e, e.message)
    return mkLocated(t, t.value)
}

/**
 * Parse a string field
 */
const stringField: Parser<StringField> = (p:ParserStream) => {
    const r = stringParser(p)
    if (!r) return undefined
    return { value: r.value }
}

/**
 * Parse a string field
 */
const lengthParser: Parser<A.Length> = (p:ParserStream) => {
    const t = stringParser(p)
    if (t === undefined) return undefined

    // Read column from end
    const token = t.value
    const num = lexer.readDigits(token, 0)
    if (num.count === 0) {
        p.pushError(t, "Expected units to start with number.")
        return undefined
    }

    // Character index to read next
    let idx = num.count
    // Value read
    let value = num.value

    if (token.charAt(idx) === '.') {
        idx++ // Skip period
        // Read fractional part and update idx & value
        const frac = lexer.readDigits(token, idx)
        if (frac.count === 0) {
            p.pushError(t, "Invalid fractional part.")
            return undefined
        }
        idx += frac.count
        value = value + frac.value / 10 ** frac.count
    }

    let choices:string[] = [A.Units.IN, A.Units.CM]
    let unitsString = token.slice(idx)
    if (unitsString === "") {
        p.pushError(t, "Units missing: " + choicesMessage(choices))
        return undefined
    } else if (choices.indexOf(unitsString) === -1) {
        p.pushError(t, "Could not parse units: " + choicesMessage(choices))
        return undefined
    }

    return {value: value, units: unitsString as A.Units }
}

function choicesMessage(choices:string[]): string {
    switch (choices.length) {
    case 0:
        return "internal error: Invalid enumerator choices."
    case 1:
        return "Expected '" + choices[0] + "'."
    default:
        let v = "Expected '" + choices[0]
        for (let i = 1; i < choices.length - 1; ++i)
            v = v + "', '" + choices[i]
        return v + "' or '" + choices[choices.length-1] + "'."
    }
}

/** A enum with a finite number of choices encoded as a keyword */
function enumField(choices:string[]): Parser<string> {
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

        return t.value
    }
}

/** A tracked enum with a finite number of choices encoded as a keyword */
function trackedEnumField(choices:string[]): Parser<TrackedValue<string>> {
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

        return p.mkTracked(t, t.value)
    }
}

/**
 * Parse a location
 */
const locationField: Parser<A.SourceLocation> = (p:ParserStream) => {
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

    const loc: A.SourceLocation = {
        filename: filename,
        line: linePair.value,
        column: colPair.value
    }

    return loc
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

function reqObjField<T>(nm: string, tp:Parser<T>, lexName?: string):ObjectField {
    return {
        fieldName: nm,
        lexName: lexName ? lexName : nm,
        arity: Arity.Required,
        setter: (p: ParserStream, obj:any, key:lexer.Identifier) => {
            if (obj[nm] !== undefined) {
                p.pushError(key, nm + " already defined.")
                return false
            }

            if (!consumeOperator(p, ':')) {
                obj[nm] = null
                p.lexer.skipToNewLine()
                return false
            }

            const r = tp(p)
            if (r === undefined) {
                obj[nm] = null
                p.lexer.skipToNewLine()
                return false
            }

            if (!consumeOperator(p, ';')) {
                obj[nm] = null
                p.lexer.skipToNewLine()
                return false
            }

            obj[nm] = r
            return true
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

interface Partial {
    [index: string]: any;
}


/**
 * Call one of matches if next token is a keyword that matches.
 *
 * @param fields List of keyword actions to match against.
 * @returns true if a match is found
 */
function objectType(fields: ObjectField[],
                    p: ParserStream,
                    obj: Partial,
                    fieldName: string,
                    tkn: lexer.Identifier): boolean {
    const name = identParser(p)
    if (!name) {
        p.lexer.skipToNewLine
        return false
    }
    if (!consumeOperator(p, '{')) {
        p.lexer.skipToNewLine
        return false
    }

    // Initialize partial object
    let nameField : A.StringField = { value: name.value }
    let partial: Partial = {name: nameField}
    for (const c of fields) {
        if (c.arity === Arity.Array)
            partial[c.fieldName] = []
    }

    let rcurly:lexer.OperatorToken|undefined = undefined
    let errorCount = p.errors.length

    while (!rcurly) {

        let t = p.peek()
        // Keep parsing while we get keywords
        switch (t.kind) {
        case '#end':
            p.pushError(t, 'Unexpected end of stream')
            return false
        case '#keyword':
            {
                let found = false
                for (const c of fields) {
                    if (t.value !== c.lexName) continue
                    found = true
                    p.next() // Read keyword
                    c.setter(p, partial, t)
                    break
                }
                if (!found) {
                    p.pushError(t, 'Unknown keyword ' + t.value)
                    p.lexer.skipToNewLine()
                }
                break
            }
        case '#operator':
            if (t.value === '}') {
                p.next()
                rcurly = t
                break
            }
        default:
            p.pushError(t, 'Unexpected token')
            p.lexer.skipToNewLine()
            break
        }
    }

    if (p.errors.length > errorCount) return false

    // Check fields are defined.
    let r : TextRange = { start: tkn.start, end: rcurly.end }
    let hasUndefined = false
    for (const c of fields) {
        if (c.arity === Arity.Required && partial[c.fieldName] === undefined) {
            hasUndefined = true
            p.pushError(r, "Missing " + c.lexName + ".")
        }
    }
    if (hasUndefined) return false

    obj[fieldName].push(partial)
    return true
}

const portType: ObjectField[] = [
    reqObjField('location', locationField),
    reqObjField('border', trackedEnumField([A.Border.Left, A.Border.Right, A.Border.Top, A.Border.Bottom])),
    reqObjField('offset', trackedNumberField)
]

/** Parser for actors */
const actorType: ObjectField[] = [
    reqObjField('location', locationField),
    reqObjField('left',   trackedNumberField),
    reqObjField('top',    trackedNumberField),
    reqObjField('width',  trackedNumberField),
    reqObjField('height', trackedNumberField),
    reqObjField('color',  stringField),
    arrayObjField('inPorts', 'in_port', portType),
    arrayObjField('outPorts', 'out_port', portType),
]

/** Parser for bus */
const busType: ObjectField[] = [
    reqObjField('orientation', enumField([A.BusOrientation.Horizontal, A.BusOrientation.Vertical])),
    reqObjField('left',   numberField),
    reqObjField('top',    numberField),
    reqObjField('width',  numberField),
    reqObjField('height', numberField),
]

/**
 * Call one of matches if next token is a keyword that matches.
 *
 * @param choices List of keyword actions to match against.
 * @returns true if a match is found
 */
function topLevelDecls<T>(p: ParserStream, choices: ObjectField[]): T | undefined {
    let partial: Partial = {}
    for (const c of choices) {
        if (c.arity === Arity.Array)
            partial[c.fieldName] = []
    }

    let recovering: Boolean = false
    let e : lexer.EndToken|undefined = undefined
    while (!e) {

        let t = p.next()
        switch (t.kind) {
        case '#keyword':
            let found = false
            for (const c of choices) {
                if (t.value === c.lexName) {
                    found = true
                    recovering = !c.setter(p, partial, t)
                    break
                }
            }
            if (!found && !recovering) {
                p.pushError(t, 'Unexpected identifier ' + t.value)
                recovering = true
            }
            break
        case '#end':
            e = t
            break
        case '#error':
            if (!recovering) {
                p.pushError(t, t.message)
                recovering = true
            }
            break
        default:
            if (!recovering) {
                p.pushError(t, 'Expected top level declaration.')
                recovering = true
            }
            break
        }
    }

    let r : TextRange = { start: e.start, end: e.end }
    if (p.hasErrors()) return undefined

    let complete:boolean = true
    for (const c of choices) {
        if (c.arity === Arity.Required) {
            const v = partial[c.fieldName]
            complete = v && complete
            if (v === undefined)
                p.pushError(r, 'Missing ' + c.lexName + '.')
        }
    }
    return complete ? (partial as T) : undefined
}

function matchOperator(p: ParserStream, v:string):lexer.OperatorToken|undefined {
    let t = p.peek()
    if (t.kind === '#operator' && t.value === v) {
        p.next()
        return t
    } else {
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
function consumeLayout(p:ParserStream): A.SystemModel|undefined {


    return topLevelDecls<A.SystemModel>(p, [
        reqObjField('pagewidth', lengthParser),
        reqObjField('pageheight', lengthParser),
        reqObjField('width', numberField),
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
}



export type ParseResult = { value: A.SystemModel | undefined
                          , errors: Error[]
                          }


export function parseArchitectureFile(tracker: Tracker, text: string): ParseResult {
    const p = new ParserStream(text, tracker)
    const r = consumeLayout(p)
    return {
        // This is a pure grammar, the value will be undefined until we add embedded actions
        // or enable automatic CST creation.
        value: r,
        errors: p.errors
    }
}