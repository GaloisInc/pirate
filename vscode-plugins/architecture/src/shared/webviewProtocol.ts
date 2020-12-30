import { SystemModel } from './architecture.js'

// Namespace for types used in multiple protocol directions.
export namespace common {
	export interface ModifyString {
		/** Index of tracked change */
		readonly trackIndex: number
		/** Value to use for replacement. */
		readonly newText: string
	}
}

// Namespace for messages and types passed from extension to webview
export namespace extension {

	export const enum Tag {
		SetSystemModel,
		DocumentEdited
	}

	/**
	 * Base interface for updates passed from vscode extension to webview.
	 */
	export type Event = SetSystemModel | DocumentEdited

	/**
	 * Notify view of new system layout.
	 *
	 * The extension will expect a response `SetSystemLayoutDone`
	 * back from webview once this is received.  While waiting it
	 * will ignore requests to update doc.
	 */
	export interface SetSystemModel {
		readonly tag: Tag.SetSystemModel
		/**
		 * New value for system layout
		 */
		readonly system: SystemModel
	}

	export type ModifyString = common.ModifyString

	/** Notify webview that some tracked regions were modified. */
	export interface DocumentEdited {
		readonly tag: Tag.DocumentEdited
		readonly edits: readonly ModifyString[]
	}
}

// Namespace for messages and types passed from webview to extension
export namespace webview {
	export const enum Tag {
		VisitURI,
		UpdateDocument,
		SetSystemModelDone
	}

	/** Type for events from view to extension. */
	export type Event = VisitURI | UpdateDocument | SetSystemModelDone

	/**
	 *  Class for commands to visit a URI in the editor.
	 */
	export interface VisitURI {
		readonly tag: Tag.VisitURI

		readonly filename: string;
		readonly line: number;
		readonly column: number;
	}

	export type ModifyString = common.ModifyString

	/** Request extension modify the underlying document for webview. */
	export interface UpdateDocument {
		readonly tag: Tag.UpdateDocument
		readonly changes: readonly ModifyString[]
	}

	/**
	 * Notify extension that model has been updated.
	 */
	export interface SetSystemModelDone {
		readonly tag: Tag.SetSystemModelDone
	}
}