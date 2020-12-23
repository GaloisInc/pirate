import { SystemLayout } from './architecture.js'

export const enum Tag {
	SetSystemLayout = "SetSystemLayout"
}

// Base interface for updates passed from model in extension to webview.
//
// To add a new update command introduce the command to the tag, and define
// an interface for storing command data (if needed).
export type ModelUpdate = SetSystemLayout

/**
 * Notify view of new service -- `tag` should be `NewService`.
 */
export interface SetSystemLayout {
	readonly tag: Tag.SetSystemLayout
	/**
	 * New value for system layout
	 */
	readonly system: SystemLayout
}