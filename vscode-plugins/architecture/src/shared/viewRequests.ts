export const enum Tag {
	VisitURI = "visitURI"
}

/** Base interface for commands given to extension. */
export type ViewRequest = VisitURI

/**
 *  Class for commands to visit a URI in the editor.
 */
export interface VisitURI {
	readonly tag: Tag.VisitURI

	readonly filename: string;
	readonly line: number;
	readonly column: number;
}