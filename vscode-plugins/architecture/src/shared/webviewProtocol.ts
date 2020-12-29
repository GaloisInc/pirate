import { SystemLayout } from './architecture.js'

export namespace extension {

	export const enum Tag {
		SetSystemLayout = "SetSystemLayout"
	}

	/**
	 * Base interface for updates passed from vscode extension to webview.
	 */
	export type Event = SetSystemLayout

	/**
	 * Notify view of new system layout.
	 * 
	 * The extension will expect a response `SetSystemLayoutDone` 
	 * back from webview once this is received.  While waiting it
	 * will ignore requests to update doc.
	 */
	export interface SetSystemLayout {
		readonly tag: Tag.SetSystemLayout
		/**
		 * New value for system layout
		 */
		readonly system: SystemLayout
	}
}

export namespace webview {
	export const enum Tag {
		VisitURI = "VisitURI",
		UpdateDoc = "UpdateDoc",
		SetSystemLayoutDone = "SetSystemLayoutDone"
	}

	/** Type for events from view to extension. */
	export type Event = VisitURI | UpdateDoc | SetSystemLayoutDone

	/**
	 *  Class for commands to visit a URI in the editor.
	 */
	export interface VisitURI {
		readonly tag: Tag.VisitURI

		readonly filename: string;
		readonly line: number;
		readonly column: number;
	}

	export interface DocEdit {
		/** Index of tracked change */
		readonly locationId: number
		/** Value to use for replacement. */
		readonly newText: string
	}

	/** Request extension modify the underlying document for webview. */
	export interface UpdateDoc {
		readonly tag: Tag.UpdateDoc
		readonly changes: DocEdit[]
	}

	/**
	 * Notify extension that model has been updated.
	 */
	export interface SetSystemLayoutDone {
		readonly tag: Tag.SetSystemLayoutDone
	}
}