/**
 * This file contains utilities for working with SVGs.
 */

export const ns = "http://www.w3.org/2000/svg"

 /**
 * This sets the base value units of an animated length
 * in scaled coordinates.
 */
export function setUserUnits(al:SVGAnimatedLength, v:number) {
    const l = al.baseVal
    l.newValueSpecifiedUnits(l.SVG_LENGTHTYPE_NUMBER, v)
}

/**
 * This sets the base value units of an animated length
 * in inches.
 */
export function setInchUnits(al:SVGAnimatedLength, v:number) {
    const l = al.baseVal
    l.newValueSpecifiedUnits(l.SVG_LENGTHTYPE_IN, v)
}

/**
 * This sets the base value units of an animated length
 * as a percentage.
 */
export function setCmUnits(al:SVGAnimatedLength, v:number) {
    const l = al.baseVal
    l.newValueSpecifiedUnits(l.SVG_LENGTHTYPE_CM, v)
}

/**
 * This sets the base value units of an animated length
 * as a percentage.
 */
export function setPercentageUnits(al:SVGAnimatedLength, v:number) {
    const l = al.baseVal
    l.newValueSpecifiedUnits(l.SVG_LENGTHTYPE_PERCENTAGE, v)
}
