import * as A from './architecture.js'

// Namespace for types used in multiple protocol directions.
export namespace common {
    export interface TrackUpdate {
        /** Index of tracked change */
        readonly trackIndex: A.TrackIndex
        /** Value to use for replacement. */
        readonly newText: string
    }
}

// Namespace for messages and types passed from extension to webview
export namespace extension {

    export const enum Tag {
        SetSystemModel,
        InvalidateModel,
        DocumentEdited
    }

    /**
     * Base interface for updates passed from vscode extension to webview.
     */
    export type Event = SetSystemModel | InvalidateModel | DocumentEdited

    /**
     * Notify view of new system layout.
     *
     * The extension will expect a response (see `webview.SetSystemModelDone`)
     * back from webview once this is received.  While waiting for this
     * response, the extension will drop any notifications from the webview.
     */
    export interface SetSystemModel {
        readonly tag: Tag.SetSystemModel
        /**
         * New value for system layout
         */
        readonly system: A.SystemModel
    }

    /**
     * Notify webview that system model is done.
     */
    export interface InvalidateModel {
        readonly tag: Tag.InvalidateModel
    }

    /** Notify webview that some tracked regions were modified. */
    export interface DocumentEdited {
        readonly tag: Tag.DocumentEdited
        readonly edits: readonly common.TrackUpdate[]
    }
}

// Namespace for messages and types passed from webview to extension
export namespace webview {
    export const enum Tag {
        Ready, // Indicates webview is now listening.
        VisitURI,
        UpdateDocument,
        SetSystemModelDone
    }

    /** Type for events from view to extension. */
    export type Event = Ready | VisitURI | UpdateDocument | SetSystemModelDone

    /** Indicates webview is now ready for other events. */
    export interface Ready {
        readonly tag: Tag.Ready
    }

    /**
     *  Command to visit a URI in the editor.
     */
    export interface VisitURI {
        readonly tag: Tag.VisitURI

        readonly locationIdx: A.LocationIndex
    }

    /** Request extension modify the underlying document for webview. */
    export interface UpdateDocument {
        readonly tag: Tag.UpdateDocument
        readonly edits: readonly common.TrackUpdate[]
    }

    /**
     * Notify extension that model has been updated.
     */
    export interface SetSystemModelDone {
        readonly tag: Tag.SetSystemModelDone
    }
}