# Open Layers Camera Demo

This project was generated with [Angular CLI](https://github.com/angular/angular-cli) version 11.1.4.

## Installation
* Install Node
* Install Angular CLI: `npm install -g @angular/cli`
* Install and run the backend Flask API in the `flask_api` directory (follow the directions in the flask_api README.md)
  * If not using the default host:port (localhost:5000), you must update the `locationUrl` in `src/app/location.service.ts`
* Run the npm magic incantations: `npm install`, `ng update`, `npm update`

## Development server

Run `ng serve` for a dev server. Navigate to `http://localhost:4200/`. The app will automatically reload if you change any of the source files. You want to use the Flask server (defaults to localhost:5000) as the endpoint you provide to the camera demo application.

## Code scaffolding

Run `ng generate component component-name` to generate a new component. You can also use `ng generate directive|pipe|service|class|guard|interface|enum|module`.

## Build

Run `ng build` to build the project. The build artifacts will be stored in the `dist/` directory. Use the `--prod` flag for a production build.

## Running unit tests

Run `ng test` to execute the unit tests via [Karma](https://karma-runner.github.io).

## Running end-to-end tests

Run `ng e2e` to execute the end-to-end tests via [Protractor](http://www.protractortest.org/).

## Further help

To get more help on the Angular CLI use `ng help` or go check out the [Angular CLI Overview and Command Reference](https://angular.io/cli) page.
