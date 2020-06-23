import * as U from "../shared/modelUpdates";

/**
 * File location for an identifier.
 */
export interface FileLocation {
	readonly uri: string;
	readonly line: number;
	readonly column: number;
}

/**
 * This represents the components of a particular configuration
 * of a system.
 */
export class Configuration {

	/**
	 * Construct a new empty system model.
	 *
	 * @param notifyFn Function to invoke when model needs to inform the view of a change.
	 */
    constructor(notifyFn:(x:U.ModelUpdate) => void) {
        this.#notifyFn = notifyFn;
    }

    #notifyFn:(x:U.ModelUpdate) => void;
    /**
     * Send an update to the webview.
     *
     * @param update Update to send.
     */
    sendUpdate(update:U.ModelUpdate):void {
		this.#notifyFn(update);
    }

	/** Services added so far to system model. */
	#services:Map<string, Service>=new Map<string, Service>();

	/**
	 * This adds an extension service to a system model.
	 *
	 * It should only be called by ExtensionService constructor.
	 */
	addService(s:Service) {
		if (this.#services.has(s.name)) {
			throw new Error("Duplicate service name " + s.name);
		}
		this.#services.set(s.name, s);
	}

    /**
     * Attempt to find a service with the given name.
     *
     * @param name Name of service.
     */
    findService(name:string) {
        return this.#services.get(name);
    }

	/** Records the number of nodes so that we can create new ones. */
	#nodeCount:number=0;
	addNode():number {
		return this.#nodeCount++;
	}
}

export class Service {
	/** Create a new extension service. */
	constructor(system:Configuration, name:string, classDefinition:FileLocation, coords:U.CoordsInterface) {
		this.#system = system;
		this.#name = name;
		this.#classDefinition = classDefinition;
        system.addService(this);
        let upd:U.NewService =
        {
            tag: U.Tag.NewService,
            name: name,
            coords: coords,
        };
        system.sendUpdate(upd);
    }

    #system: Configuration;
    get system() { return this.#system; }

	#name: string;
	/**
	 * Name of service
	 * Should be unique within system model.
	 */
	get name():string { return this.#name; }

    #classDefinition:FileLocation;
	/**
     * Class source location
     */
	get classDefinition() { return this.#classDefinition; }

    #inPorts :Map<string, InPort>  = new Map();
	#outPorts:Map<string, OutPort> = new Map();

	/**
	 * This adds an input port to a service.
	 *
	 * It should only be called by ExtensionInPort constructor.
	 */
	addInPort(s:InPort):void {
		if (this.#inPorts.has(s.name) || this.#outPorts.has(s.name)) {
			throw new Error("Duplicate port name " + s.name);
		}
		this.#inPorts.set(s.name, s);
	}

	/**
	 * This adds a output port to a service.
	 *
	 * It should only be called by ExtensionInPort constructor.
	 */
	addOutPort(s:OutPort):void {
		if (this.#inPorts.has(s.name) || this.#outPorts.has(s.name)) {
			throw new Error("Duplicate port name " + s.name);
		}
		this.#outPorts.set(s.name, s);
	}
}

/** Information about an input port. */
class InPort {

	constructor(service:Service, name:string, methodDefinition:FileLocation) {
		this.#service = service;
		this.#name = name;
		this.#methodDefinition = methodDefinition;
		service.addInPort(this);
	}

    #service:Service;
    get service() { return this.#service; }
    get system() { return this.#service.system; }

    #name:string;

	/**
	 * Name of input port.
	 */
	get name() { return this.#name; }

	#methodDefinition:FileLocation;
	/**
	 * Location of method definition for receiving messages.
	 */
	get methodDefinition() { return this.#methodDefinition; }
}

/** Information about an output port. */
class OutPort {
    /**
     * Construct a output port for a service.
     */
	constructor(service:Service, name:string, methodDefinition:FileLocation) {
		this.#service = service;
		this.#name = name;
		this.#methodDefinition = methodDefinition;
		service.addOutPort(this);
	}

    #service:Service;
    get service() { return this.#service; }
    get system() { return this.#service.system; }

    #name:string;
	/**
	 * Name of input port.
	 */
	get name() { return this.#name; }

	#methodDefinition:FileLocation;
	/**
	 * Location of method definition for receiving messages.
	 */
	get methodDefinition():FileLocation { return this.#methodDefinition; }
}

/**
 * A node is a graph entitity that channels can be connected to in-lieu of ports
 * for a more clear diagram.
 *
 * The channels should go from in-ports to out-ports without infinite paths through
 * nodes, but we do not currently check this.
 */
class Node {
	#system:Configuration;
    get system() { return this.#system; }

	/**
	 * Construct a new node for connecting services.
	 */
	constructor(system:Configuration) {
		this.#system = system;
    }
}

/**
 * Information about a channel that connects one source to
 *
 */
class Channel {
	constructor(source: OutPort|Node, target: InPort|Node) {
        if (source.system !== target.system) {
            throw new Error("Source and target must belong to same system.");
        }

		this.#source = source;
		this.#target = target;
	}

    #source: OutPort|Node;
	get source() { return this.#source; }
	#target: InPort|Node;
	get target() { return this.#target; }
}