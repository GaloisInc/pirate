/* eslint-disable */

const TsconfigPathsPlugin = require('tsconfig-paths-webpack-plugin')

const path = require('path')

module.exports = {
    devtool: 'source-map',
    entry: {
        extension: './src/extension/main.ts',
        webview: './src/webview/webview.tsx'
    },
    externals: {
        'vscode': 'commonjs vscode' // the vscode-module is created on-the-fly and must be excluded. Add other modules that cannot be webpack'ed, 📖 -> https://webpack.js.org/configuration/externals/
    },
    mode: 'development',
    module: {
        rules: [
            {
                exclude: /node_modules/,
                test: /\.tsx?$/,
                use: {
                    loader: 'ts-loader',
                    options: {
                        projectReferences: true
                    }
                },
            }
        ],
    },
    output: {
        devtoolModuleFilenameTemplate: '../[resource-path]',
        filename: '[name].bundle.js',
        libraryTarget: 'commonjs2',
        path: path.resolve(__dirname, 'out')
    },
    resolve: {
        extensions: ['.tsx', '.ts', '.js'],
        plugins: [new TsconfigPathsPlugin({
            // options here
        })]
    },
    target: 'node'
}
