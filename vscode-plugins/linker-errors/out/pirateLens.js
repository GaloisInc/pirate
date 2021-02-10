"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.PirateLens = void 0;
// pirateLens
const vscode = require("vscode");
const path = require("path");
class PirateLens {
    constructor(lr) {
        this.codeLenses = [];
        this.linkerRunner = lr;
    }
    parseLocation(loc) {
        const parts = loc.split(':');
        return [parts[0], Number.parseInt(parts[1])];
    }
    provideCodeLenses(document, _token) {
        console.log('providing code lenses');
        let lenses = new Array();
        const json = this.linkerRunner.latestJSON;
        json.forEach((item) => {
            let [file, line] = this.parseLocation(item.location);
            if (path.basename(file) === path.basename(document.uri.fsPath)) {
                const range = new vscode.Range(line - 1, 0, line - 1, 999);
                let cmd = {
                    title: "Function has capability '" + item.caps + "'",
                    tooltip: "Function has capability '" + item.caps + "'",
                    command: '',
                };
                lenses.push(new vscode.CodeLens(range, cmd));
            }
        });
        return lenses;
    }
}
exports.PirateLens = PirateLens;
//# sourceMappingURL=pirateLens.js.map