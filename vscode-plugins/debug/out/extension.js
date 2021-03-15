"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.deactivate = exports.activate = exports.PirateCppConfigurationProvider = void 0;
// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
const vscode = require("vscode");
const child_process = require("child_process");
const fs = require("fs");
const path = require("path");
class PirateCppConfigurationProvider {
    constructor(config) {
        this.config = config;
    }
    provideDebugConfigurations(folder, _token) {
        return __awaiter(this, void 0, void 0, function* () {
            const configs = [];
            if (!folder) {
                vscode.window.showErrorMessage("Debugging only works with a workspace folder, not standalone files");
                return configs;
            }
            const dir = path.join(folder.uri.fsPath, ".pirate");
            let pdlPath = this.config.get("pdlPath");
            if (!pdlPath) {
                pdlPath = "pdl";
            }
            if (!this.runPDL(dir, folder, pdlPath)) {
                return configs;
            }
            // Create one configuration per YAML file that PDL has generated
            const pdlOutFiles = fs.readdirSync(dir);
            const yamlFiles = pdlOutFiles.filter((file, _idx, _arr) => {
                return file.endsWith(".yaml");
            });
            let palPath = this.config.get('palPath');
            if (!palPath) {
                palPath = 'pal';
            }
            yamlFiles.forEach((file, _idx, _arr) => {
                const palConfig = {
                    type: "cppdbg",
                    name: file.split(".")[0],
                    request: "launch",
                    program: palPath,
                    args: [file],
                    cwd: dir,
                    setupCommands: [{ text: "-gdb-set follow-fork-mode child" }],
                };
                configs.push(palConfig);
            });
            return configs;
        });
    }
    runPDL(workdir, folder, pdlPath) {
        const procOpts = {
            cwd: folder.uri.fsPath,
            encoding: "utf8",
            shell: true,
        };
        const cmd = pdlPath + " debug " + workdir + " pdl.yaml";
        const pdlResult = child_process.spawnSync(cmd, procOpts);
        if (pdlResult.error) {
            const errText = pdlResult.stderr.toString("utf8");
            vscode.window.showErrorMessage("Error from PDL: " + errText);
            return false;
        }
        return true;
    }
}
exports.PirateCppConfigurationProvider = PirateCppConfigurationProvider;
// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
function activate(context) {
    // Use the console to output diagnostic information (console.log) and errors (console.error)
    // This line of code will only be executed once when your extension is activated
    console.log('Congratulations, your extension "piratedebug" is now active!');
    const config = vscode.workspace.getConfiguration("piratedebug");
    const provider = new PirateCppConfigurationProvider(config);
    context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider("Pirate", provider));
    // The command has been defined in the package.json file
    // Now provide the implementation of the command with registerCommand
    // The commandId parameter must match the command field in package.json
    // let command = vscode.commands.registerCommand("extension.helloWorld", () => {
    //   // The code you place here will be executed every time your command is executed
    //   // Display a message box to the user
    //   vscode.window.showInformationMessage("Hello World!");
    // });
    // context.subscriptions.push(command);
}
exports.activate = activate;
// this method is called when your extension is deactivated
function deactivate() { }
exports.deactivate = deactivate;
//# sourceMappingURL=extension.js.map