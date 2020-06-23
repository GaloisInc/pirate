
export
interface CoordsInterface {
	readonly left: number;
	readonly top: number;
	readonly width: number;
	readonly height: number;
}

export const enum Tag {
	NewService = "newService",
	NewPort    = "newPort",
	NewChannel = "newChannel"
}

// Base interface for updates passed from model in extension to webview.
//
// To add a new update command introduce the command to the tag, and define
// an interface for storing command data (if needed).
export interface ModelUpdate {
	/** Constant with command kind. */
	readonly tag: Tag;
}

/**
 * Notify view of new service -- `tag` should be `NewService`.
 */
export interface NewService extends ModelUpdate {
	/** Unique name for service. */
	readonly name: string;
	/** Coordinates for placing rectangle */
	readonly coords: CoordsInterface;
}

export interface NewPort extends ModelUpdate {
	/** Name of service this port belongs to. */
	readonly serviceName: string;
	/** Name of new port.  Should be unique for service. */
	readonly portName: string;
	/** Coordinates for placing rectangle */
	readonly coords: CoordsInterface;
}

/**
 * Identifier to uniquely describe a port.
 */
interface PortId {
	readonly serviceName: string;
	readonly portName: string;
}

/**
 * Command to create new channel in webview interface.
 */
export interface NewChannel extends ModelUpdate {
	/**
	 * Port for source.
	 */
	readonly source: PortId|number;
	/**
	 * Port for target.
	 */
	readonly target: PortId|number;
}