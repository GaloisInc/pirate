


/** Description of input or output port on service. */
interface Port {
	/** Name of port.  Names for ports should be unique on a service.  */
	readonly name:string;
}



export const enum Tag {
	VisitServiceClass = "visitServiceClass"
}

/** Base interface for commands given to extension. */
export interface ViewRequest {
	readonly tag: Tag;
}

/**
 *  Class for commands to visit a service.
 *
 * `kind` should be "visitService".
 */
export interface VisitServiceClass extends ViewRequest {
	/** Name of class to visit. */
	readonly name:string;
}