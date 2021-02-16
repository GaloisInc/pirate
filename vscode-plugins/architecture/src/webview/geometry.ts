export interface Bottom<T> { readonly bottom: T }
export interface Height<T> { readonly height: T }
export interface Left<T> { readonly left: T }
export interface Right<T> { readonly right: T }
export interface Top<T> { readonly top: T }
export interface Width<T> { readonly width: T }

export type XRange<T> = Left<T> & Width<T>
export type YRange<T> = Top<T> & Height<T>

export type XYRange<T> = XRange<T> & YRange<T>

export type LeftTopPosition<T> = Left<T> & Top<T>
export type BottomRightPosition<T> = Bottom<T> & Right<T>

export type Rectangle<T> = LeftTopPosition<T> & BottomRightPosition<T>
