/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------------------------------------------*/

import * as assert from 'assert';
import * as path from 'path';
import { PerformanceEntry } from 'perf_hooks';

import { output, exec as shellExec } from '../../test/core/testUtils';
import { plainExec, runCommandNoPty } from '../../src/common/commonUtils';
import { Result } from '../../src/node/devContainersCLI';
import { readLocalFile } from '../../src/utils/pfs';

describe('Dev Containers CLI', function () {
	this.timeout(1 * 60 * 1000);

	it('Concurrent Git Clone', async () => {
		
		const image = 'debian:10.7';
		const workspaceFolder = path.join(__dirname, 'repos', 'concurrent-git-clone');
		const overrideConfig = path.join(__dirname, 'configs', '.devcontainer.json');
		const pkgPath = path.join(__dirname, '..');
		const exec = plainExec(pkgPath);

		await shellExec(`rm -rf ${workspaceFolder}`);
		await shellExec(`if [ ! -z "$(docker images -q ${image})" ]; then docker rmi ${image}; fi`);

		{
			const logFile = path.join(__dirname, 'logs', 'concurrent-git-clone.log');
			const performanceLogFile = path.join(__dirname, 'logs', 'concurrent-git-clone.performance.log');
			const { stdout } = await runCommandNoPty({
				exec,
				cmd: 'node',
				args: [
					'--enable-source-maps',
					'cli.js',
					'--workspace-folder', workspaceFolder,
					'--override-config', overrideConfig,
					'--remove-existing-container',
					'--git-clone-url', 'https://github.com/microsoft/vscode-remote-containers.git',
					'--git-clone-token-env-var', 'GITHUB_TOKEN',
					'--terminal-log-file', 'stderr',
					'--log-file', logFile,
					'--performance-log-file', performanceLogFile,
				],
				output,
				print: 'continuous',
			});

			const result: Result = JSON.parse(stdout.toString());
			assert.strictEqual(result.outcome, 'success', stdout.toString());
	
			const { stdout: status } = await shellExec(`docker inspect -f '{{json .State.Status}}' ${result.containerId}`);
			assert.strictEqual(status.toString().trim(), '"running"');
	
			await shellExec(`test -d ${workspaceFolder}/.git`);
	
			const performanceLogLines = (await readLocalFile(performanceLogFile))
				.toString()
				.split(/\r?\n/)
				.filter(line => !!line);
			const performanceLog: PerformanceEntry[] = JSON.parse(performanceLogLines[performanceLogLines.length - 1]);
			const clone = performanceLog.find(entry => entry.name.indexOf('git clone') !== -1);
			assert.ok(clone, 'git clone not found');
			const pull = performanceLog.find(entry => entry.name.indexOf('docker pull') !== -1);
			assert.ok(pull, 'docker pull not found');
			assert.ok(clone.startTime < pull.startTime + pull.duration, 'clone after pull');
			assert.ok(pull.startTime < clone.startTime + clone.duration, 'pull after clone');
	
			await shellExec(`docker rm -f ${result.containerId}`);
		}

		{
			const logFile = path.join(__dirname, 'logs', 'concurrent-git-clone.2.log');
			const performanceLogFile = path.join(__dirname, 'logs', 'concurrent-git-clone.performance.2.log');
			const { stdout } = await runCommandNoPty({
				exec,
				cmd: 'node',
				args: [
					'--enable-source-maps',
					'cli.js',
					'--workspace-folder', workspaceFolder,
					'--override-config', overrideConfig,
					'--remove-existing-container',
					'--git-clone-url', 'https://github.com/microsoft/vscode-remote-containers.git',
					'--git-clone-token-env-var', 'GITHUB_TOKEN',
					'--terminal-log-file', 'stderr',
					'--log-file', logFile,
					'--performance-log-file', performanceLogFile,
				],
				output,
				print: 'continuous',
			});

			const result: Result = JSON.parse(stdout.toString());
			assert.strictEqual(result.outcome, 'success', stdout.toString());
	
			const { stdout: status } = await shellExec(`docker inspect -f '{{json .State.Status}}' ${result.containerId}`);
			assert.strictEqual(status.toString().trim(), '"running"');
	
			const performanceLogLines = (await readLocalFile(performanceLogFile))
				.toString()
				.split(/\r?\n/)
				.filter(line => !!line);
			const performanceLog: PerformanceEntry[] = JSON.parse(performanceLogLines[performanceLogLines.length - 1]);
			const clone = performanceLog.find(entry => entry.name.indexOf('git clone') !== -1);
			assert.ok(!clone, 'git clone found');
	
			await shellExec(`docker rm -f ${result.containerId}`);
			await shellExec(`rm -rf ${workspaceFolder}`);
		}
	});
});
