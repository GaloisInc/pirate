export const enum Border {
	Top = "top",
	Left = "left",
	Right = "right",
	Bottom = "bottom"
}

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
	/** Color of the service. */
	readonly color: string;
}

export const enum PortType {
    InPort = "in",
    OutPort = "out"
}

export interface NewPort extends ModelUpdate {
	/** Name of service this port belongs to. */
	readonly serviceName: string;
	/** Name of new port.  Should be unique for service. */
	readonly portName: string;
	/** Flag controlling whether port is for receiving (in) or sending (out). */
	readonly mode: PortType;
	/** Side that port is on in model */
	readonly border: Border;
	/**
	 * Position of port on border.
	 * On Top/Bottom border this is units from left.
	 * On Left/Right border this is units from top.
	 */
	readonly position: number;
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